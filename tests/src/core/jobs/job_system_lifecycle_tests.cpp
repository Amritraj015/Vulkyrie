// The lifecycle and degradation paths every other job test takes for granted, because the run
// listener hands them an explicit multi-worker instance that is already up: the Initialize/Shutdown
// state machine, the zero-worker synchronous fallback reached by using the job system before
// Initialize, and the foreign-thread contract. All of it is documented public API with no coverage
// otherwise — and the synchronous path is what ships whenever a tool or test binary never calls
// Initialize at all.
#include "core/jobs/job_system.h"
#include "core/status_codes.h"

#include "jobs_test_support.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace Vulkyrie;

TEST_CASE("Initialize on an already-initialized system is refused and leaves it running", "[jobs]") {
    REQUIRE(JobSystem::IsInitialized());
    const u32 workersBefore = JobSystem::WorkerCount();

    JobSystemConfig config{};
    config.WorkerCount = 2;
    config.PinToCores = false;
    REQUIRE(JobSystem::Initialize(config) == StatusCode::JobSystemAlreadyInitialized);

    // Refused means untouched: the running instance keeps its workers and keeps executing work.
    REQUIRE(JobSystem::WorkerCount() == workersBefore);

    std::atomic<bool> ran{ false };
    const JobHandle handle = JobSystem::Run([&ran] { ran.store(true, std::memory_order_relaxed); });
    JobSystem::Wait(handle);
    REQUIRE(ran.load(std::memory_order_relaxed));
}

TEST_CASE("Shutdown returns the job system to the uninitialized state", "[jobs]") {
    const Tests::JobSystemConfigRestorer restorer{};

    JobSystem::Shutdown();

    REQUIRE_FALSE(JobSystem::IsInitialized());
    REQUIRE(JobSystem::WorkerCount() == 1);                           // The calling thread alone.
    REQUIRE(JobSystem::CurrentWorkerIndex() == INVALID_WORKER_INDEX); // Shutdown released this thread's queue.

    // Queries and waits stay safe while the system is down instead of dereferencing a null state.
    REQUIRE(JobSystem::IsComplete(JobHandle{}));
    JobSystem::Wait(JobHandle{});

    JobSystem::Shutdown(); // Idempotent.
    REQUIRE_FALSE(JobSystem::IsInitialized());
}

TEST_CASE("A first use before Initialize bootstraps a synchronous zero-worker instance", "[jobs]") {
    const Tests::JobSystemConfigRestorer restorer{};

    JobSystem::Shutdown();
    REQUIRE_FALSE(JobSystem::IsInitialized());

    // Plain, non-atomic flags on purpose: with no workers every job runs on this thread, and this
    // test should stop compiling as "obviously correct" the day that changes.
    bool ran = false;
    const JobHandle handle = JobSystem::Create([&ran] { ran = true; });

    REQUIRE(JobSystem::IsInitialized()); // Create bootstrapped the implicit instance.
    REQUIRE(JobSystem::WorkerCount() == 1);
    REQUIRE(JobSystem::CurrentWorkerIndex() == 0); // The bootstrapping thread owns queue 0.
    REQUIRE_FALSE(ran);                            // Created, not scheduled: nothing runs yet.
    REQUIRE_FALSE(JobSystem::IsComplete(handle));

    JobSystem::Schedule(handle);

    REQUIRE(ran); // Synchronous: the job ran before Schedule returned.
    REQUIRE(JobSystem::IsComplete(handle));
}

TEST_CASE("Initialize transparently replaces a lazily bootstrapped instance", "[jobs]") {
    const Tests::JobSystemConfigRestorer restorer{};

    JobSystem::Shutdown();
    JobSystem::Wait(JobSystem::Run([] {})); // Bootstraps the implicit synchronous instance.
    REQUIRE(JobSystem::IsInitialized());
    REQUIRE(JobSystem::WorkerCount() == 1);

    JobSystemConfig config{};
    config.WorkerCount = 3;
    config.PinToCores = false;

    // An implicit instance is a bootstrap convenience, so this is Successful — not the
    // JobSystemAlreadyInitialized an explicit instance would have produced.
    REQUIRE(JobSystem::Initialize(config) == StatusCode::Successful);
    REQUIRE(JobSystem::WorkerCount() == 4); // 3 workers + the main thread.
    REQUIRE(JobSystem::CurrentWorkerIndex() == 0);

    // The replacement pool is the one actually executing work: every job lands on a queue that
    // belongs to it.
    constexpr u32 kJobs = 256;

    std::atomic<u32> outOfRange{ 0 };
    std::vector<JobHandle> handles;
    handles.reserve(kJobs);

    for (u32 i = 0; i < kJobs; ++i) {
        handles.push_back(JobSystem::Run([&outOfRange] {
            if (JobSystem::CurrentWorkerIndex() >= JobSystem::WorkerCount()) {
                outOfRange.fetch_add(1, std::memory_order_relaxed);
            }
        }));
    }

    for (const JobHandle &handle : handles) {
        JobSystem::Wait(handle);
    }

    REQUIRE(outOfRange.load(std::memory_order_relaxed) == 0);
}

