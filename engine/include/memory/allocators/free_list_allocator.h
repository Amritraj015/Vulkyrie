#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {

    /** @brief A general-purpose allocator operating inside one reserved block: arbitrary sizes, freed in any
     * order, with adjacent free space merged back together.
     *
     * This is the fallback of the toolkit. It is strictly more capable and strictly slower than the others -
     * allocation walks a free list, and freeing has to look at physical neighbours - so reach for `ArenaAllocator`
     * when everything can be released at once, and `PoolAllocator` when the objects are uniform. What this one buys is a
     * bounded, self-contained heap: a subsystem can be given a fixed budget it cannot exceed, and its memory stays
     * in one contiguous region rather than scattered across the process heap.
     *
     * Blocks carry boundary tags (their own size and their physical predecessor's), so freeing can find both
     * neighbours in constant time and coalesce with either. Free blocks are linked into a doubly-linked list
     * threaded through their own payloads, so an empty allocator costs nothing beyond its reserved block.
     *
     * The reservation is fixed: unlike the other allocators here this one does not grow, because growth would mean
     * a second region and defeat the point of a bounded budget. `Allocate` returns nullptr when the request cannot
     * be satisfied.
     *
     * The reserved block is taken untracked and reported through `MemoryTracker`'s reserved-pool channel.
     *
     * Not thread-safe: one allocator belongs to one thread (or is externally synchronized). */
    class FreeListAllocator final {
    public:
        /** @brief Reserves a fixed region to allocate from.
         * @param capacityBytes Size of the region in bytes.
         * @param tag Subsystem the reservation is attributed to; required, see `ArenaAllocator`. */
        FreeListAllocator(size_t capacityBytes, MemoryTag tag);

        ~FreeListAllocator();

        VE_DELETE_COPY(FreeListAllocator);

        FreeListAllocator(FreeListAllocator &&other) noexcept;
        FreeListAllocator &operator=(FreeListAllocator &&other) noexcept;

        /** @brief Finds a free block large enough, splitting it if the remainder is usable.
         * @param size Bytes to allocate; zero is treated as one byte so every allocation has a distinct address.
         * @param alignment Required alignment in bytes; must be a power of two.
         * @returns Pointer to uninitialized storage, or nullptr when the region cannot satisfy the request. */
        [[nodiscard]] void *Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

        /** @brief Carves storage for `count` objects of type `T`, correctly aligned but not constructed.
         * @tparam T The object type; only its size and alignment are used.
         * @param count Number of objects; defaults to one.
         * @returns Pointer to uninitialized storage, or nullptr when the region cannot satisfy the request. */
        template <typename T> [[nodiscard]] T *AllocateArray(size_t count = 1) {
            VASSERT(count <= SIZE_MAX / sizeof(T), "Free list allocator array allocation overflows size_t.");

            return static_cast<T *>(Allocate(sizeof(T) * count, alignof(T)));
        }

        /** @brief Carves storage for one `T` and constructs it in place.
         * @tparam T The object type.
         * @tparam TArgs Constructor argument types.
         * @param args Arguments forwarded to `T`'s constructor.
         * @returns Pointer to the constructed object, or nullptr when the region is exhausted - unlike the growing
         * allocators this one can fail, so the null case is propagated rather than constructed into.
         *
         * `Free` does not run the destructor; pair a non-trivial `T` with `std::destroy_at` before freeing. */
        template <typename T, typename... TArgs> [[nodiscard]] T *Emplace(TArgs &&...args) {
            T *storage = AllocateArray<T>();

            return storage == nullptr ? nullptr : std::construct_at(storage, std::forward<TArgs>(args)...);
        }

        /** @brief Returns a block to the allocator, merging it with any free physical neighbour.
         * @param pointer A pointer previously returned by `Allocate`, or nullptr. */
        void Free(void *pointer);

        /** @brief Returns the whole region to a single free block. Does not run destructors. */
        void Reset();

        /** @brief Returns the size of the reserved region. */
        [[nodiscard]] VE_INLINE size_t Capacity() const {
            return _capacity;
        }

        /** @brief Returns the payload bytes currently handed out, excluding per-block overhead. */
        [[nodiscard]] VE_INLINE size_t Used() const {
            return _used;
        }

        /** @brief Returns the number of allocations currently outstanding. */
        [[nodiscard]] VE_INLINE size_t AllocationCount() const {
            return _allocationCount;
        }

        /** @brief Returns the high-water mark of `Used()`. */
        [[nodiscard]] VE_INLINE size_t HighWaterMark() const {
            return _highWaterMark;
        }

        /** @brief Returns the largest single allocation the region could currently satisfy, ignoring alignment
         * padding. Comparing this against `Capacity() - Used()` is the cheap read on fragmentation: a large gap
         * between them means the free space is there but chopped up. */
        [[nodiscard]] size_t LargestFreeBlock() const;


    private:
        /** @brief Header prefixing every block, free or allocated. `PrevSize` is the boundary tag that makes
         * backwards coalescing possible without a search. */
        struct BlockHeader {
        public:
            size_t Size = 0;     ///< Total block size in bytes, header included.
            size_t PrevSize = 0; ///< Total size of the physically preceding block, or 0 for the first block.
            bool Free = true;    ///< Whether this block is on the free list.
        };

        /** @brief Free-list links, stored in a free block's payload area. This is why a block's payload can never
         * be smaller than two pointers. */
        struct FreeNode {
        public:
            FreeNode *Next = nullptr;
            FreeNode *Prev = nullptr;
        };

        /** @brief Written immediately before every returned pointer, recording how far back the block header is.
         * Alignment padding means the header is not at a fixed distance, so `Free` needs this to find it. */
        using PayloadOffset = u32;

        /** @brief Smallest payload a block can have, so any block can hold the free-list links. */
        static constexpr size_t MIN_PAYLOAD = sizeof(FreeNode);

        /** @brief Bytes of bookkeeping between a block's start and the earliest possible payload. */
        static constexpr size_t BLOCK_OVERHEAD = sizeof(BlockHeader) + sizeof(PayloadOffset);

        /** @brief Returns the header of the block containing a returned pointer. */
        [[nodiscard]] static BlockHeader *headerFromPayload(void *pointer);

        /** @brief Returns the block physically after `block`, or nullptr if it is the last. */
        [[nodiscard]] BlockHeader *nextBlock(BlockHeader *block) const;

        /** @brief Returns the block physically before `block`, or nullptr if it is the first. */
        [[nodiscard]] BlockHeader *previousBlock(BlockHeader *block) const;

        /** @brief Links a block into the free list. */
        void pushFree(BlockHeader *block);

        /** @brief Unlinks a block from the free list. */
        void popFree(BlockHeader *block);

        /** @brief Merges a free block with its free neighbours and returns the surviving block. */
        [[nodiscard]] BlockHeader *coalesce(BlockHeader *block);

        /** @brief Releases the reserved region. */
        void releaseRegion();

        /** @brief The reserved region. */
        std::byte *_memory = nullptr;

        /** @brief Size of the reserved region. */
        size_t _capacity = 0;

        /** @brief Head of the doubly-linked free list. */
        FreeNode *_freeList = nullptr;

        /** @brief Payload bytes currently handed out. */
        size_t _used = 0;

        /** @brief Allocations currently outstanding. */
        size_t _allocationCount = 0;

        /** @brief High-water mark of `_used`. */
        size_t _highWaterMark = 0;

        /** @brief Memory tag the region is attributed to. */
        MemoryTag _tag;
    };

} // namespace Vulkyrie
