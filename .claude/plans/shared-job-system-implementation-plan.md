# Shared Job System (`core/jobs/`) — Implementation Plan

## Context

Per the [roadmap](roadmap.md), the critical path is **Memory P0 → shared job system → (physics ‖ renderer)**.
Memory Phase 0 is now **implemented and verified** (`engine/{include,src}/memory/`: `MemoryTag`,
cheap-tier `MemoryTracker`, header-prefix global `operator new`/`delete` with the linker anchor,
`VE_MEMORY_SCOPE` in the PCH, shutdown report, `tests/src/memory/memory_tracker_tests.cpp`). The
next keystone is the job system.

It is **the** shared foundation: the [physics plan](physics-performance-parallelism-architecture.md)
defines it as Phase 2 and blocks Phases 3–5 (parallel broadphase/narrowphase, island + graph-colored
solve, SIMD) on it; the [renderer plan](vulkan-renderer-architecture.md) blocks Phase 4 (parallel
command-buffer recording) on it. Both plans explicitly call it *engine-wide infra, not physics-specific*.

Confirmed by grep across the repo: **there is no threading anywhere in `engine/`** — the only
`<thread>`/`<atomic>`/`<mutex>` users are `debug/profiler.h` (thread id + a mutex) and
`memory/memory_tracker.h` (atomic counters). This is greenfield.

The outcome: a lock-free, work-stealing job system with a task graph, a **machine-independent
deterministic** `ParallelFor`, and correct memory attribution across worker threads — built once,
correctly, and independently testable before either big track leans on it.

---

## Current state (what the codebase gives us)

- **Auto-globbed sources.** `engine/CMakeLists.txt` uses `file(GLOB_RECURSE src/*.cpp CONFIGURE_DEPENDS)`
  → new `engine/src/core/jobs/*.cpp` compiles with no source-list edit. Same for `tests/src/*.cpp`.