TEST_CASE("A foreign thread has no worker index and runs the work it schedules inline", "[jobs]") {
    REQUIRE(JobSystem::IsInitialized());

    std::atomic<u32> foreignIndex{ 0 };
    std::atomic<u32> indexInsideJob{ 0 };
    std::atomic<bool> ranBeforeScheduleReturned{ false };
    std::atomic<std::thread::id> executingThread{};

    std::thread foreign([&foreignIndex, &indexInsideJob, &ranBeforeScheduleReturned, &executingThread] {
        foreignIndex.store(JobSystem::CurrentWorkerIndex(), std::memory_order_relaxed);

        bool ran = false;
        const JobHandle handle = JobSystem::Create([&ran, &indexInsideJob, &executingThread] {
            ran = true;
            indexInsideJob.store(JobSystem::CurrentWorkerIndex(), std::memory_order_relaxed);
            executingThread.store(std::this_thread::get_id(), std::memory_order_relaxed);
        });

        JobSystem::Schedule(handle);

        // The documented contract: a foreign thread owns no queue, so the job it schedules runs
        // synchronously on that thread before Schedule returns (a producer thread serializes
        // itself). `ran` is a plain bool for exactly that reason.
        ranBeforeScheduleReturned.store(ran, std::memory_order_relaxed);

        JobSystem::Wait(handle);
    });

    const std::thread::id foreignId = foreign.get_id();
    foreign.join();

    REQUIRE(foreignIndex.load(std::memory_order_relaxed) == INVALID_WORKER_INDEX);
    REQUIRE(ranBeforeScheduleReturned.load(std::memory_order_relaxed));
    REQUIRE(executingThread.load(std::memory_order_relaxed) == foreignId);
    REQUIRE(indexInsideJob.load(std::memory_order_relaxed) == INVALID_WORKER_INDEX);
}

TEST_CASE("A long dependency chain unwinds in order on the synchronous inline path", "[jobs]") {
    const Tests::JobSystemConfigRestorer restorer{};

    // With no workers, each completed link recurses into the next (FinishJob -> OnJobReady ->
    // ExecutePackedJob) instead of queuing it, so the whole chain unwinds on one stack. The depth
    // is deliberately modest: this guards the recursion against regressing into something
    // frame-heavy, it is not a probe for the ~10^5 limit the header documents.
    constexpr u32 kDepth = 1000;

    JobSystem::Shutdown(); // The Create below bootstraps the synchronous instance.

    std::vector<u32> executionOrder;
    executionOrder.reserve(kDepth);

    std::vector<JobHandle> chain;
    chain.reserve(kDepth);

    for (u32 i = 0; i < kDepth; ++i) {
        chain.push_back(JobSystem::Create([&executionOrder, i] { executionOrder.push_back(i); }));

        if (i > 0) {
            JobSystem::AddDependency(chain[i], chain[i - 1]);
        }
    }

    // Tail first, so nothing can run until the head is released and the entire chain cascades
    // inside that single Schedule call.
    for (u32 i = kDepth; i-- > 1;) {
        JobSystem::Schedule(chain[i]);
    }

    REQUIRE(executionOrder.empty());

    JobSystem::Schedule(chain[0]);
    JobSystem::Wait(chain[kDepth - 1]);

    REQUIRE(executionOrder.size() == kDepth);

    u32 outOfOrder = 0;
    for (u32 i = 0; i < kDepth; ++i) {
        if (executionOrder[i] != i) {
            ++outOfOrder;
        }
    }
    REQUIRE(outOfOrder == 0);
}
