#pragma once

#include "vlkypch.h"

#include "core/jobs/job.h"
#include "core/status_codes.h"

namespace Vulkyrie {

    /** @brief Sentinel returned by `JobSystem::CurrentWorkerIndex` on threads the job system does not
     * own (anything other than the main thread and the worker pool). */
    inline constexpr u32 INVALID_WORKER_INDEX = ~u32{ 0 };

    /** @brief Configuration for `JobSystem::Initialize`. */
    struct JobSystemConfig {
    public:
        /** @brief Number of worker threads to spawn. 0 => `hardware_concurrency() - 1` (the main
         * thread participates as worker 0, so total parallelism matches the core count). */
        u32 WorkerCount = 0;

        /** @brief Pin worker i to core i for cache locality. Failure is a warning, never fatal. */
        bool PinToCores = true;

        /** @brief Job pool size; rounded up to a power of two. Must exceed peak in-flight jobs.
         * Slots held by created-but-unscheduled jobs cannot be reclaimed by the scheduler, so a
         * builder that creates a whole graph before scheduling any of it must size for the full
         * graph; exhausting the pool with nothing schedulable is a fatal error, not a wait. */
        u32 MaxJobs = 8192;

        /** @brief Dependency-edge pool size. Bounds the total fan-out of unfinished jobs; edges of
         * never-scheduled predecessors are likewise unreclaimable until those jobs run. */
        u32 MaxEdges = 8192;
    };

    /** @brief Engine-wide lock-free, work-stealing job system with dependency-graph scheduling.
     *
     * A static facade with an explicit lifecycle, mirroring `MemorySystem`: `Initialize` in `main`
     * right after the memory subsystem, `Shutdown` right before its report. Each participating
     * thread (main + workers) owns a Chase-Lev deque; ready jobs are pushed to the scheduling
     * thread's own queue and idle workers steal from random victims. Waiting threads assist (run
     * jobs) instead of blocking, so a job may safely create and wait on other jobs.
     *
     * Graceful degradation: with zero workers — or on any call made before `Initialize` — jobs
     * execute inline on the calling thread the moment their dependencies are met, in topological
     * order. That path is fully synchronous and deterministic, and no code ever spawns threads
     * without an explicit `Initialize`.
     *
     * Foreign threads: a thread the job system does not own (anything other than the main thread
     * and the workers) has no work queue, so a job it schedules — or that becomes ready during a
     * completion cascade on it — executes inline on that thread, synchronously, before the
     * scheduling call returns. A dedicated producer thread (e.g. asset streaming) that submits
     * work therefore serializes itself; submit from the main thread or from inside jobs to get
     * parallelism.
     */
    class JobSystem final {
    public:
        JobSystem() = delete;

        /** @brief Allocates the job/edge pools and spawns the worker threads.
         * @param config Pool sizes, worker count, and affinity policy.
         * @returns `StatusCode::Successful`, or `JobSystemAlreadyInitialized` if an explicit
         * instance is already running (an implicit synchronous instance is replaced transparently).
         */
        static StatusCode Initialize(const JobSystemConfig &config = {});

        /** @brief Stops and joins all workers, asserts the pool is drained, and frees the pools. */
        static void Shutdown();

        /** @brief Returns true if the job system is initialized (explicitly or implicitly). */
        [[nodiscard]] static bool IsInitialized();

        /** @brief Returns the number of threads participating in job execution, including the main
         * thread (i.e. spawned workers + 1). Useful for sizing per-thread scratch space. */
        [[nodiscard]] static u32 WorkerCount();

        /** @brief Returns the calling thread's worker index: 0 for the main thread, 1..N for pool
         * workers, `INVALID_WORKER_INDEX` for threads the job system does not own. */
        [[nodiscard]] static u32 CurrentWorkerIndex();

        /** @brief Claims a job slot and stores the callable — created, not yet scheduled. Captures
         * the calling thread's current memory tag so worker-side allocations attribute correctly.
         * Never heap-allocates; a too-large capture is a compile error. The callable must not let
         * an exception escape: on a worker that terminates the process, and on the inline path it
         * unwinds through the scheduler and leaks the slot.
         * @tparam TFunc The callable type; invoked as `fn()`.
         * @param fn The work to run.
         * @returns A generational handle for `AddDependency`/`Schedule`/`Wait`.
         */
        template <typename TFunc> [[nodiscard]] static JobHandle Create(TFunc &&fn) {
            const JobHandle handle = AcquireSlot();
            ResolveClaimedJob(handle).Emplace(std::forward<TFunc>(fn));
            return handle;
        }

        /** @brief Makes `job` wait for `dependsOn`. Must be called after creating `job` and before
         * scheduling it; `dependsOn` may be in any state (already-finished dependencies are
         * satisfied immediately).
         * @param job The successor job (created, not yet scheduled).
         * @param dependsOn The prerequisite job.
         */
        static void AddDependency(JobHandle job, JobHandle dependsOn);

        /** @brief Releases a created job for execution; it runs once all dependencies are met.
         * @param job The job to schedule.
         */
        static void Schedule(JobHandle job);

        /** @brief Creates and immediately schedules a job with no dependencies.
         * @tparam TFunc The callable type; invoked as `fn()`.
         * @param fn The work to run.
         * @returns The scheduled job's handle.
         */
        template <typename TFunc> static JobHandle Run(TFunc &&fn) {
            const JobHandle handle = Create(std::forward<TFunc>(fn));
            Schedule(handle);
            return handle;
        }

        /** @brief Blocks until `job` completes, executing other jobs while waiting (so the calling
         * thread stays useful and job-waits-on-job never deadlocks).
         * @param job The job to wait for. Stale/invalid handles return immediately.
         */
        static void Wait(JobHandle job);

        /** @brief Returns true if `job` has finished (stale/invalid handles count as finished).
         * @param job The job to query.
         */
        [[nodiscard]] static bool IsComplete(JobHandle job);

    private:
        /** @brief Claims and re-initializes a free pool slot (lazily bootstrapping a synchronous,
         * zero-worker instance if `Initialize` has not run). */
        [[nodiscard]] static JobHandle AcquireSlot();

        /** @brief Resolves a just-claimed handle to its slot for payload emplacement. */
        [[nodiscard]] static Job &ResolveClaimedJob(JobHandle handle);
    };

} // namespace Vulkyrie
