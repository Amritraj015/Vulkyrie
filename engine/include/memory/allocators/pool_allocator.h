#pragma once

#include "vlkypch.h"
#include "core/asserts.h"

namespace Vulkyrie {

    /** @brief A fixed-block allocator: every allocation is the same size, so allocation and free are both a single
     * linked-list operation with no search, no splitting and no fragmentation.
     *
     * This is the right shape for the many-uniform-objects cases an engine is full of - contact manifolds,
     * broadphase tree nodes, particles, component records. For mixed sizes use `FreeListAllocator`; for bulk
     * allocation that is released all at once use `LinearAllocator`.
     *
     * The free list is threaded through the free blocks themselves, so an empty pool costs nothing beyond its
     * chunks. Running out of blocks appends a chunk rather than failing, and chunks are retained across `Reset` so
     * a pool that reaches steady state stops allocating.
     *
     * Backing chunks are taken untracked and reported through `MemoryTracker`'s reserved-pool channel.
     *
     * Not thread-safe: one pool belongs to one thread (or is externally synchronized). */
    class PoolAllocator final {
    public:
        /** @brief Constructs a pool serving blocks of a fixed size.
         * @param blockSize Bytes per block. Rounded up to hold the free-list link and to satisfy `blockAlignment`.
         * @param blockAlignment Required alignment of each block; must be a power of two.
         * @param blocksPerChunk Blocks obtained per backing chunk. Larger means fewer, bigger reservations.
         * @param tag Memory tag the backing chunks are attributed to. */
        PoolAllocator(size_t blockSize, size_t blockAlignment, size_t blocksPerChunk, MemoryTag tag = MemoryTag::Untagged);

        ~PoolAllocator();

        VE_DELETE_COPY(PoolAllocator);

        PoolAllocator(PoolAllocator &&other) noexcept;
        PoolAllocator &operator=(PoolAllocator &&other) noexcept;

        /** @brief Takes a block from the free list, appending a chunk if none are free.
         * @returns Pointer to uninitialized block storage, aligned as requested at construction. */
        [[nodiscard]] void *Allocate();

        /** @brief Takes a block and reinterprets it as storage for one `T`, without constructing it.
         *
         * There is deliberately no `count`: a pool serves fixed-size blocks, so it cannot hand back several
         * contiguous objects the way the arena and stack can.
         *
         * @tparam T The object type; must fit the pool's block size and alignment.
         * @returns Pointer to uninitialized storage for one object. */
        template <typename T> [[nodiscard]] T *Allocate() {
            VASSERT(sizeof(T) <= _blockSize, "Type is larger than this pool's block size.");
            VASSERT(alignof(T) <= _blockAlignment, "Type is more strictly aligned than this pool's blocks.");

            return static_cast<T *>(Allocate());
        }

        /** @brief Takes a block and constructs a `T` in it.
         * @tparam T The object type; must fit the pool's block size and alignment.
         * @tparam TArgs Constructor argument types.
         * @param args Arguments forwarded to `T`'s constructor.
         * @returns Pointer to the constructed object.
         *
         * `Free` does not run the destructor; pair a non-trivial `T` with `std::destroy_at` before freeing. */
        template <typename T, typename... TArgs> [[nodiscard]] T *Emplace(TArgs &&...args) {
            return std::construct_at(Allocate<T>(), std::forward<TArgs>(args)...);
        }

        /** @brief Returns a block to the free list.
         * @param block A pointer previously returned by `Allocate`, or nullptr. */
        void Free(void *block);

        /** @brief Returns every block to the free list without releasing any chunk. Does not run destructors. */
        void Reset();

        /** @brief Returns the actual size of each block, after rounding for the link and alignment. */
        [[nodiscard]] VE_INLINE size_t BlockSize() const {
            return _blockSize;
        }

        /** @brief Returns the total number of blocks across every chunk. */
        [[nodiscard]] VE_INLINE size_t Capacity() const {
            return _totalBlocks;
        }

        /** @brief Returns the number of blocks currently handed out. */
        [[nodiscard]] VE_INLINE size_t UsedBlocks() const {
            return _usedBlocks;
        }

        /** @brief Returns the number of blocks available without growing. */
        [[nodiscard]] VE_INLINE size_t FreeBlocks() const {
            return _totalBlocks - _usedBlocks;
        }

        /** @brief Returns the largest number of blocks ever simultaneously in use - what the pool should have been
         * sized for. */
        [[nodiscard]] VE_INLINE size_t PeakUsedBlocks() const {
            return _peakUsedBlocks;
        }

        /** @brief Returns the number of backing chunks. More than one means the pool outgrew its initial size. */
        [[nodiscard]] VE_INLINE size_t ChunkCount() const {
            return _chunks.size();
        }

        /** @brief Returns the subsystem this pool's memory is attributed to. */
        [[nodiscard]] VE_INLINE MemoryTag GetTag() const {
            return _tag;
        }

    private:
        /** @brief Link stored inside a free block. A block must be at least this large, which is why tiny block
         * sizes are rounded up. */
        struct FreeBlock {
        public:
            FreeBlock *Next = nullptr;
        };

        /** @brief One contiguous run of blocks. */
        struct Chunk {
        public:
            std::byte *Memory = nullptr;
            size_t Capacity = 0;
            size_t BlockCount = 0;
        };

        /** @brief Appends a chunk and threads its blocks onto the free list. */
        void addChunk();

        /** @brief Threads every block of one chunk onto the free list, front to back. */
        void threadChunkOntoFreeList(const Chunk &chunk);

        /** @brief Frees every chunk and clears the free list. */
        void releaseChunks();

        /** @brief Retained storage, in allocation order. */
        std::vector<Chunk> _chunks;

        /** @brief Head of the intrusive free list, or nullptr when every block is handed out. */
        FreeBlock *_freeList = nullptr;

        /** @brief Size of each block after rounding. */
        size_t _blockSize = 0;

        /** @brief Alignment of each block. */
        size_t _blockAlignment = 0;

        /** @brief Blocks obtained per chunk. */
        size_t _blocksPerChunk = 0;

        /** @brief Total blocks across every chunk. */
        size_t _totalBlocks = 0;

        /** @brief Blocks currently handed out. */
        size_t _usedBlocks = 0;

        /** @brief High-water mark of `_usedBlocks`. */
        size_t _peakUsedBlocks = 0;

        /** @brief Memory tag the chunks are attributed to. */
        MemoryTag _tag = MemoryTag::Untagged;
    };

} // namespace Vulkyrie
