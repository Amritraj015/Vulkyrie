#include "memory/allocators/heap_allocator.h"

#include "memory/allocation_block.h"

namespace Vulkyrie {

    HeapAllocator::HeapAllocator(MemoryTag tag)
        : mUsed{ 0 }
        , mAllocationCount{ 0 }
        , mHighWaterMark{ 0 }
        , mTag{ tag } {
    }

    HeapAllocator::~HeapAllocator() {
        VASSERT(mAllocationCount.load(std::memory_order_relaxed) == 0, "Heap allocator destroyed with allocations still outstanding.");
    }

    void *HeapAllocator::Allocate(size_t size, size_t alignment) {
        // Zero-sized requests still get their own address, so two of them never compare equal.
        const size_t payloadSize = std::max(size, size_t{ 1 });

        // Attributed to the allocator's own tag rather than to whatever scope happens to be on the calling
        // thread, which is what makes this usable from a driver's threads.
        void *payload = detail::AllocateBlock(payloadSize, alignment, mTag, detail::BlockOwner::HeapAllocator);

        if (nullptr == payload) {
            return nullptr;
        }

        const size_t used = mUsed.fetch_add(payloadSize, std::memory_order_relaxed) + payloadSize;
        mAllocationCount.fetch_add(1, std::memory_order_relaxed);

        // Plain load first: once a caller settles at its high-water mark this is a load and a not-taken branch
        // rather than a second read-modify-write.
        if (used > mHighWaterMark.load(std::memory_order_relaxed)) {
            mHighWaterMark.fetch_max(used, std::memory_order_relaxed);
        }

        return payload;
    }

    void *HeapAllocator::Reallocate(void *pointer, size_t size, size_t alignment) {
        if (nullptr == pointer) {
            return Allocate(size, alignment);
        }

        if (0 == size) {
            Free(pointer);

            return nullptr;
        }

        void *replacement = Allocate(size, alignment);

        if (nullptr == replacement) {
            // Leaves the original intact, so a failed resize costs the caller nothing but the answer.
            return nullptr;
        }

        std::memcpy(replacement, pointer, std::min(SizeOf(pointer), size));

        Free(pointer);

        return replacement;
    }

    void HeapAllocator::Free(void *pointer) {
        if (nullptr == pointer) {
            return;
        }

        const size_t size = detail::ReleaseBlock(pointer, detail::BlockOwner::HeapAllocator);

        mUsed.fetch_sub(size, std::memory_order_relaxed);
        mAllocationCount.fetch_sub(1, std::memory_order_relaxed);
    }

    size_t HeapAllocator::SizeOf(const void *pointer) {
        VASSERT(pointer != nullptr, "Heap allocator asked for the size of a null pointer.");

        return detail::BlockSize(pointer);
    }

} // namespace Vulkyrie
