#include "core/jobs/job_system.h"

#include "core/asserts.h"
#include "core/jobs/job_queue.h"

#include <atomic>
#include <bit>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <optional>
#include <stop_token>
#include <thread>

#if defined(VE_PLATFORM_LINUX)
#include <pthread.h>
#include <sched.h>
#elif defined(VE_PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace Vulkyrie {

    namespace {

        /** @brief Hard cap on spawned worker threads (queue scans are O(workers)). */
        constexpr u32 MAX_WORKER_THREADS = 128;

        /** @brief Failed work-search rounds a worker spins (yielding) before futex-sleeping. */
        constexpr u32 SPIN_ROUNDS_BEFORE_SLEEP = 64;

        /** @brief Mask extracting the low half of a packed {generation, index} pair. */
        constexpr u64 LOW_32_BITS = 0xFFFFFFFFULL;

        /** @brief How long an exhausted pool may fail to make any progress before aborting loudly.
         * Slots/edges held by created-but-unscheduled jobs are not drainable, so spinning could
         * otherwise livelock forever (see AcquireSlot). */
        constexpr std::chrono::seconds EXHAUSTION_STALL_TIMEOUT{ 5 };

        /** @brief Tracks a no-progress streak on a pool-exhaustion path and aborts (loudly) once
         * the stall timeout elapses — a diagnosable failure instead of a silent livelock. */
        class ExhaustionStallGuard {
        public:
            /** @brief Reports that the pool made — or could still make — progress, clearing any
             * streak so the abort countdown restarts from zero at the next failure. */
            VE_INLINE void OnProgress() {
                _stalled = false;
            }

            /** @brief Reports another failed attempt to drain the pool. The first one only starts
             * the clock; the process aborts once `EXHAUSTION_STALL_TIMEOUT` has elapsed with no
             * intervening `OnProgress`, so a momentary exhaustion is never mistaken for a stall.
             * @param poolName The exhausted pool, named in the fatal diagnosis.
             */
            void OnNoProgress(const char *poolName) {
                const auto now = std::chrono::steady_clock::now();

                if (!_stalled) {
                    _stalled = true;
                    _stallStart = now;
                    return;
                }

                if (now - _stallStart >= EXHAUSTION_STALL_TIMEOUT) {
                    // State the observation, not a guessed cause: this fires only after the pool
                    // stayed exhausted with no job dispatched (queued or executing) anywhere for
                    // the whole window - nothing could have freed a slot or an edge.
                    VFATAL("JobSystem: {} exhausted and no job was dispatched (queued or executing) for {} seconds - "
                           "nothing can drain the pool (jobs were created but never scheduled, or the pool is smaller "
                           "than the graph being built). Raise the pool size in JobSystemConfig or schedule the graph "
                           "as it is built. Aborting to prevent livelock.",
                           poolName,
                           EXHAUSTION_STALL_TIMEOUT.count());
                    std::abort();
                }
            }

        private:
            bool _stalled = false;
            std::chrono::steady_clock::time_point _stallStart{};
        };

        /** @brief An entry in the successor lists, threaded through a preallocated pool. `Next` is
         * also the link while the edge sits on the free list. */
        struct JobEdge {
        public:
            /** @brief The dependent job (packed handle); written before the publishing CAS on the
             * predecessor's `FirstEdge`, read only after acquiring that head — never racy. */
            u64 PackedSuccessor = 0;

            /** @brief Next edge in the successor list / free list. */
            std::atomic<u32> Next{ INVALID_JOB_EDGE };
        };

        /** @brief Everything the running job system owns; created by `Initialize`, destroyed by
         * `Shutdown`. Published through an atomic pointer so pre-`Initialize` (lazy) bootstrap is
         * race-free. */
        struct JobSystemState {
        public:
            JobSystemConfig Config;
            bool Implicit = false; ///< Lazily created zero-worker instance (replaceable by Initialize).

            u32 MaxJobs = 0;
            u64 JobIndexMask = 0;
            u32 MaxEdges = 0;
            u32 QueueCount = 1; ///< Spawned workers + 1 (index 0 is the main thread's queue).

            Scope<Job[]> Jobs;
            Scope<JobEdge[]> Edges;
            std::vector<Scope<JobQueue>> Queues;
            std::vector<std::jthread> Workers;

            /** @brief Bump cursor for job-slot claims (wraps via `JobIndexMask`). */
            std::atomic<u64> NextJobSlot{ 0 };

            /** @brief Bump cursor for never-yet-used edges; recycled edges come from the free list. */
            std::atomic<u64> NextEdge{ 0 };

            /** @brief ABA-tagged Treiber-stack head of recycled edges: {tag:32, index:32}. */
            std::atomic<u64> EdgeFreeListHead{ PackEdgeHead(0, INVALID_JOB_EDGE) };

            /** @brief Eventcount for idle workers: bumped (and notified) on every push, so a
             * value-checked `atomic::wait` can never miss a wakeup. */
            std::atomic<u32> WorkSignal{ 0 };
        };

        /** @brief The live state; atomic so the lazy-bootstrap check is race-free on any thread. */
        std::atomic<JobSystemState *> gState{ nullptr };

        /** @brief Serializes state creation/destruction (lazy bootstrap vs. explicit lifecycle). */
        std::mutex gLifecycleMutex;

        /** @brief The calling thread's queue index: 0 main, 1..N workers, -1 foreign. */
        thread_local i32 tQueueIndex = -1;

        /** @brief Per-thread xorshift32 state for steal-victim selection. */
        thread_local u32 tStealSeed = 0;

        /** @brief Number of jobs currently executing on this thread's stack (nesting comes from
         * Wait-assist and the inline cascade). The stall guard subtracts it so a thread blocked
         * inside one of its own jobs can never count that job as proof of system liveness. */
        thread_local u32 tExecutingDepth = 0;

        /** @brief Draws this thread's next xorshift32 value, used to choose a steal victim. The
         * seed is lazily derived from the thread id (and forced odd), so every thread starts its
         * scan at a different queue — uncoordinated victim choice is what keeps idle workers from
         * all piling onto queue 0. Since xorshift maps non-zero states to non-zero states, the
         * generator can never collapse to its zero fixed point.
         * @returns A non-zero pseudo-random value; callers reduce it modulo the queue count.
         */
        [[nodiscard]] u32 NextStealRandom() {
            u32 x = tStealSeed;

            if (0 == x) {
                x = static_cast<u32>(std::hash<std::thread::id>{}(std::this_thread::get_id()) & LOW_32_BITS) | 1U;
            }

            x ^= x << 13U;
            x ^= x >> 17U;
            x ^= x << 5U;
            tStealSeed = x;

            return x;
        }

        /** @brief Packs a handle into the single `u64` the queues and edges carry:
         * `{generation:32, index:32}`. Moving one word instead of a `JobHandle` keeps publishing a
         * queue slot to a single relaxed atomic store, and carrying the generation along is what
         * lets a dequeued entry be validated against its slot before it runs.
         * @param handle The handle to pack.
         * @returns The packed pair; unpack the index with `& JobIndexMask` and the generation with
         * `>> 32`.
         */
        [[nodiscard]] VE_INLINE u64 PackJobHandle(const JobHandle &handle) {
            return (static_cast<u64>(handle.Generation) << 32U) | (static_cast<u64>(handle.Index) & LOW_32_BITS);
        }

        /** @brief Defined below; declared here because completion is mutually recursive
         * (`ExecutePackedJob` -> `FinishJob` -> `OnJobReady` -> `ExecutePackedJob`). */
        void ExecutePackedJob(JobSystemState &state, u64 packedHandle);

        /** @brief Pushes a freed edge onto the ABA-tagged free list. */
        void FreeEdge(JobSystemState &state, u32 edgeIndex) {
            u64 head = state.EdgeFreeListHead.load(std::memory_order_relaxed);

            for (;;) {
                state.Edges[edgeIndex].Next.store(static_cast<u32>(head & LOW_32_BITS), std::memory_order_relaxed);
                const u64 newHead = (((head >> 32U) + 1U) << 32U) | static_cast<u64>(edgeIndex);

                if (state.EdgeFreeListHead.compare_exchange_weak(head, newHead, std::memory_order_release, std::memory_order_relaxed)) {
                    return;
                }
            }
        }

        /** @brief Runs one job found in any queue (own pop first, then random-victim steals).
         * @returns False if no work was available anywhere. */
        bool AssistOne(JobSystemState &state) {
            const i32 own = tQueueIndex;

            if (own >= 0 && static_cast<u32>(own) < state.QueueCount) {
                if (const auto handle = state.Queues[static_cast<std::size_t>(own)]->TryPop()) {
                    ExecutePackedJob(state, *handle);

                    return true;
                }
            }

            const u32 queueCount = state.QueueCount;
            const u32 start = NextStealRandom() % queueCount;

            for (u32 i = 0; i < queueCount; ++i) {
                const u32 victim = (start + i) % queueCount;

                if (own >= 0 && victim == static_cast<u32>(own)) {
                    continue;
                }

                if (const auto handle = state.Queues[victim]->TrySteal()) {
                    ExecutePackedJob(state, *handle);
                    return true;
                }
            }

            return false;
        }

        /** @brief True if a dispatched-but-unfinished job (`!Finished`, pending == 0) exists
         * *beyond the ones executing on this thread's own stack*: queued (this thread can assist)
         * or executing on another thread (it will free its slot when it completes). That is real
         * progress potential even when `AssistOne` fails, which only proves *this* thread found no
         * work. A created-but-unscheduled slot holds `Create`'s +1 (pending >= 1) and can never
         * run; if the pool is full and nothing is dispatched elsewhere, nothing can ever be
         * dispatched again — dispatch requires a predecessor to finish and nothing runnable
         * exists — so that observation, held for the whole stall window, is a permanent stall.
         *
         * The dispatched count is compared against `tExecutingDepth` rather than tested for
         * nonzero because every job on the calling thread's stack is itself dispatched-and-
         * unfinished: a builder job that exhausts the pool from inside its own body would
         * otherwise vouch for its own liveness forever and livelock the guard away.
         *
         * Known blind spots, accepted deliberately (a hang under a debugger beats aborting a
         * possibly-healthy program): a job deadlocked inside `Wait` stays dispatched forever and
         * silences the guard — including for *other* threads' guards, whose depth cannot account
         * for it; and at the edge-pool site a dispatched job with no successor edges keeps the
         * guard quiet without ever freeing an edge. */
        [[nodiscard]] bool AnyJobDispatchedElsewhere(const JobSystemState &state) {
            u32 dispatched = 0;

            for (u32 i = 0; i < state.MaxJobs; ++i) {
                const Job &job = state.Jobs[i];

                if (!job.Finished.load(std::memory_order_acquire) && job.PendingDependencies.load(std::memory_order_relaxed) == 0) {
                    if (++dispatched > tExecutingDepth) {
                        return true;
                    }
                }
            }

            return false;
        }

        /** @brief Pops a recycled edge or bumps a fresh one; under exhaustion, assists (runs jobs,
         * which frees edges) until one is available — always correct, loud in debug. */
        [[nodiscard]] u32 AllocateEdge(JobSystemState &state) {
            ExhaustionStallGuard stallGuard;

            for (;;) {
                u64 head = state.EdgeFreeListHead.load(std::memory_order_acquire);

                while (static_cast<u32>(head & LOW_32_BITS) != INVALID_JOB_EDGE) {
                    const auto edgeIndex = static_cast<u32>(head & LOW_32_BITS);
                    const u32 next = state.Edges[edgeIndex].Next.load(std::memory_order_relaxed);
                    const u64 newHead = (((head >> 32U) + 1U) << 32U) | static_cast<u64>(next);
                    if (state.EdgeFreeListHead.compare_exchange_weak(head, newHead, std::memory_order_acq_rel, std::memory_order_acquire)) {
                        return edgeIndex;
                    }
                }

                const u64 fresh = state.NextEdge.fetch_add(1, std::memory_order_relaxed);

                if (fresh < state.MaxEdges) {
                    return static_cast<u32>(fresh);
                }

                state.NextEdge.store(state.MaxEdges, std::memory_order_relaxed);

                // Edges only recycle when a *finished* predecessor walks its successor list, so —
                // exactly like the job pool — edges of never-scheduled predecessors are not
                // drainable. While any job is dispatched elsewhere the system may still recycle
                // edges, so only a pool exhausted with nothing dispatched beyond this thread's own
                // stack for the whole stall window aborts (see AnyJobDispatchedElsewhere for the
                // accepted blind spot at this site).
                VASSERT(false, "Job edge pool exhausted ({} edges) - raise JobSystemConfig::MaxEdges.", state.MaxEdges);

                const bool assisted = AssistOne(state);

                if (assisted || AnyJobDispatchedElsewhere(state)) {
                    stallGuard.OnProgress();
                } else {
                    stallGuard.OnNoProgress("dependency-edge pool (MaxEdges)");
                }

                if (!assisted) {
                    std::this_thread::yield();
                }
            }
        }

        /** @brief Bumps the eventcount and wakes one sleeping worker (no-op syscall when none sleep). */
        VE_INLINE void WakeOne(JobSystemState &state) {
            state.WorkSignal.fetch_add(1, std::memory_order_release);
            state.WorkSignal.notify_one();
        }

        /** @brief Hands a dependency-free job to the scheduler: pushed to the calling thread's own
         * queue in threaded mode, executed inline (topological, deterministic) in synchronous mode,
         * on foreign threads, or if the queue is full.
         *
         * The inline path recurses one frame per dependency-chain link (FinishJob -> OnJobReady ->
         * ExecutePackedJob), measured at ~48 bytes/frame at -O2 (~288 at -O0), so an 8 MiB stack
         * handles chains into the ~10^5 range — far beyond sane graph depth. Revisit with an
         * explicit work list if graphs ever get that deep. */
        void OnJobReady(JobSystemState &state, u64 packedHandle) {
            if (state.Workers.empty()) {
                ExecutePackedJob(state, packedHandle);
                return;
            }

            const i32 own = tQueueIndex;

            if (own >= 0 && static_cast<u32>(own) < state.QueueCount && state.Queues[static_cast<std::size_t>(own)]->TryPush(packedHandle)) {
                WakeOne(state);
                return;
            }

            ExecutePackedJob(state, packedHandle);
        }

        /** @brief Completion: closes the successor list, marks the job finished (freeing the slot
         * for recycling), then releases each successor — the decrement that reaches zero schedules
         * it. This is the entire scheduler; there is no central lock. */
        void FinishJob(JobSystemState &state, Job &job) {
            const u32 generation = job.Generation.load(std::memory_order_relaxed);
            const u64 closedHead = job.FirstEdge.exchange(PackEdgeHead(generation, JOB_EDGE_LIST_CLOSED), std::memory_order_acq_rel);
            job.Finished.store(true, std::memory_order_release);

            u32 edgeIndex = static_cast<u32>(closedHead & LOW_32_BITS);

            // A closed head here means this job was finished twice — only reachable through an
            // upstream contract violation (e.g. AddDependency after the successor was dispatched).
            // Without the loop guard the sentinel would be used as a pool index (wild OOB read).
            VASSERT(edgeIndex != JOB_EDGE_LIST_CLOSED, "FinishJob on an already-finished job - the job ran twice.");

            while (edgeIndex != INVALID_JOB_EDGE && edgeIndex != JOB_EDGE_LIST_CLOSED) {
                JobEdge &edge = state.Edges[edgeIndex];
                const u32 next = edge.Next.load(std::memory_order_relaxed);
                const u64 successor = edge.PackedSuccessor;
                FreeEdge(state, edgeIndex);

                Job &successorJob = state.Jobs[successor & state.JobIndexMask];

                // Under correct usage a successor can never be recycled while this edge holds a
                // pending count on it, so the generation always matches; a mismatch means a
                // contract violation upstream — skip rather than corrupt an unrelated
                // incarnation's scheduling state.
                if (successorJob.Generation.load(std::memory_order_relaxed) == static_cast<u32>(successor >> 32U)) {
                    if (successorJob.PendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                        OnJobReady(state, successor);
                    }
                } else {
                    VASSERT(false, "FinishJob: stale successor edge - AddDependency contract was violated upstream.");
                }

                edgeIndex = next;
            }
        }

        /** @brief Runs one dispatched job to completion — the single place a job body executes,
         * on whichever thread claimed it. Validates the packed handle against the slot, invokes
         * the payload under the memory tag captured at submission, destroys the capture, then
         * finishes the job, which frees the slot and releases its successors.
         * @param state The state object of the job system.
         * @param packedHandle The job to run, as produced by `PackJobHandle`.
         */
        void ExecutePackedJob(JobSystemState &state, u64 packedHandle) {
            Job &job = state.Jobs[packedHandle & state.JobIndexMask];

            // A legitimately dispatched job can never be stale (its slot cannot be recycled while
            // unfinished), so a mismatch is always an upstream contract violation (e.g. a double
            // dispatch whose slot was reused). Skip rather than run an unrelated job early.
            if (job.Generation.load(std::memory_order_relaxed) != static_cast<u32>(packedHandle >> 32U)) {
                VASSERT(false, "Executing a stale job handle - skipping (upstream scheduling contract violation).");
                return;
            }

            ++tExecutingDepth;

            {
                // The submitting thread's memory tag follows the work onto this thread, so worker-
                // side allocations attribute to the right subsystem instead of Untagged.
                MemoryScope memoryScope(job.Tag);
                VLKY_PROFILE_SCOPE("Job");
                job.Invoke(job.Payload);
            }

            if (nullptr != job.Destroy) {
                job.Destroy(job.Payload);
            }

            // Decrement BEFORE FinishJob. Once Finished is set the slot stops counting as
            // dispatched, so decrementing after would leave the depth over-counted relative to
            // the dispatched count and make the stall guard eager (risking a false abort). The
            // reverse window — still dispatched, already off the depth — errs toward patience.
            --tExecutingDepth;

            FinishJob(state, job);
        }

        /** @brief Scans every queue for work an idle worker could still pick up. Deliberately
         * approximate — it reads each deque's `LooksEmpty` heuristic — because the answer only
         * ever avoids a needless sleep: the eventcount, never this scan, is what makes a missed
         * wakeup impossible.
         * @param state The state object of the job system.
         * @returns True if any queue looked non-empty during the scan.
         */
        [[nodiscard]] bool HasPendingWork(JobSystemState &state) {
            for (u32 i = 0; i < state.QueueCount; ++i) {
                if (!state.Queues[i]->LooksEmpty()) {
                    return true;
                }
            }

            return false;
        }

        /** @brief Futex-sleeps on the eventcount. The value is sampled before the final queue scan,
         * and every push bumps it after publishing, so a wakeup can never be lost; a stop request
         * bumps it too via the callback, so shutdown can never hang here. */
        void IdleWait(JobSystemState &state, const std::stop_token &stopToken) {
            const u32 seen = state.WorkSignal.load(std::memory_order_acquire);

            if (HasPendingWork(state) || stopToken.stop_requested()) {
                return;
            }

            std::stop_callback wakeOnStop(stopToken, [&state] {
                state.WorkSignal.fetch_add(1, std::memory_order_release);
                state.WorkSignal.notify_all();
            });

            state.WorkSignal.wait(seen, std::memory_order_acquire);
        }

        /** @brief Pins the calling worker to one core so its deque and the data its jobs touch
         * stay in that core's caches. The index wraps at the machine's core count, so an
         * oversubscribed pool shares cores instead of failing. Failure is a warning, never fatal —
         * a restricted affinity mask (container, cgroup) is a normal deployment, not an error —
         * and platforms with no affinity API simply run unpinned.
         * @param core The core to pin to, taken modulo the core count.
         */
        void PinCurrentThreadToCore(u32 core) {
            const u32 coreCount = std::max(std::thread::hardware_concurrency(), 1U);

#if defined(VE_PLATFORM_LINUX)
            cpu_set_t cpuSet;
            CPU_ZERO(&cpuSet);
            CPU_SET(static_cast<i32>(core % coreCount), &cpuSet);
            if (pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuSet) != 0) {
                VWARN("JobSystem: failed to pin worker to core {}.", core % coreCount);
            }
#elif defined(VE_PLATFORM_WINDOWS)
            const u32 maskBits = static_cast<u32>(sizeof(DWORD_PTR) * 8U);
            const DWORD_PTR mask = DWORD_PTR{ 1 } << (core % std::min(coreCount, maskBits));
            if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0) {
                VWARN("JobSystem: failed to pin worker to core {}.", core % coreCount);
            }
