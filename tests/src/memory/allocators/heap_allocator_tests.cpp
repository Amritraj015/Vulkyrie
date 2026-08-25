#include <catch2/catch_test_macros.hpp>
#include <memory/allocators/heap_allocator.h>
#include <memory/memory_tracker.h>

#include <cstring>
#include <latch>
#include <set>
#include <thread>
#include <vector>

using namespace Vulkyrie;

namespace {

    constexpr auto TAG = MemoryTag::Assets;

} // namespace

TEST_CASE("HeapAllocator - Allocations are distinct, aligned and non-overlapping", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    auto *first = static_cast<std::byte *>(allocator.Allocate(100, 8));
    auto *second = static_cast<std::byte *>(allocator.Allocate(200, 32));
    auto *third = static_cast<std::byte *>(allocator.Allocate(50, 64));

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(third != nullptr);

    REQUIRE(reinterpret_cast<uintptr_t>(first) % 8 == 0);
    REQUIRE(reinterpret_cast<uintptr_t>(second) % 32 == 0);
    REQUIRE(reinterpret_cast<uintptr_t>(third) % 64 == 0);

    std::memset(first, 0xAA, 100);
    std::memset(second, 0xBB, 200);
    std::memset(third, 0xCC, 50);

    REQUIRE(static_cast<u8>(first[99]) == 0xAA);
    REQUIRE(static_cast<u8>(second[199]) == 0xBB);
    REQUIRE(static_cast<u8>(third[49]) == 0xCC);

    allocator.Free(first);
    allocator.Free(second);
    allocator.Free(third);
}

TEST_CASE("HeapAllocator - Over-aligned allocations are honoured", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    std::vector<void *> blocks;

    for (size_t alignment : { size_t{ 1 }, size_t{ 2 }, size_t{ 16 }, size_t{ 256 }, size_t{ 4096 } }) {
        void *block = allocator.Allocate(64, alignment);

        REQUIRE(block != nullptr);
        REQUIRE(reinterpret_cast<uintptr_t>(block) % alignment == 0);

        blocks.push_back(block);
    }

    for (void *block : blocks) {
        allocator.Free(block);
    }
}

TEST_CASE("HeapAllocator - A zero-sized request still gets its own address", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    void *first = allocator.Allocate(0);
    void *second = allocator.Allocate(0);

    REQUIRE(first != nullptr);
    REQUIRE(second != nullptr);
    REQUIRE(first != second);

    allocator.Free(first);
    allocator.Free(second);
}

TEST_CASE("HeapAllocator - SizeOf reports the requested size", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    void *block = allocator.Allocate(12345, 64);

    REQUIRE(HeapAllocator::SizeOf(block) == 12345);

    allocator.Free(block);
}

TEST_CASE("HeapAllocator - Reallocation preserves contents when growing and when shrinking", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    auto *block = static_cast<std::byte *>(allocator.Allocate(64));
    REQUIRE(block != nullptr);

    for (size_t i = 0; i < 64; ++i) {
        block[i] = static_cast<std::byte>(i);
    }

    auto *grown = static_cast<std::byte *>(allocator.Reallocate(block, 4096));
    REQUIRE(grown != nullptr);
    REQUIRE(HeapAllocator::SizeOf(grown) == 4096);

    for (size_t i = 0; i < 64; ++i) {
        REQUIRE(static_cast<u8>(grown[i]) == i);
    }

    auto *shrunk = static_cast<std::byte *>(allocator.Reallocate(grown, 16));
    REQUIRE(shrunk != nullptr);
    REQUIRE(HeapAllocator::SizeOf(shrunk) == 16);

    for (size_t i = 0; i < 16; ++i) {
        REQUIRE(static_cast<u8>(shrunk[i]) == i);
    }

    allocator.Free(shrunk);
}

