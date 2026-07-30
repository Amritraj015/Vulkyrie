#include <catch2/catch_test_macros.hpp>
#include <memory/allocators/free_list_allocator.h>
#include <memory/memory_tracker.h>

#include <cstring>
#include <set>
#include <vector>

using namespace Vulkyrie;

namespace {

    constexpr auto TAG = MemoryTag::Assets;

} // namespace

TEST_CASE("FreeListAllocator - Allocations are distinct, aligned and non-overlapping", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 64 * 1024, TAG };

    auto *first = static_cast<std::byte *>(allocator.Allocate(100, 8));
    auto *second = static_cast<std::byte *>(allocator.Allocate(200, 32));
    auto *third = static_cast<std::byte *>(allocator.Allocate(50, 64));

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);

    REQUIRE(reinterpret_cast<uintptr_t>(second) % 32 == 0);
    REQUIRE(reinterpret_cast<uintptr_t>(third) % 64 == 0);

    // Writing each region fully must not disturb the others, which is the real test that the blocks are disjoint.
    std::memset(first, 0xAA, 100);
    std::memset(second, 0xBB, 200);
    std::memset(third, 0xCC, 50);

    REQUIRE(static_cast<u8>(first[99]) == 0xAA);
    REQUIRE(static_cast<u8>(second[199]) == 0xBB);
    REQUIRE(static_cast<u8>(third[49]) == 0xCC);

    REQUIRE(allocator.AllocationCount() == 3);
}

TEST_CASE("FreeListAllocator - Typed Allocate and Emplace propagate exhaustion", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 4096, TAG };

    u64 *array = allocator.Allocate<u64>(8);
    REQUIRE(array != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(array) % alignof(u64) == 0);

    i32 *value = allocator.Emplace<i32>(99);
    REQUIRE(value != nullptr);
    REQUIRE(*value == 99);

    allocator.Free(array);
    allocator.Free(value);

    // This allocator is the one that can genuinely fail, so the typed forms must hand back nullptr rather than
    // constructing into it.
    FreeListAllocator tiny{ 64, TAG };

    REQUIRE(tiny.Allocate<u64>(4096) == nullptr);
    REQUIRE(tiny.Emplace<std::array<u64, 512>>() == nullptr);
}

TEST_CASE("FreeListAllocator - Freed space is reused", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 8192, TAG };

    void *block = allocator.Allocate(256, 16);
    REQUIRE(block != nullptr);

    allocator.Free(block);
    REQUIRE(allocator.AllocationCount() == 0);

    // Same size and alignment, so the block just released is the obvious fit.
    REQUIRE(allocator.Allocate(256, 16) == block);
}

TEST_CASE("FreeListAllocator - Adjacent free blocks coalesce", "[memory][allocator][freelist]") {
    constexpr size_t CAPACITY = 4096;
    FreeListAllocator allocator{ CAPACITY, TAG };

    const size_t wholeRegion = allocator.LargestFreeBlock();

    void *a = allocator.Allocate(512, 16);
    void *b = allocator.Allocate(512, 16);
    void *c = allocator.Allocate(512, 16);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    REQUIRE(c != nullptr);

    // Free the middle first: it has an allocated neighbour on each side, so nothing can merge yet.
    allocator.Free(b);
    const size_t afterMiddle = allocator.LargestFreeBlock();

    // Now free both neighbours. Backward and forward coalescing must fold all three back into the tail.
    allocator.Free(a);
    allocator.Free(c);

    REQUIRE(allocator.AllocationCount() == 0);
    REQUIRE(allocator.Used() == 0);
    REQUIRE(allocator.LargestFreeBlock() > afterMiddle);

    // The decisive check: without coalescing the region would be three stranded holes and this could not fit.
    REQUIRE(allocator.LargestFreeBlock() == wholeRegion);
    REQUIRE(allocator.Allocate(wholeRegion, 1) != nullptr);
}