- **Private headers live in `src/`.** `engine/src/core/console_log_sink.h` is included as
  `"core/console_log_sink.h"` (engine's `PRIVATE` include dir is `src`). The work-stealing deque is an
  implementation detail and follows this precedent — it does **not** belong in `include/`.
- **Generational handles are an established pattern.** `renderer/renderer_context.h`
  `Handle<T>{ size_t Index; u32 Generation; }`, and `core/entity.h` packs index+generation into a `u64`.
  `JobHandle` mirrors `Handle<T>`.
- **Static-facade lifecycle precedent.** `MemorySystem::Initialize()/Shutdown()` in
  `engine/src/main.cpp`, sitting beside `Logger::InitializeLogger` and `VLKY_PROFILE_BEGIN_SESSION`.
  `JobSystem` slots in next to it.
- **`Profiler::WriteProfile` is already mutex-guarded and records `std::thread::id`** — wrapping job
  execution in `VLKY_PROFILE_SCOPE` gives per-worker rows in chrome://tracing for free.
- **`MemoryScope`'s stack is `thread_local`** (`memory/memory_scope.h:34`). This is the critical seam:
  see §4.
- **`Threads` is not linked.** `Dependencies.cmake` has no `find_package(Threads)` and no target links
  `Threads::Threads`. `std::thread`/`std::jthread` on Linux needs it — **this is a required CMake edit**,
  not an optional one. (It may currently work transitively via glfw; do not rely on that.)
- **`Logger` has no synchronization at all** — no mutex in `logger.h`, `logger.cpp`,
  `console_log_sink.cpp`, or `file_log_sink.cpp`. See §6.
- **`-Wall -Wextra -Wconversion -Wsign-conversion -Wpedantic -Werror`** on clang/gcc. Index math in the
  deque and the partitioner must funnel every `size_t`/`i64`/`u32` conversion through explicit
  `static_cast` — this is where this module will most likely fail the build.
- **`.clang-tidy`** enables `concurrency-*` and `cppcoreguidelines-*`, and sets
  `ConstexprVariableCase: UPPER_CASE` (as in `core/constants.h`'s `VE_MACHINE_EPSILON`). Use
  UPPER_CASE for `constexpr` constants here.

## Locked decisions (confirmed with the user)

1. **Full scope** — pool + `ParallelFor` + `JobHandle` **and** the task graph with dependency edges.
2. **Static facade + explicit lifecycle** — `JobSystem::Initialize(config)` / `Shutdown()` in
   `main.cpp`, mirroring `MemorySystem`.
3. **Affinity pinning included now** — `pthread_setaffinity_np` / `SetThreadAffinityMask` behind
   `VE_PLATFORM_LINUX` / `VE_PLATFORM_WINDOWS`.
4. **Fix the logger** — add a mutex to the sinks so worker-thread logging is safe.

---

## Design

### 1. `core/jobs/job.h` (new, public) — handle + job storage

```cpp
inline constexpr std::size_t JOB_PAYLOAD_CAPACITY = 64;   // bytes of inline callable storage
inline constexpr std::size_t INVALID_JOB_INDEX    = ~std::size_t{ 0 };

/** @brief Generational handle to a job slot. Mirrors renderer_context.h's Handle<T>. */
struct JobHandle {
    std::size_t Index      = INVALID_JOB_INDEX;
    u32         Generation = 0;
};
```

- **Type-erased, allocation-free callable.** `Job` holds
  `void (*Invoke)(void*)`, `void (*Destroy)(void*)`, and
  `alignas(std::max_align_t) std::byte Payload[JOB_PAYLOAD_CAPACITY]`. `Create()` `static_assert`s the
  callable fits and is nothrow-move-constructible → a too-large capture is a **compile error** telling
  the caller to shrink it, never a silent heap allocation. Submitting must not allocate: it would
  recurse through the tracked global `operator new` and add cost to the hot path.
- **Job state:** `std::atomic<u32> PendingDependencies`, `std::atomic<u32> FirstEdge`,
  `std::atomic<bool> Finished`, `std::atomic<u32> Generation`, and `MemoryTag Tag`.

### 2. `engine/src/core/jobs/job_queue.h` (new, private) — Chase-Lev deque

One per thread (workers **and** the main thread). Fixed power-of-two capacity, mask indexing.

- Owner pushes/pops at `_bottom` (`std::atomic<i64>`); thieves CAS `_top`. Standard Chase-Lev memory
  ordering: `release` on push, a `seq_cst` fence in pop, `acquire` in steal. Stores `JobHandle`, not
  pointers, so a stale steal is caught by generation validation.
- No dynamic growth — overflow `VASSERT`s (debug) and falls back to running the job inline on the
  pushing thread (release), which is always correct, just slower.

### 3. `core/jobs/job_system.h` + `engine/src/core/jobs/job_system.cpp` (new) — the facade

```cpp
struct JobSystemConfig {
    u32  WorkerCount = 0;      ///< 0 => hardware_concurrency() - 1 (main thread is worker 0).
    bool PinToCores  = true;   ///< Pin worker i to core i for cache locality.
    u32  MaxJobs     = 8192;   ///< Job pool size (power of two). Must exceed peak in-flight jobs.
    u32  MaxEdges    = 8192;   ///< Dependency-edge pool size.
};

class JobSystem final {
public:
    JobSystem() = delete;

    static StatusCode Initialize(const JobSystemConfig &config = {});
    static void       Shutdown();
    [[nodiscard]] static bool IsInitialized();

    [[nodiscard]] static u32 WorkerCount();         ///< Threads participating, including the main thread.
    [[nodiscard]] static u32 CurrentWorkerIndex();  ///< 0 == main thread; 1..N == workers.

    static JobHandle Create(TFunc &&fn);                                  ///< Created, not scheduled.
    static void      AddDependency(JobHandle job, JobHandle dependsOn);   ///< Before Schedule(job).
    static void      Schedule(JobHandle job);                             ///< Enqueue if deps are met.
    static JobHandle Run(TFunc &&fn);                                     ///< Create + Schedule.
    static void      Wait(JobHandle job);                                 ///< Assists while waiting.
    [[nodiscard]] static bool IsComplete(JobHandle job);
};
```

- **Job pool.** `MaxJobs` slots allocated once in `Initialize`, claimed by an atomic bump counter with
  wraparound; the slot's `Generation` increments on claim. A claim over a slot that is not `Finished`
  is a `VASSERT` (pool undersized) — loud and debuggable rather than silent corruption.
- **Dependency edges.** Successors are a lock-free singly-linked list threaded through a preallocated
  `JobEdge{ JobHandle Successor; u32 NextEdge; }` pool, head CAS-pushed into the job's `FirstEdge`.
  No fan-out cap beyond the pool, and no allocation. `AddDependency` increments the successor's
  `PendingDependencies`.
- **Completion.** On finish: set `Finished`, then walk the successor list and
  `fetch_sub(1)` each `PendingDependencies`; any that reaches 0 is pushed to the finishing thread's
  queue. This is the whole scheduler — no central lock.
- **Main-thread assist.** `Wait(handle)` runs jobs from the caller's own queue, then steals, until the
  handle completes. This keeps the main thread useful **and** makes job-waits-on-job non-deadlocking.
- **Idle policy.** Workers spin a bounded number of failed steal attempts, then sleep on a
  `std::condition_variable`; a push wakes one. A game engine must not burn 100% CPU when idle.
- **Affinity pinning** (`PinToCores`): `pthread_setaffinity_np` under `VE_PLATFORM_LINUX`,
  `SetThreadAffinityMask` under `VE_PLATFORM_WINDOWS` (both macros come from `vlkypch.h`). Failure is a
  `VWARN`, never fatal.
- **Graceful degradation.** `WorkerCount == 0`, or any call made **before** `Initialize`, executes jobs
  **inline on the calling thread**. Dependencies still resolve in topological order. This gives a
  fully synchronous debug path for free and means no code accidentally spawns threads.
- **Shutdown** sets a stop flag, wakes all workers, joins, and asserts the pool is drained.

### 4. Memory-tag propagation — the Phase 0 seam (do not skip)

`MemoryScope`'s stack is `thread_local` (`memory/memory_scope.h:34`), so a worker thread starts with an
**empty** stack. Left alone, the moment physics goes parallel every allocation inside a job lands in
`MemoryTag::Untagged` and the Phase 0 attribution silently regresses to useless.

Therefore: `Create()` captures `CurrentMemoryTag()` from the **submitting** thread into `Job::Tag`, and
the worker brackets execution in `MemoryScope scope(job.Tag);`. A `VE_MEMORY_SCOPE(MemoryTag::Physics)`
on the main thread then correctly follows the work onto every worker. This is covered by a dedicated
test (§Verification) so the seam cannot rot.

### 5. `core/jobs/parallel_for.h` (new) — deterministic partitioning

```cpp
inline constexpr u32 MAX_PARALLEL_FOR_CHUNKS = 1024;

[[nodiscard]] constexpr u32 ChunkCountFor(u32 count, u32 grainSize);          ///< Pure: the contract.
[[nodiscard]] constexpr void ChunkRange(u32 count, u32 chunkCount, u32 chunkIndex, u32 &begin, u32 &end);

template <typename TFunc> void ParallelForRange(u32 count, u32 grainSize, TFunc &&fn); ///< fn(chunk, begin, end)
template <typename TFunc> void ParallelFor(u32 count, u32 grainSize, TFunc &&fn);      ///< fn(index)
```

Partition rule (documented and unit-tested as a pure function):

```
chunkCount = clamp(ceilDiv(count, grainSize), 1, MAX_PARALLEL_FOR_CHUNKS)
base = count / chunkCount;  remainder = count % chunkCount
chunk c covers [ c*base + min(c, remainder), (c+1)*base + min(c+1, remainder) )
```

**Deliberate strengthening of the physics plan.** That plan specifies partitioning fixed "given a fixed
thread count". Deriving `chunkCount` from `(count, grainSize)` and a **fixed constant** instead of from
hardware makes the partition identical on every machine, so a determinism hash matches across a 4-core
and a 32-core box — not just across runs on one box. Core count then affects only *scheduling*, never
results. This costs nothing and removes a whole class of "deterministic on my machine" bugs.

**Deterministic merges are chunk-keyed, not worker-keyed** — for the same reason:

```cpp
template <typename T> class ChunkedOutput {   ///< Per-chunk buffers, merged in ascending chunk order.
public:
    explicit ChunkedOutput(u32 chunkCount);
    std::vector<T> &Chunk(u32 chunkIndex);
    void MergeInto(std::vector<T> &out) const;
};
```

This is the primitive the physics plan's parallel broadphase pair lists and narrowphase contact arenas
consume. `CurrentWorkerIndex()` remains available for genuinely per-thread *scratch* (reused buffers,
per-worker pools) where the data never reaches a result.

### 6. Logger thread-safety (`engine/src/core/{console,file}_log_sink.{h,cpp}`)

Add a `std::mutex _mutex;` member to `ConsoleLogSink` and `FileLogSink` and take a `std::lock_guard` in
`LogMessage` (and in `FileLogSink`'s `Initialize`/destructor around `_logFile`). The formatting buffer is
already a per-call stack `std::array`, so the lock is about making the write **and** the `fflush`
atomic as a pair and guarding `_logFile`'s lifetime — not about the buffer. Small, contained, and it
makes `VWARN`/`VERROR` from worker threads (e.g. affinity failures) safe.

### 7. Build (`Dependencies.cmake`, `engine/CMakeLists.txt`)

- `Dependencies.cmake`: add `find_package(Threads REQUIRED)`.
- `engine/CMakeLists.txt`: add `Threads::Threads` to the `PUBLIC` `target_link_libraries` block.
- **No source-list edits** — `GLOB_RECURSE CONFIGURE_DEPENDS` picks up `engine/src/core/jobs/*.cpp` and
  `tests/src/core/jobs/*.cpp` automatically.

### 8. Bootstrap (`engine/src/main.cpp`)

`JobSystem::Initialize()` immediately after `MemorySystem::Initialize()`; `JobSystem::Shutdown()`
immediately **before** `MemorySystem::Shutdown()` — workers must be joined before the memory report is
emitted or the counters are read mid-flight. Optionally mirror in `runtime/src/main.cpp` and
`vulky-cli/src/main.cpp`.

For the Catch2 binary (which gets `main` from Catch2, not `engine/src/main.cpp`), a
`CATCH_REGISTER_LISTENER` in `tests/src/core/jobs/jobs_test_listener.cpp` initializes on
`testRunStarting` and shuts down on `testRunEnded`.

---

## File checklist

**New**
- `engine/include/core/jobs/job.h` — `JobHandle`, inline-storage `Job`, constants
- `engine/include/core/jobs/job_system.h` — `JobSystemConfig` + the static facade
- `engine/include/core/jobs/parallel_for.h` — `ParallelFor`/`ParallelForRange`/`ChunkedOutput`
- `engine/src/core/jobs/job_queue.h` — Chase-Lev deque (private, `src/` per the sink precedent)
- `engine/src/core/jobs/job_system.cpp` — pools, workers, scheduling, affinity, shutdown
- `tests/src/core/jobs/job_system_tests.cpp`, `job_graph_tests.cpp`, `parallel_for_tests.cpp`,
  `job_memory_tag_tests.cpp`, `jobs_test_listener.cpp`

**Modify**
- `Dependencies.cmake` — `find_package(Threads REQUIRED)`
- `engine/CMakeLists.txt` — link `Threads::Threads`
- `engine/src/main.cpp` — `JobSystem::Initialize()` / `Shutdown()`
- `engine/src/core/console_log_sink.{h,cpp}`, `engine/src/core/file_log_sink.{h,cpp}` — sink mutex

---

## Verification

1. **Build** — `/build clang-all-debug`, then `/build clang-all-release`. `-Wconversion`/
   `-Wsign-conversion`/`-Werror` over the deque's `i64` index math and the partitioner's `u32` math is
   the expected failure point; fix with explicit `static_cast`, not by relaxing flags.
2. **Unit tests** — `/test clang-all-debug "[jobs]"`:
   - *Execution:* N submitted jobs each run exactly once; `Wait` blocks until complete;
     `IsComplete` agrees; a job that submits and waits on another job does **not** deadlock
     (proves main-thread/worker assist).
   - *Graph:* chain `A→B→C` observes that order; diamond `A→{B,C}→D` runs `D` last and each node once;
     a job with unmet dependencies never runs early.
   - *Partitioning (pure, no threads):* chunks tile `[0, count)` exactly once with no gap or overlap —
     assert a per-index counter array is all `1`; `count == 0` is a no-op; `grainSize > count` yields
     one chunk; `ChunkCountFor` is independent of `WorkerCount`.
   - *Determinism:* run the same `ParallelForRange` under `WorkerCount` = 0, 1, 2, and
     `hardware_concurrency()-1`, merge via `ChunkedOutput`, and assert **byte-identical** output. This
     is the gate the physics determinism test will later depend on.
   - *Memory seam:* allocate inside a job submitted under `VE_MEMORY_SCOPE(MemoryTag::Physics)` and
     assert `MemoryTracker::CurrentBytes(MemoryTag::Physics)` moves, not `Untagged` — mirrors the
     existing style in `tests/src/memory/memory_tracker_tests.cpp`.
   - *Stress:* thousands of small jobs with heavy steal pressure; assert an atomic completion counter
     equals the submitted count and no handle is executed twice.
3. **ThreadSanitizer** — a one-off `-fsanitize=thread` build of the `tests` target run over `[jobs]`.
   A hand-written Chase-Lev deque is exactly the code where TSan earns its keep, and Phase 0 already
   shipped the `VE_MEMORY_DISABLE_GLOBAL_NEW` escape hatch so the sanitizer and the global `operator new`
   override do not fight. **Treat this as required, not optional.**
4. **End-to-end** — run `build/clang-all-debug/examples/sandbox/sandbox`; confirm it still runs clean,
   the shutdown memory report is unchanged, and (with `VLKY_PROFILE` enabled) chrome://tracing shows one
   row per worker thread.
5. **Format/tidy** — `/format-check`. Expect `concurrency-*` and `cppcoreguidelines-*` to have opinions
   about the atomics; consult `.clang-tidy` before suppressing anything.

## Out of scope (later, do not build now)

Consuming the job system in physics (parallel broadphase/narrowphase = physics P3, island + graph-colored
solve = P4) · the renderer's render thread and parallel command recording (renderer P4) · fibers or
coroutine integration · job priorities/affinity classes (e.g. pinning I/O jobs off the sim workers) ·
GPU job offload · arena-backed job payloads (waits on memory Phase 2's allocator toolkit).
