#include <catch2/catch_test_macros.hpp>
#include <memory/allocators/stack_allocator.h>
#include <memory/memory_tracker.h>

#include <vector>

using namespace Vulkyrie;

namespace {

    constexpr auto TAG = MemoryTag::Events;

} // namespace

TEST_CASE("StackAllocator - Allocations are aligned and distinct", "[memory][allocator][stack]") {
    StackAllocator stack{ 4096, TAG };

    void *first = stack.Allocate(1, 1);
    void *second = stack.Allocate(8, 8);
    void *third = stack.Allocate(4, 64);

    REQUIRE(first != nullptr);
    REQUIRE(first != second);
    REQUIRE(second != third);
    REQUIRE(reinterpret_cast<uintptr_t>(second) % 8 == 0);
    REQUIRE(reinterpret_cast<uintptr_t>(third) % 64 == 0);
    REQUIRE(stack.Used() == 13);
}

TEST_CASE("StackAllocator - Typed Allocate and Emplace", "[memory][allocator][stack]") {
    StackAllocator stack{ 4096, TAG };

    const StackMarker marker = stack.GetMarker();

    u32 *array = stack.Allocate<u32>(8);
    REQUIRE(array != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(array) % alignof(u32) == 0);

    i64 *value = stack.Emplace<i64>(-42);
    REQUIRE(*value == -42);

    // Both went through the same bump path, so a rewind reclaims them together.
    stack.FreeToMarker(marker);
    REQUIRE(stack.Used() == 0);
}

TEST_CASE("StackAllocator - FreeToMarker unwinds to a saved position", "[memory][allocator][stack]") {
    StackAllocator stack{ 4096, TAG };

    void *kept = stack.Allocate(128, 16);
    const StackMarker marker = stack.GetMarker();
    const size_t usedAtMarker = stack.Used();

    void *scratch = stack.Allocate(256, 16);
    REQUIRE(scratch != nullptr);
    REQUIRE(stack.Used() > usedAtMarker);

    stack.FreeToMarker(marker);

    REQUIRE(stack.Used() == usedAtMarker);

    // The rewound space is handed out again; what was allocated before the marker is untouched.
    REQUIRE(stack.Allocate(256, 16) == scratch);
    REQUIRE(kept != scratch);
}

TEST_CASE("StackAllocator - Markers nest", "[memory][allocator][stack]") {
    StackAllocator stack{ 4096, TAG };

    (void)stack.Allocate(64, 8);
    const StackMarker outer = stack.GetMarker();

    (void)stack.Allocate(64, 8);
    const StackMarker inner = stack.GetMarker();

    (void)stack.Allocate(64, 8);
    REQUIRE(stack.Used() == 192);

    stack.FreeToMarker(inner);
    REQUIRE(stack.Used() == 128);

    stack.FreeToMarker(outer);
    REQUIRE(stack.Used() == 64);
}

TEST_CASE("StackAllocator - Scope guard rewinds on exit", "[memory][allocator][stack]") {
    StackAllocator stack{ 4096, TAG };

    (void)stack.Allocate(32, 8);
    const size_t baseline = stack.Used();

    {
        const StackAllocatorScope scope{ stack };
        (void)stack.Allocate(512, 32);
        REQUIRE(stack.Used() > baseline);
    }

    // The guard exists so an early return cannot leave the stack grown.
    REQUIRE(stack.Used() == baseline);
}

TEST_CASE("StackAllocator - Growth keeps markers valid across chunks", "[memory][allocator][stack]") {
    StackAllocator stack{ 64, TAG };

    const StackMarker marker = stack.GetMarker();
    std::vector<u64 *> pointers;

    // Comfortably past the initial chunk, so the stack has to append storage.
    for (u64 i = 0; i < 4096; ++i) {
        auto *value = static_cast<u64 *>(stack.Allocate(sizeof(u64), alignof(u64)));
        *value = i;
        pointers.push_back(value);
    }

    REQUIRE(stack.ChunkCount() > 1);

    // Markers name a chunk and an offset, not a raw pointer, so growth does not invalidate them.
    for (u64 i = 0; i < pointers.size(); ++i) {
        REQUIRE(*pointers[i] == i);
    }

    stack.FreeToMarker(marker);

    REQUIRE(stack.Used() == 0);
    REQUIRE(stack.ChunkCount() > 1); // Chunks are retained for the next round.
    REQUIRE(stack.HighWaterMark() >= 4096 * sizeof(u64));
}

TEST_CASE("StackAllocator - Reset releases everything and keeps capacity", "[memory][allocator][stack]") {
    StackAllocator stack{ 1024, TAG };

    void *first = stack.Allocate(128, 16);
    (void)stack.Allocate(128, 16);

    const size_t capacityBefore = stack.Capacity();
    stack.Reset();

    REQUIRE(stack.Used() == 0);
    REQUIRE(stack.Capacity() == capacityBefore);
    REQUIRE(stack.Allocate(128, 16) == first);
}

TEST_CASE("StackAllocator - Reservation and usage are reported to the tracker", "[memory][allocator][stack]") {
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(TAG);
    const i64 usedBefore = MemoryTracker::PoolUsedBytes(TAG);

    {
        StackAllocator stack{ 8192, TAG };

        REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore + 8192);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);

        const StackMarker marker = stack.GetMarker();
        (void)stack.Allocate(1024, 16);

        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore + 1024);

        stack.FreeToMarker(marker);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
    }

    REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore);
    REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
}
