#include <catch2/catch_test_macros.hpp>
#include <memory/allocators/arena_allocator.h>
#include <memory/memory_tracker.h>

#include <cstring>
#include <vector>

using namespace Vulkyrie;

namespace {

    constexpr auto TAG = MemoryTag::Core;

} // namespace

TEST_CASE("ArenaAllocator - Allocations are aligned and distinct", "[memory][allocator]") {
    ArenaAllocator arena{ 4096, TAG };

    void *first = arena.Allocate(1, 1);
    void *second = arena.Allocate(8, 8);
    void *third = arena.Allocate(4, 64);

    REQUIRE(first != nullptr);
    REQUIRE(first != second);
    REQUIRE(second != third);

    REQUIRE(reinterpret_cast<uintptr_t>(second) % 8 == 0);
    REQUIRE(reinterpret_cast<uintptr_t>(third) % 64 == 0);

    // A one-byte allocation followed by an 8-aligned one must not overlap.
    REQUIRE(static_cast<std::byte *>(second) >= static_cast<std::byte *>(first) + 1);
}

TEST_CASE("ArenaAllocator - Typed Allocate and Emplace", "[memory][allocator]") {
    ArenaAllocator arena{ 4096, TAG };

    // Count defaults to one, so the same overload covers a single object and an array.
    u64 *single = arena.AllocateArray<u64>();
    u64 *array = arena.AllocateArray<u64>(16);

    REQUIRE(single != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(single) % alignof(u64) == 0);
    REQUIRE(reinterpret_cast<uintptr_t>(array) % alignof(u64) == 0);
    REQUIRE(arena.Used() == sizeof(u64) * 17);

    for (u64 i = 0; i < 16; ++i) {
        array[i] = i;
    }

    REQUIRE(array[15] == 15);

    struct Widget {
        i32 First;
        f32 Second;
    };

    auto *widget = arena.Emplace<Widget>(7, 1.5f);

    REQUIRE(widget->First == 7);
    REQUIRE(widget->Second == 1.5f);
    REQUIRE(reinterpret_cast<uintptr_t>(widget) % alignof(Widget) == 0);
}

TEST_CASE("ArenaAllocator - Reset rewinds without freeing", "[memory][allocator]") {
    ArenaAllocator arena{ 4096, TAG };

    void *first = arena.Allocate(128, 16);
    const size_t capacityBefore = arena.Capacity();

    REQUIRE(arena.Used() == 128);

    arena.Reset();

    REQUIRE(arena.Used() == 0);
    REQUIRE(arena.Capacity() == capacityBefore); // Chunks are retained, which is the point.

    // The next frame hands back the same memory.
    REQUIRE(arena.Allocate(128, 16) == first);
}

TEST_CASE("ArenaAllocator - Growth keeps earlier pointers valid", "[memory][allocator]") {
    ArenaAllocator arena{ 64, TAG };

    // Chunked storage means outgrowing the arena appends storage rather than relocating what was handed out - the
    // property a std::vector-backed arena could not provide.
    std::vector<u64 *> pointers;

    // Comfortably past MIN_CHUNK_BYTES, so the arena is forced to chunk regardless of the requested capacity.
    for (u64 i = 0; i < 4096; ++i) {
        auto *value = static_cast<u64 *>(arena.Allocate(sizeof(u64), alignof(u64)));
        *value = i;
        pointers.push_back(value);
    }

    REQUIRE(arena.ChunkCount() > 1);

    for (u64 i = 0; i < pointers.size(); ++i) {
        REQUIRE(*pointers[i] == i);
    }
}

TEST_CASE("ArenaAllocator - Steady state stops allocating", "[memory][allocator]") {
    ArenaAllocator arena{ 256, TAG };

    const auto runFrame = [&arena] {
        for (i32 i = 0; i < 200; ++i) {
            (void)arena.Allocate(32, 8);
        }

        arena.Reset();
    };

    runFrame(); // Grows to the high-water mark.

    const i64 heapBefore = MemoryTracker::TotalAllocated(TAG);
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(TAG);
    const size_t chunksBefore = arena.ChunkCount();

    runFrame();
    runFrame();

    // Neither channel moves: no new chunks reserved, and no heap allocation for the chunk bookkeeping either.
    REQUIRE(MemoryTracker::TotalAllocated(TAG) == heapBefore);
    REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore);
    REQUIRE(arena.ChunkCount() == chunksBefore);
}

TEST_CASE("ArenaAllocator - An allocation larger than a chunk still succeeds", "[memory][allocator]") {
    ArenaAllocator arena{ 128, TAG };

    auto *block = static_cast<std::byte *>(arena.Allocate(64 * 1024, 32));

    REQUIRE(block != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(block) % 32 == 0);

    std::memset(block, 0xAB, 64 * 1024);
    REQUIRE(static_cast<u8>(block[64 * 1024 - 1]) == 0xAB);
}

TEST_CASE("ArenaAllocator - Reservation and usage are reported to the tracker", "[memory][allocator]") {
    const i64 reservedBefore = MemoryTracker::PoolReservedBytes(TAG);
    const i64 usedBefore = MemoryTracker::PoolUsedBytes(TAG);
    const i64 heapBefore = MemoryTracker::CurrentBytes(TAG);

    {
        ArenaAllocator arena{ 8192, TAG };

        // The chunk is reported as reserved, not as bytes the subsystem has handed out.
        REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore + 8192);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);

        // Usage is live: every allocation reports, so the tracker is accurate the instant it is read. This is the
        // same contract the pool, stack and free-list allocators honour.
        (void)arena.Allocate(16, 8);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore + 16);

        (void)arena.Allocate(32, 8);
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore + 48);

        arena.Reset();
        REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
        REQUIRE(MemoryTracker::PoolPeakUsedBytes(TAG) >= usedBefore + 48);

        // Deliberately not charged against the per-allocation heap counters: an arena's reservation and a
        // subsystem's live objects are different questions and the report keeps them apart.
        REQUIRE(MemoryTracker::CurrentBytes(TAG) < heapBefore + 8192);
    }

    REQUIRE(MemoryTracker::PoolReservedBytes(TAG) == reservedBefore);
    REQUIRE(MemoryTracker::PoolUsedBytes(TAG) == usedBefore);
}
