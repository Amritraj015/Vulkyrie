#include "memory/allocators/arena_allocator.h"

#include "core/asserts.h"
#include "memory/memory_tracker.h"

namespace Vulkyrie {

    namespace {

        /** @brief Rounds a pointer-sized value up to the next multiple of a power-of-two alignment. */
        [[nodiscard]] VE_INLINE size_t AlignUp(size_t value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

    } // namespace

    ArenaAllocator::ArenaAllocator(size_t capacityBytes, MemoryTag tag)
        : _tag{ tag } {
        addChunk(capacityBytes);
    }

    ArenaAllocator::~ArenaAllocator() {
        releaseChunks();
    }

    ArenaAllocator::ArenaAllocator(ArenaAllocator &&other) noexcept
        : _chunks{ std::move(other._chunks) }
        , _currentChunk{ other._currentChunk }
        , _chunkOffset{ other._chunkOffset }
        , _used{ other._used }
        , _capacity{ other._capacity }
        , _highWaterMark{ other._highWaterMark }
        , _tag{ other._tag } {
        other._chunks.clear();
        other._currentChunk = 0;
        other._chunkOffset = 0;
        other._used = 0;
        other._capacity = 0;
    }

    ArenaAllocator &ArenaAllocator::operator=(ArenaAllocator &&other) noexcept {
        if (this != &other) {
            releaseChunks();

            _chunks = std::move(other._chunks);
            _currentChunk = other._currentChunk;
            _chunkOffset = other._chunkOffset;
            _used = other._used;
            _capacity = other._capacity;
            _highWaterMark = other._highWaterMark;
            _tag = other._tag;

            // releaseChunks reported this arena's own usage away; the adopted chunks were already reported as
            // reserved under the same tag, but their in-use bytes now belong to this object.
            MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(_used));

            other._chunks.clear();
            other._currentChunk = 0;
            other._chunkOffset = 0;
            other._used = 0;
            other._capacity = 0;
        }

        return *this;
    }

    void *ArenaAllocator::Allocate(size_t size, size_t alignment) {
        VASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Arena allocator alignment must be a power of two.");

        const Chunk &chunk = _chunks[_currentChunk];
        size_t offset = AlignUp(reinterpret_cast<size_t>(chunk.Memory) + _chunkOffset, alignment) - reinterpret_cast<size_t>(chunk.Memory);

        if (offset + size > chunk.Capacity) {
            // The current chunk is full. Advance through any chunks retained from an earlier cycle before
            // reserving a new one, so a caller whose demand shrinks stops growing the arena.
            bool placed = false;

            for (size_t next = _currentChunk + 1; next < _chunks.size(); ++next) {
                const Chunk &candidate = _chunks[next];
                const size_t candidateOffset = AlignUp(reinterpret_cast<size_t>(candidate.Memory), alignment) - reinterpret_cast<size_t>(candidate.Memory);

                if (candidateOffset + size <= candidate.Capacity) {
                    _currentChunk = next;
                    offset = candidateOffset;
                    placed = true;
                    break;
                }
            }

            if (!placed) {
                addChunk(size + alignment);
                _currentChunk = _chunks.size() - 1;
                offset = AlignUp(reinterpret_cast<size_t>(_chunks[_currentChunk].Memory), alignment) - reinterpret_cast<size_t>(_chunks[_currentChunk].Memory);
            }
        }

        std::byte *result = _chunks[_currentChunk].Memory + offset;
        _chunkOffset = offset + size;
        _used += size;
        _highWaterMark = std::max(_highWaterMark, _used);

        MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(size));

        return result;
    }

    void ArenaAllocator::Reset() {
        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_used));

        _currentChunk = 0;
        _chunkOffset = 0;
        _used = 0;
    }

    void ArenaAllocator::addChunk(size_t minimumBytes) {
        // Double the previous chunk so a caller that keeps outgrowing the arena converges in a few cycles rather
        // than adding one small chunk at a time.
        const size_t previous = _chunks.empty() ? 0 : _chunks.back().Capacity;
        const size_t capacity = std::max({ minimumBytes, previous * 2, MIN_CHUNK_BYTES });

        // Untracked on purpose: reported below as a reserved pool instead, so the arena's reservation is not
        // conflated with bytes the subsystem has actually handed out. `_chunks` itself is an ordinary tracked
        // container - it is bookkeeping, not pooled storage.
        auto *memory = static_cast<std::byte *>(std::malloc(capacity));
        VASSERT(memory != nullptr, "Arena allocator failed to reserve a chunk.");

        {
            VE_MEMORY_SCOPE(_tag);
            _chunks.push_back(Chunk{ .Memory = memory, .Capacity = capacity });
        }

        _capacity += capacity;

        MemoryTracker::OnPoolReserve(_tag, static_cast<i64>(capacity));
    }

    void ArenaAllocator::releaseChunks() {
        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_used));
        _used = 0;

        for (Chunk &chunk : _chunks) {
            MemoryTracker::OnPoolRelease(_tag, static_cast<i64>(chunk.Capacity));
            std::free(chunk.Memory);
        }

        _chunks.clear();
        _capacity = 0;
    }

} // namespace Vulkyrie
