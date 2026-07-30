#include <catch2/catch_test_macros.hpp>
#include <memory/allocators/pool_allocator.h>
#include <memory/memory_tracker.h>

#include <set>
#include <vector>

using namespace Vulkyrie;

namespace {

    constexpr auto TAG = MemoryTag::GpuVram;

    /** @brief Counts its own live instances, so a test can see that a pool block really is usable storage for a
     * constructed object and that releasing it does not disturb the pool's bookkeeping. */
    struct Tracked {
        static inline i32 LiveCount = 0;

        i32 Value;

        explicit Tracked(i32 value)
            : Value{ value } {
            ++LiveCount;
        }

        ~Tracked() {
            --LiveCount;
        }

        VE_DELETE_MOVE_AND_COPY(Tracked);
    };

} // namespace

TEST_CASE("PoolAllocator - Blocks are distinct, aligned and reusable", "[memory][allocator][pool]") {
    PoolAllocator pool{ 64, 16, 8, TAG };

    REQUIRE(pool.BlockSize() == 64);
    REQUIRE(pool.Capacity() == 8);
    REQUIRE(pool.UsedBlocks() == 0);

    std::set<void *> addresses;

    for (size_t i = 0; i < 8; ++i) {
        void *block = pool.Allocate();

        REQUIRE(block != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(block) % 16 == 0);
        REQUIRE(addresses.insert(block).second); // Never hands the same block out twice.
    }

    REQUIRE(pool.UsedBlocks() == 8);
    REQUIRE(pool.FreeBlocks() == 0);

    // Freeing puts the block straight back at the head, so the next Allocate returns it.
    void *first = *addresses.begin();
    pool.Free(first);

    REQUIRE(pool.UsedBlocks() == 7);
    REQUIRE(pool.Allocate() == first);
}

TEST_CASE("PoolAllocator - Typed Allocate and Emplace", "[memory][allocator][pool]") {
    Tracked::LiveCount = 0;

    PoolAllocator pool{ sizeof(Tracked), alignof(Tracked), 4, TAG };

    // No count parameter here on purpose: a pool cannot hand back contiguous objects.
    Tracked *raw = pool.Allocate<Tracked>();
    REQUIRE(raw != nullptr);
    REQUIRE(Tracked::LiveCount == 0); // Storage only - nothing constructed yet.
    pool.Free(raw);

    Tracked *built = pool.Emplace<Tracked>(5);
    REQUIRE(built->Value == 5);
    REQUIRE(Tracked::LiveCount == 1);

    std::destroy_at(built);
    pool.Free(built);

    REQUIRE(Tracked::LiveCount == 0);
    REQUIRE(pool.UsedBlocks() == 0);
}

TEST_CASE("PoolAllocator - Block size is rounded up to hold the free-list link", "[memory][allocator][pool]") {
    // A one-byte block could not hold the intrusive next-pointer, so the pool widens it rather than corrupting
    // the free list.
    PoolAllocator pool{ 1, 1, 4, TAG };

    REQUIRE(pool.BlockSize() >= sizeof(void *));

    void *a = pool.Allocate();
    void *b = pool.Allocate();

    REQUIRE(a != b);

    pool.Free(a);
    pool.Free(b);

    REQUIRE(pool.UsedBlocks() == 0);
}

TEST_CASE("PoolAllocator - Exhaustion appends a chunk instead of failing", "[memory][allocator][pool]") {
    PoolAllocator pool{ 32, 8, 4, TAG };

    REQUIRE(pool.ChunkCount() == 1);
    REQUIRE(pool.Capacity() == 4);

    std::vector<void *> blocks;

    for (size_t i = 0; i < 10; ++i) {
        void *block = pool.Allocate();
        REQUIRE(block != nullptr);
        blocks.push_back(block);
    }

    REQUIRE(pool.ChunkCount() > 1);
    REQUIRE(pool.Capacity() >= 10);
    REQUIRE(pool.UsedBlocks() == 10);
    REQUIRE(pool.PeakUsedBlocks() == 10);

    // Blocks from earlier chunks stay valid after growth - nothing is relocated.
    for (void *block : blocks) {
        pool.Free(block);
    }

    REQUIRE(pool.UsedBlocks() == 0);
}

TEST_CASE("PoolAllocator - Blocks can hold objects with non-trivial lifetimes", "[memory][allocator][pool]") {
    Tracked::LiveCount = 0;

    PoolAllocator pool{ sizeof(Tracked), alignof(Tracked), 4, TAG };

    auto *first = std::construct_at(static_cast<Tracked *>(pool.Allocate()), 11);
    auto *second = std::construct_at(static_cast<Tracked *>(pool.Allocate()), 22);

    REQUIRE(first->Value == 11);
    REQUIRE(second->Value == 22);
    REQUIRE(Tracked::LiveCount == 2);

    std::destroy_at(first);
    pool.Free(first);
    REQUIRE(Tracked::LiveCount == 1);

    std::destroy_at(second);
    pool.Free(second);
    REQUIRE(Tracked::LiveCount == 0);
    REQUIRE(pool.UsedBlocks() == 0);

    // Freeing a null pointer is a no-op, matching delete.
    pool.Free(nullptr);
    REQUIRE(pool.UsedBlocks() == 0);
}

TEST_CASE("PoolAllocator - Reset reclaims every block without releasing chunks", "[memory][allocator][pool]") {
    PoolAllocator pool{ 32, 8, 4, TAG };

    for (size_t i = 0; i < 9; ++i) {
        (void)pool.Allocate();
    }

    const size_t chunksBefore = pool.ChunkCount();
    const size_t capacityBefore = pool.Capacity();

    pool.Reset();

    REQUIRE(pool.UsedBlocks() == 0);
    REQUIRE(pool.FreeBlocks() == capacityBefore);
    REQUIRE(pool.ChunkCount() == chunksBefore); // Capacity is retained, which is what makes reuse allocation-free.

    for (size_t i = 0; i < capacityBefore; ++i) {
        REQUIRE(pool.Allocate() != nullptr);
    }
}

TEST_CASE("PoolAllocator - Reservation and usage are reported to the tracker", "[memory][allocator][pool]") {
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(TAG);
    const i64 usedBefore = MemoryTracker::PoolUsedBytes(TAG);
    const i64 heapBefore = MemoryTracker::CurrentBytes(TAG);

    {
        PoolAllocator pool{ 128, 16, 16, TAG };

        // The whole chunk is reserved up front, and nothing is in use yet.
        REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore + static_cast<i64>(128 * 16));
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);

        void *first = pool.Allocate();
        void *second = pool.Allocate();

        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore + 256);

        pool.Free(first);
        pool.Free(second);

        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);

        // The reservation is not charged against the per-allocation heap counters - that separation is the point
        // of the reserved-pool channel.
        REQUIRE(MemoryTracker::CurrentBytes(TAG) < heapBefore + static_cast<i64>(128 * 16));
    }

    // Destruction returns the reservation.
    REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore);
    REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
}

TEST_CASE("PoolAllocator - Moving transfers ownership and accounting", "[memory][allocator][pool]") {
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(TAG);
    const i64 usedBefore = MemoryTracker::PoolUsedBytes(TAG);

    {
        PoolAllocator source{ 64, 8, 8, TAG };
        void *block = source.Allocate();

        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore + 64);

        PoolAllocator moved{ std::move(source) };

        REQUIRE(moved.UsedBlocks() == 1);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore + 64);

        moved.Free(block);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
    }

    REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore);
}