TEST_CASE("HeapAllocator - Reallocation handles the null and zero-size edges", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    // A null original allocates.
    void *fresh = allocator.Reallocate(nullptr, 128);
    REQUIRE(fresh != nullptr);
    REQUIRE(HeapAllocator::SizeOf(fresh) == 128);

    // A zero size frees and yields null.
    REQUIRE(allocator.Reallocate(fresh, 0) == nullptr);
    REQUIRE(allocator.AllocationCount() == 0);
}

TEST_CASE("HeapAllocator - Reallocation can raise the alignment", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    void *block = allocator.Allocate(32, 8);
    REQUIRE(block != nullptr);

    void *realigned = allocator.Reallocate(block, 32, 512);
    REQUIRE(realigned != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(realigned) % 512 == 0);

    allocator.Free(realigned);
}

TEST_CASE("HeapAllocator - Counters follow allocation and free", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    REQUIRE(allocator.Used() == 0);
    REQUIRE(allocator.AllocationCount() == 0);

    void *first = allocator.Allocate(1000);
    void *second = allocator.Allocate(2000);

    REQUIRE(allocator.Used() == 3000);
    REQUIRE(allocator.AllocationCount() == 2);
    REQUIRE(allocator.HighWaterMark() == 3000);

    allocator.Free(first);

    REQUIRE(allocator.Used() == 2000);
    REQUIRE(allocator.AllocationCount() == 1);

    // The peak is a high-water mark, so freeing does not lower it.
    REQUIRE(allocator.HighWaterMark() == 3000);

    allocator.Free(second);

    REQUIRE(allocator.Used() == 0);
    REQUIRE(allocator.AllocationCount() == 0);
}

TEST_CASE("HeapAllocator - Freeing null is a no-op", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    allocator.Free(nullptr);

    REQUIRE(allocator.AllocationCount() == 0);
}

TEST_CASE("HeapAllocator - Bytes are attributed to the allocator's tag", "[memory][allocator][heap]") {
    constexpr auto LOCAL_TAG = MemoryTag::Networking;

    const i64 allocatedBefore = MemoryTracker::TotalAllocated(LOCAL_TAG);
    const i64 freedBefore = MemoryTracker::TotalFreed(LOCAL_TAG);

    HeapAllocator allocator{ LOCAL_TAG };

    void *block = allocator.Allocate(8192);

    REQUIRE(MemoryTracker::TotalAllocated(LOCAL_TAG) >= allocatedBefore + 8192);

    allocator.Free(block);

    REQUIRE(MemoryTracker::TotalFreed(LOCAL_TAG) >= freedBefore + 8192);
}

TEST_CASE("HeapAllocator - Concurrent allocation and free stays consistent", "[memory][allocator][heap]") {
    HeapAllocator allocator{ TAG };

    constexpr size_t THREADS = 8;
    constexpr size_t PER_THREAD = 2000;

    std::latch start{ THREADS };
    std::vector<std::thread> workers;
    std::atomic<size_t> failures{ 0 };

    for (size_t t = 0; t < THREADS; ++t) {
        workers.emplace_back([&allocator, &start, &failures, t] {
            start.arrive_and_wait();

            std::vector<void *> mine;
            mine.reserve(PER_THREAD);

            for (size_t i = 0; i < PER_THREAD; ++i) {
                const size_t size = 1 + ((t * PER_THREAD + i) % 512);
                void *block = allocator.Allocate(size, 16);

                if (block == nullptr || HeapAllocator::SizeOf(block) != size) {
                    ++failures;
                    continue;
                }

                // Write the whole block so an overlapping allocation would corrupt a neighbour.
                std::memset(block, static_cast<int>(t + 1), size);
                mine.push_back(block);
            }

            for (void *block : mine) {
                allocator.Free(block);
            }
        });
    }

    for (std::thread &worker : workers) {
        worker.join();
    }

    REQUIRE(failures.load() == 0);
    REQUIRE(allocator.Used() == 0);
    REQUIRE(allocator.AllocationCount() == 0);
}