TEST_CASE("FreeListAllocator - Fragmentation is visible without coalescing opportunities", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 8192, TAG };

    std::vector<void *> blocks;

    for (i32 i = 0; i < 8; ++i) {
        void *block = allocator.Allocate(256, 16);
        REQUIRE(block != nullptr);
        blocks.push_back(block);
    }

    // Free every other block, leaving holes separated by live allocations.
    for (size_t i = 0; i < blocks.size(); i += 2) {
        allocator.Free(blocks[i]);
    }

    // Plenty of free bytes in total, but none of the holes is large enough on its own - which is exactly the
    // condition LargestFreeBlock exists to surface.
    REQUIRE(allocator.LargestFreeBlock() < allocator.Capacity() - allocator.Used());
    REQUIRE(allocator.Allocate(2048, 16) != nullptr); // The untouched tail can still serve a big request.

    for (size_t i = 1; i < blocks.size(); i += 2) {
        allocator.Free(blocks[i]);
    }
}

TEST_CASE("FreeListAllocator - A request that cannot fit returns nullptr", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 1024, TAG };

    // Fixed reservation by design: growth would mean a second region and defeat the bounded budget.
    REQUIRE(allocator.Allocate(4096, 8) == nullptr);
    REQUIRE(allocator.AllocationCount() == 0);

    void *block = allocator.Allocate(256, 8);
    REQUIRE(block != nullptr);
    REQUIRE(allocator.Allocate(1024, 8) == nullptr);

    allocator.Free(block);
}

TEST_CASE("FreeListAllocator - Reset returns the region to one free block", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 4096, TAG };

    const size_t wholeRegion = allocator.LargestFreeBlock();

    for (i32 i = 0; i < 4; ++i) {
        REQUIRE(allocator.Allocate(256, 16) != nullptr);
    }

    REQUIRE(allocator.AllocationCount() == 4);

    allocator.Reset();

    REQUIRE(allocator.AllocationCount() == 0);
    REQUIRE(allocator.Used() == 0);
    REQUIRE(allocator.LargestFreeBlock() == wholeRegion);
}

TEST_CASE("FreeListAllocator - Zero-sized requests get distinct addresses", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 4096, TAG };

    // Matches operator new: a zero-byte allocation is still a unique address.
    void *first = allocator.Allocate(0, 8);
    void *second = allocator.Allocate(0, 8);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first != second);
}

TEST_CASE("FreeListAllocator - Churn keeps the region consistent", "[memory][allocator][freelist]") {
    FreeListAllocator allocator{ 32 * 1024, TAG };

    std::vector<void *> live;
    std::set<void *> seen;

    // Interleaved allocate/free with varying sizes, so blocks split, merge and get reused repeatedly.
    for (i32 round = 0; round < 200; ++round) {
        const size_t size = 16 + static_cast<size_t>((round * 37) % 400);

        if (void *block = allocator.Allocate(size, 16); block != nullptr) {
            std::memset(block, round & 0xFF, size);
            live.push_back(block);
            seen.insert(block);
        }

        if (live.size() > 12) {
            allocator.Free(live.front());
            live.erase(live.begin());
        }
    }

    for (void *block : live) {
        allocator.Free(block);
    }

    REQUIRE(allocator.AllocationCount() == 0);
    REQUIRE(allocator.Used() == 0);
    REQUIRE(seen.size() > 1); // Addresses were genuinely recycled rather than the region just marching forward.

    // Everything merged back: the region is whole again after all that churn.
    REQUIRE(allocator.Allocate(allocator.LargestFreeBlock(), 1) != nullptr);
}

TEST_CASE("FreeListAllocator - Reservation and usage are reported to the tracker", "[memory][allocator][freelist]") {
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(TAG);
    const i64 usedBefore = MemoryTracker::PoolUsedBytes(TAG);

    {
        FreeListAllocator allocator{ 16384, TAG };

        REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore + 16384);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);

        void *block = allocator.Allocate(1000, 16);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) >= usedBefore + 1000);

        allocator.Free(block);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
    }

    REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore);
    REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
}
