// Release-only regression tests for API-contract violations. In debug builds these paths trap in
// VASSERT (the correct, loud behavior), so the tests only compile in release, where the guarantee
// under test is *containment*: misuse must degrade to a bounded, harmless outcome — never a wild
// out-of-bounds pool access (the FinishJob closed-sentinel regression) and never a livelock.
#if !defined(VE_DEBUG)

#include "core/jobs/job_system.h"

#include "jobs_test_support.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace Vulkyrie;

TEST_CASE("Release: AddDependency after the successor already ran is contained", "[jobs]") {
    std::atomic<u32> runs{ 0 };

    const JobHandle late = JobSystem::Run([&runs] { runs.fetch_add(1, std::memory_order_relaxed); });
    JobSystem::Wait(late);
    REQUIRE(runs.load(std::memory_order_relaxed) == 1);

    const JobHandle slow = JobSystem::Create([] {});
    JobSystem::AddDependency(late, slow); // Deliberate misuse: `late` was already dispatched.
    JobSystem::Schedule(slow);
    JobSystem::Wait(slow);

    // Wait(slow) returns when slow's Finished flag is set, which happens before the successor
    // cascade runs, so give the (re-)dispatch of `late` a moment to land.
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (runs.load(std::memory_order_relaxed) < 2 && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }

    // The contained outcome is a bounded double-execution (or a skipped stale dispatch) — the
    // regression this guards against was FinishJob indexing the edge pool with the closed
    // sentinel and crashing.
    const u32 total = runs.load(std::memory_order_relaxed);
    REQUIRE(total >= 1);
    REQUIRE(total <= 2);
}

TEST_CASE("Release: a full pool of scheduled jobs drains via assist instead of livelocking", "[jobs]") {
    // Rebuild the job system around a deliberately tiny pool.
    JobSystem::Shutdown();
    {
        JobSystemConfig config{};
        config.WorkerCount = 2;
        config.PinToCores = false;
        config.MaxJobs = 64;
        config.MaxEdges = 64;
        JobSystem::Initialize(config);
    }

    // Far more scheduled jobs than slots: AcquireSlot's release recovery path must assist-drain
    // the backlog (every held slot is schedulable work) and keep making progress.
    constexpr u32 kJobs = 2000;
    std::atomic<u32> executed{ 0 };

    std::vector<JobHandle> handles;
    handles.reserve(kJobs);
    for (u32 i = 0; i < kJobs; ++i) {
        handles.push_back(JobSystem::Run([&executed] { executed.fetch_add(1, std::memory_order_relaxed); }));
    }
    for (const JobHandle &handle : handles) {
        JobSystem::Wait(handle);
    }

    REQUIRE(executed.load(std::memory_order_relaxed) == kJobs);

    // Restore the listener's configuration for the tests that follow.
    JobSystem::Shutdown();
    JobSystemConfig config{};
    config.PinToCores = false;
    JobSystem::Initialize(config);
}

TEST_CASE("Release: a pool fully in flight on workers is not misdiagnosed as a permanent stall", "[jobs]") {
    // Regression for the stall guard's progress signal: AssistOne() failing only proves *this*
    // thread found no work. With every slot legitimately executing on workers (nothing stealable,
    // nothing unscheduled), Create() must patiently wait for a slot to free — the old guard
    // aborted after its 5-second window with a misdiagnosis. The long jobs must outlast that
    // window for this test to prove anything, hence the ~6.5s runtime.
    JobSystem::Shutdown();
    {
        JobSystemConfig config{};
        config.WorkerCount = 4;
        config.PinToCores = false;
        config.MaxJobs = 4;
        config.MaxEdges = 64;
        JobSystem::Initialize(config);
    }

    static constexpr auto kJobDuration = std::chrono::milliseconds(6500);

    std::atomic<u32> started{ 0 };
    std::atomic<u32> done{ 0 };
    std::vector<JobHandle> longJobs;
    longJobs.reserve(4);
    for (u32 i = 0; i < 4; ++i) {
        longJobs.push_back(JobSystem::Run([&started, &done] {
            started.fetch_add(1, std::memory_order_relaxed);
            std::this_thread::sleep_for(kJobDuration);
            done.fetch_add(1, std::memory_order_relaxed);
        }));
    }

    // Wait until every slot is executing on a worker, so the pool is full and all queues empty.
    while (started.load(std::memory_order_relaxed) < 4) {
        std::this_thread::yield();
    }

    const auto submitStart = std::chrono::steady_clock::now();
    const JobHandle extra = JobSystem::Run([&done] { done.fetch_add(1, std::memory_order_relaxed); });
    const auto blocked = std::chrono::steady_clock::now() - submitStart;

    // Reaching this line at all is the regression check (the old guard aborted the process at
    // ~5s). The elapsed check proves we actually sat inside AcquireSlot across the abort window.
    REQUIRE(blocked >= std::chrono::seconds(5));

    JobSystem::Wait(extra);
    for (const JobHandle &handle : longJobs) {
        JobSystem::Wait(handle);
    }
    REQUIRE(done.load(std::memory_order_relaxed) == 5);

    // Restore the listener's configuration for the tests that follow.
    JobSystem::Shutdown();
    JobSystemConfig config{};
    config.PinToCores = false;
    JobSystem::Initialize(config);
}

