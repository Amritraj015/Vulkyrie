#include "memory/allocators/stack_allocator.h"

#include "core/asserts.h"
#include "memory/memory_tracker.h"

namespace Vulkyrie {

    namespace {

        /** @brief Rounds a pointer-sized value up to the next multiple of a power-of-two alignment. */
        [[nodiscard]] VE_INLINE size_t AlignUp(size_t value, size_t alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

    } // namespace

    StackAllocator::StackAllocator(size_t capacityBytes, MemoryTag tag)
        : _tag{ tag } {
        addChunk(capacityBytes);
    }

    StackAllocator::~StackAllocator() {
        releaseChunks();
    }

    StackAllocator::StackAllocator(StackAllocator &&other) noexcept
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

    StackAllocator &StackAllocator::operator=(StackAllocator &&other) noexcept {
        if (this != &other) {
            releaseChunks();

            _chunks = std::move(other._chunks);
            _currentChunk = other._currentChunk;
            _chunkOffset = other._chunkOffset;
            _used = other._used;
            _capacity = other._capacity;
            _highWaterMark = other._highWaterMark;
            _tag = other._tag;

            MemoryTracker::OnPoolAllocate(_tag, static_cast<i64>(_used));

            other._chunks.clear();
            other._currentChunk = 0;
            other._chunkOffset = 0;
            other._used = 0;
            other._capacity = 0;
        }

        return *this;
    }

    void *StackAllocator::Allocate(size_t size, size_t alignment) {
        VASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, "Stack allocator alignment must be a power of two.");

        const Chunk &chunk = _chunks[_currentChunk];
        size_t offset = AlignUp(reinterpret_cast<size_t>(chunk.Memory) + _chunkOffset, alignment) - reinterpret_cast<size_t>(chunk.Memory);

        if (offset + size > chunk.Capacity) {
            // Move to the next chunk rather than failing. Unlike the arena this never reuses a partially consumed
            // chunk further down the stack: a marker names (chunk, offset), so rewinding has to be able to drop
            // whole chunks in order.
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

    StackMarker StackAllocator::GetMarker() const {
        return StackMarker{ .Chunk = _currentChunk, .Offset = _chunkOffset, .Used = _used };
    }

    void StackAllocator::FreeToMarker(const StackMarker &marker) {
        VASSERT(marker.Chunk < _chunks.size(), "Stack marker refers to a chunk this allocator no longer has.");
        VASSERT(marker.Used <= _used, "Stack marker is ahead of the current top; markers must be released in LIFO order.");

        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_used - marker.Used));

        _currentChunk = marker.Chunk;
        _chunkOffset = marker.Offset;
        _used = marker.Used;
    }

    void StackAllocator::Reset() {
        MemoryTracker::OnPoolFree(_tag, static_cast<i64>(_used));

        _currentChunk = 0;
        _chunkOffset = 0;
        _used = 0;
    }

    void StackAllocator::addChunk(size_t minimumBytes) {
        const size_t previous = _chunks.empty() ? 0 : _chunks.back().Capacity;
        const size_t capacity = std::max({ minimumBytes, previous * 2, MIN_CHUNK_BYTES });

        // Untracked on purpose: reported below through the reserved-pool channel instead.
        auto *memory = static_cast<std::byte *>(std::malloc(capacity));
        VASSERT(memory != nullptr, "Stack allocator failed to reserve a chunk.");

        {
            VE_MEMORY_SCOPE(_tag);
            _chunks.push_back(Chunk{ .Memory = memory, .Capacity = capacity });
        }

        _capacity += capacity;

        MemoryTracker::OnPoolReserve(_tag, static_cast<i64>(capacity));
    }

    void StackAllocator::releaseChunks() {
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
