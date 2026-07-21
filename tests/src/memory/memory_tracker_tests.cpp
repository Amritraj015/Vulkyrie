// Referencing MemoryTracker here is also what pulls `global_new_delete.o` (via the anchor in
// memory_scope.h, reached through memory_tracker.h -> vlkypch.h) into the Catch2 `tests` binary,
// which gets its own `main` and would otherwise never link the override. If the anchor failed, the
// `new`/`delete` round-trips below would not move the counters and these tests would fail loudly.
#include "memory/memory_tracker.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

using namespace Vulkyrie;

namespace {

    // Escapes an allocation so the compiler cannot elide the new/delete pair (C++ N3664). Atomic so
    // concurrent stores from the multi-threaded test are not a data race.
    std::atomic<void *> gMemorySink{ nullptr };

    // Launders a size through volatile storage so it is not a compile-time constant the optimizer
    // could reason the allocation away from.
    [[nodiscard]] std::size_t RuntimeSize(std::size_t value) {
        volatile std::size_t v = value;
        return v;
    }

    constexpr MemoryTag kThreadTag = MemoryTag::Networking;
    constexpr std::size_t kThreadCount = 8;
    constexpr std::size_t kIterations = 1000;

} // namespace

TEST_CASE("MemoryTracker attributes a scoped allocation to the correct subsystem", "[memory]") {
    const i64 currentBefore = MemoryTracker::CurrentBytes(MemoryTag::Physics);
    const i64 liveBefore = MemoryTracker::LiveAllocations(MemoryTag::Physics);
    const i64 totalBefore = MemoryTracker::TotalAllocated(MemoryTag::Physics);

    const std::size_t size = RuntimeSize(4096);

    char *buffer = nullptr;
    {
        VE_MEMORY_SCOPE(MemoryTag::Physics);
        buffer = new char[size];
    }
    gMemorySink.store(buffer, std::memory_order_relaxed); // escape to defeat allocation elision
    buffer[0] = char{ 1 };
    buffer[size - 1] = char{ 2 };

    // While the allocation is live, the Physics bucket reflects it — proof the override is active.
    REQUIRE(MemoryTracker::CurrentBytes(MemoryTag::Physics) - currentBefore >= static_cast<i64>(size));
    REQUIRE(MemoryTracker::LiveAllocations(MemoryTag::Physics) == liveBefore + 1);
    REQUIRE(MemoryTracker::TotalAllocated(MemoryTag::Physics) - totalBefore >= static_cast<i64>(size));

    // The accounting identity holds at all times.
    REQUIRE(MemoryTracker::TotalAllocated(MemoryTag::Physics) - MemoryTracker::TotalFreed(MemoryTag::Physics) ==
            MemoryTracker::CurrentBytes(MemoryTag::Physics));

    delete[] buffer;

    // After the free, current bytes and live count return to baseline.
    REQUIRE(MemoryTracker::CurrentBytes(MemoryTag::Physics) == currentBefore);
    REQUIRE(MemoryTracker::LiveAllocations(MemoryTag::Physics) == liveBefore);
}

TEST_CASE("MemoryScope pushes and pops the current tag; nested scopes attribute to the innermost", "[memory]") {
    REQUIRE(CurrentMemoryTag() == MemoryTag::Untagged);

    {
        VE_MEMORY_SCOPE(MemoryTag::Physics);
        REQUIRE(CurrentMemoryTag() == MemoryTag::Physics);

        {
            VE_MEMORY_SCOPE(MemoryTag::Rendering);
            REQUIRE(CurrentMemoryTag() == MemoryTag::Rendering);

            const i64 physicsBefore = MemoryTracker::CurrentBytes(MemoryTag::Physics);
            const i64 renderingBefore = MemoryTracker::CurrentBytes(MemoryTag::Rendering);

            const std::size_t size = RuntimeSize(2048);
            char *pointer = new char[size];
            gMemorySink.store(pointer, std::memory_order_relaxed);
            pointer[0] = char{ 7 };

            // The allocation lands in the innermost (Rendering) bucket, not the outer (Physics) one.
            REQUIRE(MemoryTracker::CurrentBytes(MemoryTag::Rendering) - renderingBefore >= static_cast<i64>(size));
            REQUIRE(MemoryTracker::CurrentBytes(MemoryTag::Physics) == physicsBefore);

            delete[] pointer;
        }

        // Popping the inner scope restores the outer tag.
        REQUIRE(CurrentMemoryTag() == MemoryTag::Physics);
    }

    REQUIRE(CurrentMemoryTag() == MemoryTag::Untagged);
}

TEST_CASE("MemoryTracker peak bytes is a monotonic high-water mark", "[memory]") {
    VE_MEMORY_SCOPE(MemoryTag::Materials);

    const i64 peakBefore = MemoryTracker::PeakBytes(MemoryTag::Materials);

    const std::size_t big = RuntimeSize(64 * 1024);
    char *large = new char[big];
    gMemorySink.store(large, std::memory_order_relaxed);
    large[0] = char{ 1 };

    const i64 peakAfterBig = MemoryTracker::PeakBytes(MemoryTag::Materials);
    REQUIRE(peakAfterBig >= peakBefore + static_cast<i64>(big));

    delete[] large;

    // Freeing must not lower the recorded peak.
    REQUIRE(MemoryTracker::PeakBytes(MemoryTag::Materials) == peakAfterBig);

    // A subsequent smaller allocation must not lower it either.
    const std::size_t small = RuntimeSize(1024);
    char *tiny = new char[small];
    gMemorySink.store(tiny, std::memory_order_relaxed);
    tiny[0] = char{ 1 };
    REQUIRE(MemoryTracker::PeakBytes(MemoryTag::Materials) == peakAfterBig);
    delete[] tiny;
}

TEST_CASE("MemoryTracker reconciles under concurrent allocation with no negative counters", "[memory]") {
    const i64 currentBefore = MemoryTracker::CurrentBytes(kThreadTag);
    const i64 liveBefore = MemoryTracker::LiveAllocations(kThreadTag);

    std::vector<std::thread> threads;
    threads.reserve(kThreadCount);

    for (std::size_t t = 0; t < kThreadCount; ++t) {
        threads.emplace_back([]() {
            for (std::size_t i = 0; i < kIterations; ++i) {
                VE_MEMORY_SCOPE(kThreadTag);
                const std::size_t size = RuntimeSize(128);
                char *pointer = new char[size];
                gMemorySink.store(pointer, std::memory_order_relaxed);
                pointer[0] = char{ 1 };
                delete[] pointer;
            }
        });
    }

    for (std::thread &thread : threads) {
        thread.join();
    }

    // Everything allocated was freed → counters return to baseline, stay non-negative, identity holds.
    REQUIRE(MemoryTracker::CurrentBytes(kThreadTag) == currentBefore);
    REQUIRE(MemoryTracker::LiveAllocations(kThreadTag) == liveBefore);
    REQUIRE(MemoryTracker::CurrentBytes(kThreadTag) >= 0);
    REQUIRE(MemoryTracker::LiveAllocations(kThreadTag) >= 0);
    REQUIRE(MemoryTracker::TotalAllocated(kThreadTag) - MemoryTracker::TotalFreed(kThreadTag) == MemoryTracker::CurrentBytes(kThreadTag));
}