#else
            (void)core;
            (void)coreCount;
#endif
        }

        /** @brief Gives the calling worker a debugger-visible name (`VlkyJob<index>`). Purely
         * diagnostic, so naming is best-effort and failures are ignored — unlike core pinning,
         * which warns — and a platform (or an OS build) without the API simply keeps the thread's
         * default name.
         * @param workerIndex The worker's queue index; becomes the name's suffix. */
        void NameCurrentThread(u32 workerIndex) {
#if defined(VE_PLATFORM_LINUX)

            // pthread caps the name at 16 bytes *including* the terminator: at most 15 characters
            // go into a value-initialized buffer, which keeps the NUL whatever the index is.
            std::array<char, 16> name{};
            std::format_to_n(name.data(), static_cast<std::ptrdiff_t>(name.size() - 1U), "VlkyJob{}", workerIndex);
            pthread_setname_np(pthread_self(), name.data());

#elif defined(VE_PLATFORM_WINDOWS) && defined(NTDDI_VERSION) && defined(NTDDI_WIN10_RS1) && (NTDDI_VERSION >= NTDDI_WIN10_RS1)

            // Windows imposes no length limit; the buffer mirrors the Linux branch so both stay
            // allocation-free. MAX_WORKER_THREADS is 128, so "VlkyJob128" fits with room to spare.
            std::array<wchar_t, 16> name{};
            std::format_to_n(name.data(), static_cast<std::ptrdiff_t>(name.size() - 1U), L"VlkyJob{}", workerIndex);
            SetThreadDescription(GetCurrentThread(), name.data());

#endif
        }

        /** @brief A pool worker's entire life. Claims its queue index, names and (optionally) pins
         * the thread, then loops: run whatever job it finds in any queue, and when a search comes
         * up empty, yield-spin for `SPIN_ROUNDS_BEFORE_SLEEP` rounds before futex-sleeping on the
         * eventcount — cheap to wake for bursty work, free to leave idle. Returns (ending the
         * thread) once `DestroyState` signals the stop token.
         * @param stopToken Cancellation signal delivered at shutdown.
         * @param workerIndex This worker's queue index (1..N; index 0 belongs to the main thread).
         */
        void WorkerMain(const std::stop_token &stopToken, u32 workerIndex) {
            JobSystemState &state = *gState.load(std::memory_order_acquire);

            tQueueIndex = static_cast<i32>(workerIndex);
            NameCurrentThread(workerIndex);

            if (state.Config.PinToCores) {
                PinCurrentThreadToCore(workerIndex);
            }

            u32 failedRounds = 0;

            while (!stopToken.stop_requested()) {
                if (AssistOne(state)) {
                    failedRounds = 0;
                    continue;
                }

                if (++failedRounds < SPIN_ROUNDS_BEFORE_SLEEP) {
                    std::this_thread::yield();
                    continue;
                }

                IdleWait(state, stopToken);
                failedRounds = 0;
            }
        }

        /** @brief Builds and publishes a fresh state, then spawns the workers. Caller must hold the
         * lifecycle mutex (or be the single bootstrap thread). */
        void CreateState(const JobSystemConfig &config, u32 workerCount, bool implicit) {
            // The pools are engine infrastructure: attribute them to Core regardless of the
            // caller's current scope.
            VE_MEMORY_SCOPE(MemoryTag::Core);

            auto *state = new JobSystemState();
            state->Config = config;
            state->Implicit = implicit;
            state->MaxJobs = std::bit_ceil(std::max(config.MaxJobs, 2U));
            state->JobIndexMask = static_cast<u64>(state->MaxJobs) - 1U;
            state->MaxEdges = std::max(config.MaxEdges, 1U);
            state->QueueCount = workerCount + 1U;

            // state->Jobs = Scope<Job[]>(new Job[state->MaxJobs]);
            // state->Edges = Scope<JobEdge[]>(new JobEdge[state->MaxEdges]);

            state->Jobs = CreateScope<Job[]>(state->MaxJobs);
            state->Edges = CreateScope<JobEdge[]>(state->MaxEdges);

            state->Queues.reserve(state->QueueCount);

            for (u32 i = 0; i < state->QueueCount; ++i) {
                // Each queue holds the whole pool, so TryPush can only fail on a logic error.
                state->Queues.push_back(CreateScope<JobQueue>(static_cast<std::size_t>(state->MaxJobs)));
            }

            // The creating thread owns queue 0 (it is "worker 0").
            tQueueIndex = 0;

            gState.store(state, std::memory_order_release);

            state->Workers.reserve(workerCount);

            for (u32 i = 1; i <= workerCount; ++i) {
                state->Workers.emplace_back([i](const std::stop_token &stopToken) { WorkerMain(stopToken, i); });
            }
        }

        /** @brief Stops and joins the workers, asserts the pool drained, and frees the state.
         * Caller must hold the lifecycle mutex. */
        void DestroyState() {
            JobSystemState *state = gState.load(std::memory_order_acquire);

            if (nullptr == state) {
                return;
            }

            for (std::jthread &worker : state->Workers) {
                worker.request_stop();
            }

            state->WorkSignal.fetch_add(1, std::memory_order_release);
            state->WorkSignal.notify_all();

            for (std::jthread &worker : state->Workers) {
                worker.join();
            }

            state->Workers.clear();

#if defined(VE_DEBUG)
            for (u32 i = 0; i < state->MaxJobs; ++i) {
                VASSERT(state->Jobs[i].Finished.load(std::memory_order_acquire),
                        "JobSystem::Shutdown with unfinished job in slot {} - a created job was never scheduled or never waited on.",
                        i);
            }
#endif

            gState.store(nullptr, std::memory_order_release);
            delete state;
            tQueueIndex = -1;
        }

        /** @brief Lazy bootstrap: the first pre-`Initialize` use creates a zero-worker (fully
         * synchronous) instance, so the job system degrades gracefully instead of crashing — and
         * never spawns threads without an explicit `Initialize`. */
        JobSystemState &EnsureState() {
            if (JobSystemState *state = gState.load(std::memory_order_acquire)) {
                return *state;
            }

            const std::lock_guard<std::mutex> lock(gLifecycleMutex);

            if (JobSystemState *state = gState.load(std::memory_order_acquire)) {
                return *state;
            }

            CreateState(JobSystemConfig{}, 0, true);

            return *gState.load(std::memory_order_acquire);
        }

        /** @brief Validates a handle against the pool.
         * @returns The live slot, or nullptr if the handle is invalid or stale. */
        [[nodiscard]] Job *ResolveJob(JobSystemState &state, const JobHandle &handle) {
            if (!handle.IsValid() || handle.Index >= state.MaxJobs) {
                return nullptr;
            }

            Job &job = state.Jobs[handle.Index];

            if (job.Generation.load(std::memory_order_acquire) != handle.Generation) {
                return nullptr;
            }

            return &job;
        }

    } // namespace

    StatusCode JobSystem::Initialize(const JobSystemConfig &config) {
        const std::lock_guard<std::mutex> lock(gLifecycleMutex);

        if (JobSystemState *state = gState.load(std::memory_order_acquire)) {
            if (!state->Implicit) {
                VWARN("JobSystem::Initialize called while already initialized - call Shutdown first.");
                return StatusCode::JobSystemAlreadyInitialized;
            }

            // An implicit synchronous instance is a bootstrap convenience; replace it silently.
            DestroyState();
        }

        u32 workerCount = config.WorkerCount;

        if (0 == workerCount) {
            workerCount = std::max(std::thread::hardware_concurrency(), 1U) - 1U;
        }

        workerCount = std::min(workerCount, MAX_WORKER_THREADS);

        CreateState(config, workerCount, false);

#if defined(VE_DEBUG)
        const JobSystemState &state = *gState.load(std::memory_order_acquire);
        VINFO("Job system initialized: {} worker thread(s) + main, {} job slots, {} dependency edges.", workerCount, state.MaxJobs, state.MaxEdges);
#endif

        return StatusCode::Successful;
    }

    void JobSystem::Shutdown() {
        const std::lock_guard<std::mutex> lock(gLifecycleMutex);
        DestroyState();

        VINFO("Job system shutdown successfully.");
    }

    bool JobSystem::IsInitialized() {
        return gState.load(std::memory_order_acquire) != nullptr;
    }

    u32 JobSystem::WorkerCount() {
        const JobSystemState *state = gState.load(std::memory_order_acquire);
        return nullptr != state ? state->QueueCount : 1U;
    }

    u32 JobSystem::CurrentWorkerIndex() {
        return tQueueIndex >= 0 ? static_cast<u32>(tQueueIndex) : INVALID_WORKER_INDEX;
    }

    JobHandle JobSystem::AcquireSlot() {
        JobSystemState &state = EnsureState();

        ExhaustionStallGuard stallGuard;
        for (;;) {
            for (u32 attempt = 0; attempt < state.MaxJobs; ++attempt) {
                const u64 cursor = state.NextJobSlot.fetch_add(1, std::memory_order_relaxed);
                const auto index = static_cast<std::size_t>(cursor & state.JobIndexMask);
                Job &job = state.Jobs[index];

                bool expected = true;

                if (!job.Finished.compare_exchange_strong(expected, false, std::memory_order_acquire, std::memory_order_relaxed)) {
                    continue;
                }

                const u32 generation = job.Generation.fetch_add(1, std::memory_order_relaxed) + 1U;
                job.PendingDependencies.store(1, std::memory_order_relaxed); // Held until Schedule.
                job.FirstEdge.store(PackEdgeHead(generation, INVALID_JOB_EDGE), std::memory_order_relaxed);
                job.Tag = CurrentMemoryTag();
                job.Invoke = nullptr;
                job.Destroy = nullptr;

                return JobHandle{ index, generation };
            }

            // Every slot is claimed and unfinished: the pool is undersized for the workload. Loud
            // in debug; in release, drain what is drainable and retry. Only *dispatched* work can
            // free slots — a created-but-unscheduled job can never be freed by assisting (nothing
            // may run it), so a builder that creates a whole graph before scheduling any of it
            // must size MaxJobs for the full graph. A pool fully in flight on other threads is
            // healthy (their jobs will finish), which is why the progress signal is the
            // system-wide AnyJobDispatchedElsewhere, never just this thread's failed assist; only
            // a pool with nothing dispatched beyond this thread's own blocked stack for the whole
            // stall window aborts.
            VASSERT(false, "Job pool exhausted ({} slots) - raise JobSystemConfig::MaxJobs.", state.MaxJobs);

            const bool assisted = AssistOne(state);

            if (assisted || AnyJobDispatchedElsewhere(state)) {
                stallGuard.OnProgress();
            } else {
                stallGuard.OnNoProgress("job pool (MaxJobs)");
            }

            if (!assisted) {
                std::this_thread::yield();
            }
        }
    }

    Job &JobSystem::ResolveClaimedJob(JobHandle handle) {
        return gState.load(std::memory_order_acquire)->Jobs[handle.Index];
    }

    void JobSystem::AddDependency(JobHandle job, JobHandle dependsOn) {
        JobSystemState *state = gState.load(std::memory_order_acquire);

        if (nullptr == state) {
            return;
        }

        Job *successor = ResolveJob(*state, job);
        VASSERT(nullptr != successor, "AddDependency: successor handle is stale or invalid.");

        if (nullptr == successor) {
            return;
        }

        // An unscheduled successor always holds Create's +1, so pending >= 1. Pending == 0 means
        // it was already dispatched — including the scheduled-but-still-running case a Finished
        // check would miss. Violations in release are contained (not fixed) by the generation
        // checks in FinishJob/ExecutePackedJob and the closed-sentinel guard in FinishJob.
        VASSERT(successor->PendingDependencies.load(std::memory_order_acquire) >= 1,
                "AddDependency must be called before the successor is scheduled (the successor was already dispatched).");

        Job *predecessor = ResolveJob(*state, dependsOn);

        if (predecessor == nullptr || predecessor->Finished.load(std::memory_order_acquire)) {
            return; // Already satisfied (finished or recycled): nothing to record.
        }

        successor->PendingDependencies.fetch_add(1, std::memory_order_relaxed);

        const u32 edgeIndex = AllocateEdge(*state);
        JobEdge &edge = state->Edges[edgeIndex];
        edge.PackedSuccessor = PackJobHandle(job);

        u64 head = predecessor->FirstEdge.load(std::memory_order_acquire);

        for (;;) {
            // The generation tag makes this push atomic with "is this still the incarnation the
            // handle refers to, and is its successor list still open".
            if (static_cast<u32>(head >> 32U) != dependsOn.Generation || static_cast<u32>(head & LOW_32_BITS) == JOB_EDGE_LIST_CLOSED) {
                // The predecessor finished (and possibly got recycled) while we prepared the edge:
                // the dependency is satisfied. Roll back our pending increment.
                FreeEdge(*state, edgeIndex);

                if (successor->PendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                    OnJobReady(*state, PackJobHandle(job));
                }

                return;
            }

            edge.Next.store(static_cast<u32>(head & LOW_32_BITS), std::memory_order_relaxed);
            const u64 newHead = PackEdgeHead(dependsOn.Generation, edgeIndex);

            if (predecessor->FirstEdge.compare_exchange_weak(head, newHead, std::memory_order_release, std::memory_order_acquire)) {
                return;
            }
        }
    }

    void JobSystem::Schedule(JobHandle job) {
        JobSystemState *state = gState.load(std::memory_order_acquire);

        if (nullptr == state || !job.IsValid()) {
            return;
        }

        Job *slot = ResolveJob(*state, job);
        VASSERT(nullptr != slot, "Schedule: job handle is stale or invalid.");

        if (nullptr == slot) {
            return;
        }

        // Release the +1 held since Create; whichever decrement reaches zero schedules the job.
        if (slot->PendingDependencies.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            OnJobReady(*state, PackJobHandle(job));
        }
    }

    void JobSystem::Wait(JobHandle job) {
        JobSystemState *state = gState.load(std::memory_order_acquire);

        if (nullptr == state) {
            return;
        }

        // Assist instead of blocking: keeps this thread useful and makes job-waits-on-job safe.
        while (!IsComplete(job)) {
            if (!AssistOne(*state)) {
                std::this_thread::yield();
            }
        }
    }

    bool JobSystem::IsComplete(JobHandle job) {
        JobSystemState *state = gState.load(std::memory_order_acquire);

        if (nullptr == state || !job.IsValid() || job.Index >= state->MaxJobs) {
            return true;
        }

        const Job &slot = state->Jobs[job.Index];

        // Read Finished before Generation: if the generation still matches afterwards, the flag
        // belonged to this incarnation; if it does not, the slot was recycled, which itself proves
        // this incarnation finished.
        const bool finished = slot.Finished.load(std::memory_order_acquire);
        const u32 generation = slot.Generation.load(std::memory_order_acquire);

        if (generation != job.Generation) {
            return true;
        }

        return finished;
    }

} // namespace Vulkyrie
