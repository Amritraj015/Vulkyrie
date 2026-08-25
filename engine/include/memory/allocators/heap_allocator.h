#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include <atomic>

namespace Vulkyrie {

    /** @brief A general-purpose allocator with no capacity limit, safe to call from several threads at once.
     *
     * The rest of the toolkit trades generality for speed: `ArenaAllocator` cannot free one allocation,
     * `PoolAllocator` needs uniform sizes, `FreeListAllocator` is bounded and single-threaded. This one gives up
     * that speed to place no demands at all on the caller - any size, any alignment, freed in any order, from any
     * thread, growing as far as the process heap allows. Prefer any of the others when the use case fits one.
     *
     * It exists for memory whose shape and volume are decided by code outside the engine, where a fixed budget
     * would be a guess: a graphics driver's host allocations are the case it was written for. Storage comes from
     * the process heap; what the allocator adds is attribution, a size that can be asked of a pointer, and
     * reallocation.
     *
     * Bytes are reported through `MemoryTracker`'s per-subsystem counters rather than its reserved-pool channel,
     * because there is no reservation to report - each allocation is taken and returned on demand.
     *
     * Thread-safe: allocation, reallocation and free may run concurrently. */
    class HeapAllocator final {
    public:
        /** @brief Constructs an allocator attributing everything it serves to one subsystem.
         * @param tag Subsystem the allocations are attributed to. */
        explicit HeapAllocator(MemoryTag tag);

        ~HeapAllocator();

        // Counters cannot move with the storage they describe, and nothing owns a region to transfer.
        VE_DELETE_MOVE_AND_COPY(HeapAllocator);

        /** @brief Allocates uninitialized storage.
         * @param size Bytes to allocate; zero is treated as one byte so every allocation has a distinct address.
         * @param alignment Required alignment in bytes; must be a power of two.
         * @returns Pointer to uninitialized storage, or nullptr when the heap cannot satisfy the request. */
        [[nodiscard]] void *Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

        /** @brief Resizes an allocation, preserving its contents up to the smaller of the two sizes.
         * @param pointer A pointer previously returned by `Allocate`, or nullptr to allocate afresh.
         * @param size New size in bytes; zero frees `pointer` and returns nullptr.
         * @param alignment Required alignment of the result; must be a power of two.
         * @returns Pointer to the resized storage, or nullptr on failure - in which case `pointer` is untouched
         * and still needs freeing. */
        [[nodiscard]] void *Reallocate(void *pointer, size_t size, size_t alignment = alignof(std::max_align_t));

        /** @brief Returns an allocation to the heap.
         * @param pointer A pointer previously returned by `Allocate` or `Reallocate`, or nullptr. */
        void Free(void *pointer);

        /** @brief Returns the size an allocation was requested with.
         * @param pointer A pointer previously returned by `Allocate` or `Reallocate`; must not be nullptr.
         * @returns The size in bytes, which is what was asked for rather than what was reserved. */
        [[nodiscard]] static size_t SizeOf(const void *pointer);

        /** @brief Carves storage for `count` objects of type `T`, correctly aligned but not constructed.
         * @tparam T The object type; only its size and alignment are used.
         * @param count Number of objects; defaults to one.
         * @returns Pointer to uninitialized storage, or nullptr when the heap cannot satisfy the request. */
        template <typename T> [[nodiscard]] T *AllocateArray(size_t count = 1) {
            VASSERT(count <= SIZE_MAX / sizeof(T), "Heap allocator array allocation overflows size_t.");

            return static_cast<T *>(Allocate(sizeof(T) * count, alignof(T)));
        }

        /** @brief Carves storage for one `T` and constructs it in place.
         * @tparam T The object type.
         * @tparam TArgs Constructor argument types.
         * @param args Arguments forwarded to `T`'s constructor.
         * @returns Pointer to the constructed object, or nullptr when the heap is exhausted.
         *
         * `Free` does not run the destructor; pair a non-trivial `T` with `std::destroy_at` before freeing. */
        template <typename T, typename... TArgs> [[nodiscard]] T *Emplace(TArgs &&...args) {
            T *storage = AllocateArray<T>();

            return storage == nullptr ? nullptr : std::construct_at(storage, std::forward<TArgs>(args)...);
        }

        /** @brief Returns the payload bytes currently handed out, excluding per-allocation overhead. */
        [[nodiscard]] VE_INLINE size_t Used() const noexcept {
            return mUsed.load(std::memory_order_relaxed);
        }

        /** @brief Returns the number of allocations currently outstanding. */
        [[nodiscard]] VE_INLINE size_t AllocationCount() const noexcept {
            return mAllocationCount.load(std::memory_order_relaxed);
        }

        /** @brief Returns the high-water mark of `Used()`. */
        [[nodiscard]] VE_INLINE size_t HighWaterMark() const noexcept {
            return mHighWaterMark.load(std::memory_order_relaxed);
        }

        /** @brief Returns the subsystem allocations are attributed to. */
        [[nodiscard]] VE_INLINE MemoryTag Tag() const noexcept {
            return mTag;
        }

    private:
        /** @brief Payload bytes currently handed out. */
        std::atomic<size_t> mUsed;

        /** @brief Allocations currently outstanding. */
        std::atomic<size_t> mAllocationCount;

        /** @brief High-water mark of `_used`. */
        std::atomic<size_t> mHighWaterMark;

        /** @brief Memory tag the allocations are attributed to. */
        MemoryTag mTag;
    };

} // namespace Vulkyrie
