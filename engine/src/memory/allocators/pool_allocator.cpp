#include "memory/allocators/pool_allocator.h"

#include "core/asserts.h"
#include "memory/memory_tracker.h"

namespace Vulkyrie {

    namespace {

        /** @brief Rounds a size up to the next multiple of a power-of-two alignment. */
        [[nodiscard]] VE_INLINE size_t AlignUp(size_t value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

    } // namespace

    PoolAllocator::PoolAllocator(size_t blockSize, size_t blockAlignment, size_t blocksPerChunk, MemoryTag tag)
        : _blockAlignment{ std::max<size_t>(blockAlignment, alignof(FreeBlock)) }
        , _blocksPerChunk{ std::max<size_t>(blocksPerChunk, 1) }
        , _tag{ tag } {
        VASSERT(blockAlignment > 0 && (blockAlignment & (blockAlignment - 1)) == 0, "Pool allocator block alignment must be a power of two.");

        // A free block has to hold the list link, and consecutive blocks have to stay aligned, so the effective
        // block size is the requested size rounded up past both constraints.
        _blockSize = AlignUp(std::max(blockSize, sizeof(FreeBlock)), _blockAlignment);

        addChunk();
    }

    PoolAllocator::~PoolAllocator() {
        releaseChunks();
    }

    PoolAllocator::PoolAllocator(PoolAllocator &&other) noexcept
        : _chunks{ std::move(other._chunks) }
        , _freeList{ other._freeList }
        , _blockSize{ other._blockSize }
        , _blockAlignment{ other._blockAlignment }
        , _blocksPerChunk{ other._blocksPerChunk }
        , _totalBlocks{ other._totalBlocks }
        , _usedBlocks{ other._usedBlocks }
        , _peakUsedBlocks{ other._peakUsedBlocks }
        , _tag{ other._tag } {
        other._chunks.clear();
        other._freeList = nullptr;
        other._totalBlocks = 0;
        other._usedBlocks = 0;
    }

    PoolAllocator &PoolAllocator::operator=(PoolAllocator &&other) noexcept {
        if (this != &other) {
            releaseChunks();

            _chunks = std::move(other._chunks);
            _freeList = other._freeList;
            _blockSize = other._blockSize;
            _blockAlignment = other._blockAlignment;
            _blocksPerChunk = other._blocksPerChunk;
            _totalBlocks = other._totalBlocks;
            _usedBlocks = other._usedBlocks;
            _peakUsedBlocks = other._peakUsedBlocks;
            _tag = other._tag;

            // The adopted chunks were already reported as reserved under the same tag; only the in-use bytes
            // change owner.
            MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(_usedBlocks * _blockSize));

            other._chunks.clear();
            other._freeList = nullptr;
            other._totalBlocks = 0;
            other._usedBlocks = 0;
        }

        return *this;
    }

    void *PoolAllocator::Allocate() {
        if (_freeList == nullptr) {
            addChunk();
        }

        FreeBlock *block = _freeList;
        _freeList = block->Next;

        ++_usedBlocks;
        _peakUsedBlocks = std::max(_peakUsedBlocks, _usedBlocks);

        MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(_blockSize));

        return block;
    }

    void PoolAllocator::Free(void *block) {
        if (block == nullptr) {
            return;
        }

        VASSERT(_usedBlocks > 0, "Pool allocator freed a block while none were handed out.");

        // The free list is threaded through the blocks themselves, so a foreign pointer would not just be
        // rejected late - it would be linked in and handed out again by a later Allocate.
        VASSERT(ownsBlock(block), "Pool allocator freed a pointer that is not one of its blocks.");

        auto *node = static_cast<FreeBlock *>(block);
        node->Next = _freeList;
        _freeList = node;

        --_usedBlocks;

        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_blockSize));
    }

    void PoolAllocator::Reset() {
        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_usedBlocks * _blockSize));

        _freeList = nullptr;
        _usedBlocks = 0;

        // Rebuild front to back so the free list walks chunks in address order, which keeps a freshly reset pool
        // handing out blocks in a cache-friendly sequence.
        for (auto chunk = _chunks.rbegin(); chunk != _chunks.rend(); ++chunk) {
            threadChunkOntoFreeList(*chunk);
        }
    }

    bool PoolAllocator::ownsBlock(const void *block) const {
        const auto *address = static_cast<const std::byte *>(block);

        for (const Chunk &chunk : _chunks) {
            if (address >= chunk.Memory && address < chunk.Memory + chunk.Capacity) {
                return static_cast<size_t>(address - chunk.Memory) % _blockSize == 0;
            }
        }

        return false;
    }

    void PoolAllocator::addChunk() {
        const size_t capacity = _blockSize * _blocksPerChunk;

        // Untracked on purpose: reported below through the reserved-pool channel instead, so a pool's reservation
        // is not conflated with bytes a subsystem has handed out.
        auto *memory = static_cast<std::byte *>(std::aligned_alloc(_blockAlignment, AlignUp(capacity, _blockAlignment)));
        VASSERT(memory != nullptr, "Pool allocator failed to reserve a chunk.");

        Chunk chunk{ .Memory = memory, .Capacity = capacity, .BlockCount = _blocksPerChunk };

        {
            VE_MEMORY_SCOPE(_tag);
            _chunks.push_back(chunk);
        }

        _totalBlocks += _blocksPerChunk;

        MemoryTracker::OnPoolReserve(_tag, static_cast<i64>(capacity));

        threadChunkOntoFreeList(chunk);
    }

    void PoolAllocator::threadChunkOntoFreeList(const Chunk &chunk) {
        // Back to front, so the head ends up being the chunk's first block.
        for (size_t index = chunk.BlockCount; index > 0; --index) {
            auto *node = reinterpret_cast<FreeBlock *>(chunk.Memory + (index - 1) * _blockSize);
            node->Next = _freeList;
            _freeList = node;
        }
    }

    void PoolAllocator::releaseChunks() {
        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_usedBlocks * _blockSize));
        _usedBlocks = 0;

        for (Chunk &chunk : _chunks) {
            MemoryTracker::OnPoolRelease(_tag, static_cast<i64>(chunk.Capacity));
            std::free(chunk.Memory);
        }

        _chunks.clear();
        _freeList = nullptr;
        _totalBlocks = 0;
    }

} // namespace Vulkyrie
