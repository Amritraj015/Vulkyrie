// The generational-handle contract. A JobHandle is a value that outlives the job it names, so every
// entry point has to cope with handles that name nothing, point outside the pool, or are stale
// because their slot was recycled into a later incarnation. The rest of the suite only ever passes
// live handles, so the "detected instead of aliasing a newer job" guarantee the generation exists
// for is what these tests pin down.
#include "core/jobs/job_system.h"

#include "jobs_test_support.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <limits>

using namespace Vulkyrie;

// The handle vocabulary is pure, so it is provable at compile time.
static_assert(!JobHandle{}.IsValid(), "A default-constructed handle names no job.");
static_assert(JobHandle{ 0, 0 }.IsValid(), "Slot 0 is a real slot; only the sentinel index is invalid.");
static_assert(JobHandle{ 4, 1 } == JobHandle{ 4, 1 });
static_assert(!(JobHandle{ 4, 1 } == JobHandle{ 4, 2 }), "The generation is part of a handle's identity, not decoration.");
static_assert(PackEdgeHead(u32{ 3 }, u32{ 9 }) == ((u64{ 3 } << 32U) | u64{ 9 }));
static_assert(PackEdgeHead(u32{ 0 }, JOB_EDGE_LIST_CLOSED) != PackEdgeHead(u32{ 1 }, JOB_EDGE_LIST_CLOSED),
              "The packed head tags the generation so an edge push cannot link into the wrong incarnation.");

TEST_CASE("Handles that name no job are inert on every entry point", "[jobs]") {
    constexpr JobHandle kInvalid{};
    REQUIRE_FALSE(kInvalid.IsValid());

    REQUIRE(JobSystem::IsComplete(kInvalid)); // "Nothing to wait for" reads as finished.
    JobSystem::Wait(kInvalid);                // Returns immediately instead of spinning forever.
    JobSystem::Schedule(kInvalid);            // Dropped before any slot is touched.

    // An in-range-looking but out-of-pool index is caught by the bounds check rather than indexing
    // the pool. Only the read-only entry points are exercised: Schedule/AddDependency treat a
    // handle this broken as an upstream bug and trap on it in debug builds, by design.
    constexpr JobHandle kOutOfRange{ std::numeric_limits<std::size_t>::max() - 1U, 1 };
    REQUIRE(kOutOfRange.IsValid());
    REQUIRE(JobSystem::IsComplete(kOutOfRange));
    JobSystem::Wait(kOutOfRange);

    // As a prerequisite, a handle that names nothing counts as already satisfied — the successor
    // must still run rather than wait forever on a dependency that can never be met.
    std::atomic<bool> ran{ false };
    const JobHandle successor = JobSystem::Create([&ran] { ran.store(true, std::memory_order_relaxed); });
    JobSystem::AddDependency(successor, kInvalid);
    JobSystem::Schedule(successor);
    JobSystem::Wait(successor);

    REQUIRE(ran.load(std::memory_order_relaxed));
}

TEST_CASE("A handle whose slot was recycled is stale, never an alias for the new incarnation", "[jobs]") {
    const Tests::JobSystemConfigRestorer restorer{};

    // Two slots, so the pool wraps every couple of jobs and the recycling is deterministic rather
    // than a matter of running 8192 jobs and hoping.
    JobSystem::Shutdown();
    {
        JobSystemConfig config{};
        config.WorkerCount = 1;
        config.PinToCores = false;
        config.MaxJobs = 2;
        config.MaxEdges = 8;
        JobSystem::Initialize(config);
    }

    const JobHandle recycled = JobSystem::Run([] {});
    JobSystem::Wait(recycled);

    // Cycle the pool several times over; that slot now holds a much later generation. Each job is
    // waited on before the next is created, so the two slots are always free and the claim cursor
    // walks them in lockstep.
    constexpr u32 kCycles = 8;

    std::atomic<u32> executed{ 0 };
    for (u32 i = 0; i < kCycles; ++i) {
        const JobHandle handle = JobSystem::Run([&executed] { executed.fetch_add(1, std::memory_order_relaxed); });
        JobSystem::Wait(handle);
    }
    REQUIRE(executed.load(std::memory_order_relaxed) == kCycles);

    // The stale handle reads as finished — its incarnation did finish — and waiting on it returns
    // instead of tracking whatever now lives in that slot.
    REQUIRE(JobSystem::IsComplete(recycled));
    JobSystem::Wait(recycled);

    // The same for a stale prerequisite: satisfied immediately, and no edge is recorded against the
    // slot's current occupant.
    std::atomic<bool> ran{ false };
    const JobHandle successor = JobSystem::Create([&ran] { ran.store(true, std::memory_order_relaxed); });
    JobSystem::AddDependency(successor, recycled);
    JobSystem::Schedule(successor);
    JobSystem::Wait(successor);

    REQUIRE(ran.load(std::memory_order_relaxed));
}
