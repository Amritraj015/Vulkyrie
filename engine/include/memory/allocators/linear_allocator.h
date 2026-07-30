#pragma once

#include "vlkypch.h"
#include <utility>

namespace Vulkyrie {

    /** @brief A bump allocator: allocation is a pointer advance, and the whole arena is freed at once by `Reset`.
     * Individual allocations are never released.
     *
     * Storage is a list of chunks rather than one block, so running out of room widens the arena instead of
     * failing. `Reset` keeps every chunk and rewinds to the first, so a caller that repeatedly fills and resets
     * stops allocating once it reaches its high-water mark: the first cycle grows, every later one reuses.
     *
     * Chunked storage also means allocated pointers stay valid until `Reset` - unlike a `std::vector` arena, a
     * growth does not relocate what was already handed out.
     *
     * Backing chunks are taken untracked and reported to `MemoryTracker` through the reserved-pool channel, so an
     * arena's reservation shows up as memory a subsystem is holding rather than as memory it has handed out. Bytes
     * served from within the arena are reported as pool usage on every allocation, so `PoolUsedBytes` is always
     * live - the same contract every allocator in the toolkit honours.
     *
     * That reporting is the dominant per-allocation cost here, since the allocation itself is only a pointer bump.
     * It is a deliberate trade: uniform, always-accurate accounting across the toolkit is worth more than shaving
     * an atomic off an operation that is already close to free.
     *
     * Not thread-safe: one arena belongs to one thread (or is externally synchronized). */
    class LinearAllocator final {
    public:
        /** @brief Constructs an arena with an initial chunk of the requested size.
         * @param capacityBytes Size of the first chunk in bytes; rounded up to at least `MIN_CHUNK_BYTES`.
         * @param tag Memory tag the backing chunks are attributed to. */
        explicit LinearAllocator(size_t capacityBytes, MemoryTag tag = MemoryTag::Untagged);

        ~LinearAllocator();

        VE_DELETE_COPY(LinearAllocator);

        LinearAllocator(LinearAllocator &&other) noexcept;
        LinearAllocator &operator=(LinearAllocator &&other) noexcept;

        /** @brief Carves an aligned block out of the arena, growing it with a new chunk if the current one is full.
         * @param size Bytes to allocate. Zero returns a non-null, non-dereferenceable pointer.
         * @param alignment Required alignment in bytes; must be a power of two.
         * @returns Pointer to uninitialized storage, valid until the next `Reset`. */
        [[nodiscard]] void *Allocate(size_t size, size_t alignment);

        /** @brief Carves storage for `count` objects of type `T`, correctly aligned but not constructed.
         * @tparam T The object type; only its size and alignment are used.
         * @param count Number of objects; defaults to one.
         * @returns Pointer to uninitialized storage, valid until the next `Reset`. */
        template <typename T> [[nodiscard]] T *Allocate(size_t count = 1) {
            return static_cast<T *>(Allocate(sizeof(T) * count, alignof(T)));
        }

        /** @brief Carves storage for one `T` and constructs it in place.
         * @tparam T The object type.
         * @tparam TArgs Constructor argument types.
         * @param args Arguments forwarded to `T`'s constructor.
         * @returns Pointer to the constructed object, valid until the next `Reset`.         *
         * Nothing here runs the destructor: `Reset` rewinds without destroying, so a non-trivially-destructible `T`
         * has to be destroyed by whoever emplaced it. That is a deliberate cost of a bump allocator, not an
         * oversight - the frame graph, for instance, destroys its arena-held pass payloads through a stored
         * function pointer. */
        template <typename T, typename... TArgs> [[nodiscard]] T *Emplace(TArgs &&...args) {
            return std::construct_at(Allocate<T>(), std::forward<TArgs>(args)...);
        }

        /** @brief Releases every allocation at once by rewinding to the first chunk. Chunks are retained, so this
         * performs no deallocation and the next cycle reuses the same memory. Does not run destructors - the caller
         * is responsible for destroying non-trivial objects it constructed in the arena. */
        void Reset();

        /** @brief Returns the bytes handed out since the last `Reset`. */
        [[nodiscard]] VE_INLINE size_t Used() const {
            return _used;
        }

        /** @brief Returns the total bytes across all retained chunks. */
        [[nodiscard]] VE_INLINE size_t Capacity() const {
            return _capacity;
        }

        /** @brief Returns the largest `Used()` ever reached, i.e. the size a single chunk would have needed. */
        [[nodiscard]] VE_INLINE size_t HighWaterMark() const {
            return _highWaterMark;
        }

        /** @brief Returns the number of chunks currently retained. More than one means some cycle outgrew the
         * initial capacity; the arena is allocation-free again once it stops growing. */
        [[nodiscard]] VE_INLINE size_t ChunkCount() const {
            return _chunks.size();
        }

        /** @brief Returns the subsystem this arena's memory is attributed to. */
        [[nodiscard]] VE_INLINE MemoryTag GetTag() const {
            return _tag;
        }

    private:
        /** @brief Smallest chunk the arena will allocate, so a tiny requested capacity does not degrade into a
         * chunk per allocation. */
        static constexpr size_t MIN_CHUNK_BYTES = 4096;

        /** @brief One contiguous span of arena storage. */
        struct Chunk {
        public:
            std::byte *Memory = nullptr;
            size_t Capacity = 0;
        };

        /** @brief Appends a chunk of at least `minimumBytes`, doubling the last chunk's size to amortize growth.
         * @param minimumBytes The allocation that did not fit, which the new chunk must be able to satisfy. */
        void addChunk(size_t minimumBytes);

        /** @brief Frees every chunk. */
        void releaseChunks();

        /** @brief Retained storage, in allocation order. */
        std::vector<Chunk> _chunks;

        /** @brief Index of the chunk currently being bumped. */
        size_t _currentChunk = 0;

        /** @brief Bytes used within the current chunk. */
        size_t _chunkOffset = 0;

        /** @brief Bytes handed out since the last `Reset`, across all chunks. */
        size_t _used = 0;

        /** @brief Total bytes across all retained chunks. */
        size_t _capacity = 0;

        /** @brief Largest `_used` ever observed. */
        size_t _highWaterMark = 0;

        /** @brief Memory tag the chunks are attributed to. */
        MemoryTag _tag = MemoryTag::Untagged;
    };

} // namespace Vulkyrie
