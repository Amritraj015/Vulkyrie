// Single-threaded semantics of the private Chase-Lev deque (hence the engine/src include path in
// tests/CMakeLists.txt). The contention stress in job_queue_stress_tests.cpp proves nothing is lost
// or duplicated, but it deliberately ignores *which* end a value came out of — so the properties
// the scheduler actually leans on are unchecked: the owner pops newest-first (cache-hot), thieves
// take oldest-first (the long tail of work), a push is rejected only at full capacity (the cue to
// run a job inline), and the deque stays usable after every one of those edges.
#include "core/jobs/job_queue.h"

#include <catch2/catch_test_macros.hpp>

#include <optional>

using namespace Vulkyrie;

TEST_CASE("An empty JobQueue yields nothing and stays usable afterwards", "[jobs]") {
    JobQueue queue(4);

    REQUIRE(queue.LooksEmpty());
    REQUIRE_FALSE(queue.TryPop().has_value());
    REQUIRE_FALSE(queue.TrySteal().has_value());

    // The failed pop speculatively decremented `bottom` and must have restored it; a queue that
    // did not would mis-index (or claim to hold work) from here on.
    REQUIRE(queue.TryPush(u64{ 7 }));
    REQUIRE_FALSE(queue.LooksEmpty());

    const std::optional<u64> popped = queue.TryPop();
    REQUIRE(popped.has_value());
    REQUIRE(*popped == 7);
    REQUIRE(queue.LooksEmpty());
}

TEST_CASE("The owner pops in LIFO order while thieves steal in FIFO order", "[jobs]") {
    JobQueue queue(8);

    for (u64 i = 0; i < 4; ++i) {
        REQUIRE(queue.TryPush(i));
    }

    // Owner end: newest first.
    const std::optional<u64> newest = queue.TryPop();
    REQUIRE(newest.has_value());
    REQUIRE(*newest == 3);

    // Steal end: oldest first.
    const std::optional<u64> oldest = queue.TrySteal();
    REQUIRE(oldest.has_value());
    REQUIRE(*oldest == 0);

    const std::optional<u64> secondOldest = queue.TrySteal();
    REQUIRE(secondOldest.has_value());
    REQUIRE(*secondOldest == 1);

    // One element left, so the owner takes it through the CAS path it shares with thieves —
    // uncontended here, it must win.
    const std::optional<u64> last = queue.TryPop();
    REQUIRE(last.has_value());
    REQUIRE(*last == 2);

    REQUIRE(queue.LooksEmpty());
    REQUIRE_FALSE(queue.TrySteal().has_value());
}

TEST_CASE("JobQueue rejects a push only once it holds its full capacity", "[jobs]") {
    constexpr std::size_t kCapacity = 8;
    JobQueue queue(kCapacity);

    for (u64 i = 0; i < kCapacity; ++i) {
        REQUIRE(queue.TryPush(i)); // The whole capacity is usable, not capacity - 1.
    }

    REQUIRE_FALSE(queue.TryPush(u64{ 999 })); // Full: the scheduler's cue to run the job inline.

    // Freeing one slot makes room for exactly one more push.
    REQUIRE(queue.TrySteal().has_value());
    REQUIRE(queue.TryPush(u64{ 999 }));
    REQUIRE_FALSE(queue.TryPush(u64{ 1000 }));
}

TEST_CASE("JobQueue indices wrap past its capacity without losing or aliasing values", "[jobs]") {
    constexpr std::size_t kCapacity = 4;
    constexpr u32 kRounds = 100; // 200 pushes through 4 slots: the mask-indexed buffer wraps 50 times.

    JobQueue queue(kCapacity);

    u64 value = 0;
    for (u32 round = 0; round < kRounds; ++round) {
        const u64 first = value++;
        const u64 second = value++;

        REQUIRE(queue.TryPush(first));
        REQUIRE(queue.TryPush(second));

        const std::optional<u64> stolen = queue.TrySteal();
        REQUIRE(stolen.has_value());
        REQUIRE(*stolen == first);

        const std::optional<u64> popped = queue.TryPop();
        REQUIRE(popped.has_value());
        REQUIRE(*popped == second);

        REQUIRE(queue.LooksEmpty());
    }
}