TEST_CASE("Release: an exhausted edge pool drains via assist instead of livelocking", "[jobs]") {
    // The edge-pool twin of the test above. AllocateEdge has its own exhaustion-recovery loop, and
    // edges recycle on a different schedule from slots — only when a *finished* predecessor walks
    // its successor list — so the job-pool test says nothing about it. Here the slots are plentiful
    // and only MaxEdges is scarce, which pins the recovery to the edge path.
    const Tests::JobSystemConfigRestorer restorer{};

    JobSystem::Shutdown();
    {
        JobSystemConfig config{};
        config.WorkerCount = 2;
        config.PinToCores = false;
        config.MaxJobs = 512; // Comfortably more than the 200 slots this test holds at once.
        config.MaxEdges = 8;  // Far fewer than the 100 dependencies it declares.
        JobSystem::Initialize(config);
    }

    constexpr u32 kPairs = 100;
    static constexpr auto kHeadDuration = std::chrono::milliseconds(1);

    std::atomic<u32> executed{ 0 };
    std::vector<JobHandle> tails;
    tails.reserve(kPairs);

    for (u32 i = 0; i < kPairs; ++i) {
        // The heads are deliberately slow. Edges come back only when a head *finishes*, so without
        // this the two workers would recycle the eight edges faster than the loop consumes them and
        // the recovery path under test would never be entered at all.
        const JobHandle head = JobSystem::Create([&executed] {
            std::this_thread::sleep_for(kHeadDuration);
            executed.fetch_add(1, std::memory_order_relaxed);
        });
        const JobHandle tail = JobSystem::Create([&executed] { executed.fetch_add(1, std::memory_order_relaxed); });

        // Each pair burns an edge held until its head runs: from the ninth pair on, AddDependency
        // finds the pool empty and has to assist-drain the backlog to get one.
        JobSystem::AddDependency(tail, head);
        JobSystem::Schedule(head);
        JobSystem::Schedule(tail);

        tails.push_back(tail);
    }

    for (const JobHandle &handle : tails) {
        JobSystem::Wait(handle);
    }

    REQUIRE(executed.load(std::memory_order_relaxed) == 2U * kPairs);
}

// Manual-only (hidden tag, excluded from default and "[jobs]" runs): the genuine-stall direction
// of the guard cannot run in the suite because its correct outcome is process death. Run it
// explicitly with:
//     build/clang-all-release/tests/tests "[jobs-stall-abort]"
// EXPECTED OUTCOME: a fatal log stating the pool was exhausted with nothing dispatched, then
// abort (exit code 134), roughly 5 seconds in.
TEST_CASE("Manual: a pool held entirely by created-but-unscheduled jobs aborts loudly", "[.][jobs-stall-abort]") {
    // A filtered run never touches the logger tests, so bring the console sink up ourselves —
    // the whole point of running this manually is seeing the fatal diagnosis before the abort.
    (void)Logger::InitializeLogger(LoggerType::Console);

    JobSystem::Shutdown();
    JobSystemConfig config{};
    config.WorkerCount = 2;
    config.PinToCores = false;
    config.MaxJobs = 4;
    config.MaxEdges = 64;
    JobSystem::Initialize(config);

    std::vector<JobHandle> held;
    held.reserve(4);
    for (u32 i = 0; i < 4; ++i) {
        held.push_back(JobSystem::Create([] {}));
    }

    (void)JobSystem::Create([] {}); // Genuine permanent stall: must VFATAL + abort, not livelock.
    FAIL("unreachable - the Create above must abort the process");
}

// Third manual variant: the same permanent stall created from *inside a job* — the submission
// context the class docs recommend. The builder's own slot is dispatched-and-unfinished, and a
// guard that merely tests "is anything dispatched?" counts the blocked builder as system liveness
// and livelocks; the depth-aware predicate must still abort. Run explicitly with:
//     build/clang-all-release/tests/tests "[jobs-stall-abort-in-job]"
// EXPECTED OUTCOME: the fatal diagnosis, then abort (exit code 134), roughly 5 seconds in.
TEST_CASE("Manual: a builder job that exhausts the pool from inside its own body aborts loudly", "[.][jobs-stall-abort-in-job]") {
    (void)Logger::InitializeLogger(LoggerType::Console);

    JobSystem::Shutdown();
    JobSystemConfig config{};
    config.WorkerCount = 2;
    config.PinToCores = false;
    config.MaxJobs = 4;
    config.MaxEdges = 64;
    JobSystem::Initialize(config);

    const JobHandle builder = JobSystem::Run([] {
        std::vector<JobHandle> held;
        held.reserve(3);
        for (u32 i = 0; i < 3; ++i) {
            held.push_back(JobSystem::Create([] {})); // Builder's own slot + 3 unscheduled = pool full.
        }
        (void)JobSystem::Create([] {}); // Must VFATAL + abort at ~5s, not silence its own guard.
    });
    JobSystem::Wait(builder);
    FAIL("unreachable - the builder job must abort the process");
}

#endif // !defined(VE_DEBUG)
