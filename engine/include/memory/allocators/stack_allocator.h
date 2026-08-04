#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief A position in a `StackAllocator`, taken before a batch of allocations and used to release all of
     * them at once. Opaque; only meaningful to the allocator that produced it. */
    struct StackMarker {
    public:
        size_t Chunk = 0;  ///< Index of the chunk that was current.
        size_t Offset = 0; ///< Bump offset within that chunk.
        size_t Used = 0;   ///< Total bytes outstanding at the time the marker was taken.
    };

    /** @brief A bump allocator that can also unwind, in LIFO order.
     *
     * The difference from `ArenaAllocator` is the middle ground it offers: an arena can only release everything,
     * a general allocator can release anything, and a stack releases everything back to a marker. That fits
     * scoped scratch work - a subsystem takes a marker on entry, allocates freely, and rewinds on exit without
     * having to track individual allocations.
     *
     * Storage grows by chunking rather than failing, and chunks are retained across a rewind, so a stack that
     * reaches its high-water mark stops allocating. Markers survive growth: they name a chunk and an offset within
     * it, not a raw pointer.
     *
     * Backing chunks are taken untracked and reported through `MemoryTracker`'s reserved-pool channel.
     *
     * Not thread-safe: one stack belongs to one thread (or is externally synchronized). */
    class StackAllocator final {
    public:
        /** @brief Constructs a stack with an initial chunk.
         * @param capacityBytes Size of the first chunk in bytes; rounded up to at least `MIN_CHUNK_BYTES`.
         * @param tag Subsystem the reservation is attributed to; required, see `ArenaAllocator`. */
        StackAllocator(size_t capacityBytes, MemoryTag tag);

        ~StackAllocator();

        VE_DELETE_COPY(StackAllocator);

        StackAllocator(StackAllocator &&other) noexcept;
        StackAllocator &operator=(StackAllocator &&other) noexcept;

        /** @brief Carves an aligned block off the top of the stack.
         * @param size Bytes to allocate.
         * @param alignment Required alignment in bytes; must be a power of two.
         * @returns Pointer to uninitialized storage, valid until the stack is rewound past it. */
        [[nodiscard]] void *Allocate(size_t size, size_t alignment);

        /** @brief Carves storage for `count` objects of type `T`, correctly aligned but not constructed.
         * @tparam T The object type; only its size and alignment are used.
         * @param count Number of objects; defaults to one.
         * @returns Pointer to uninitialized storage, valid until the stack is rewound past it. */
        template <typename T> [[nodiscard]] T *AllocateArray(size_t count = 1) {
            return static_cast<T *>(Allocate(sizeof(T) * count, alignof(T)));
        }

        /** @brief Carves storage for one `T` and constructs it in place.
         * @tparam T The object type.
         * @tparam TArgs Constructor argument types.
         * @param args Arguments forwarded to `T`'s constructor.
         * @returns Pointer to the constructed object, valid until the stack is rewound past it.
         *
         * Nothing here runs the destructor: `FreeToMarker` and `Reset` rewind without destroying, so a
         * non-trivially-destructible `T` has to be destroyed by whoever emplaced it. */
        template <typename T, typename... TArgs> [[nodiscard]] T *Emplace(TArgs &&...args) {
            return std::construct_at(AllocateArray<T>(), std::forward<TArgs>(args)...);
        }

        /** @brief Returns the current top of the stack, for a later `FreeToMarker`. */
        [[nodiscard]] StackMarker GetMarker() const;

        /** @brief Releases everything allocated since a marker was taken. Does not run destructors - the caller
         * owns the lifetime of anything non-trivial it built in the released region.
         * @param marker A marker previously returned by `GetMarker` on this allocator. */
        void FreeToMarker(const StackMarker &marker);

        /** @brief Releases everything, keeping the chunks. */
        void Reset();

        /** @brief Returns the bytes currently outstanding. */
        [[nodiscard]] VE_INLINE size_t Used() const {
            return _used;
        }

        /** @brief Returns the total bytes across all retained chunks. */
        [[nodiscard]] VE_INLINE size_t Capacity() const {
            return _capacity;
        }

        /** @brief Returns the largest `Used()` ever reached. */
        [[nodiscard]] VE_INLINE size_t HighWaterMark() const {
            return _highWaterMark;
        }

        /** @brief Returns the number of backing chunks. */
        [[nodiscard]] VE_INLINE size_t ChunkCount() const {
            return _chunks.size();
        }


    private:
        /** @brief Smallest chunk the stack will reserve. */
        static constexpr size_t MIN_CHUNK_BYTES = 4096;

        /** @brief One contiguous span of stack storage. */
        struct Chunk {
        public:
            std::byte *Memory = nullptr;
            size_t Capacity = 0;
        };

        /** @brief Appends a chunk able to satisfy `minimumBytes`. */
        void addChunk(size_t minimumBytes);

        /** @brief Frees every chunk. */
        void releaseChunks();

        /** @brief Retained storage, in allocation order. */
        std::vector<Chunk> _chunks;

        /** @brief Index of the chunk currently being bumped. */
        size_t _currentChunk = 0;

        /** @brief Bytes used within the current chunk. */
        size_t _chunkOffset = 0;

        /** @brief Bytes outstanding across all chunks. */
        size_t _used = 0;

        /** @brief Total bytes across all retained chunks. */
        size_t _capacity = 0;

        /** @brief Largest `_used` ever observed. */
        size_t _highWaterMark = 0;

        /** @brief Memory tag the chunks are attributed to. */
        MemoryTag _tag;
    };

    /** @brief RAII rewind: takes a marker on construction and returns the stack to it on destruction.
     *
     * Written as a guard rather than a trailing `FreeToMarker` call so an early return or a throw cannot leave the
     * stack permanently grown. */
    class StackAllocatorScope final {
    public:
        /** @brief Takes a marker on `allocator`.
         * @param allocator The stack to rewind on destruction. */
        explicit StackAllocatorScope(StackAllocator &allocator)
            : _allocator{ allocator }
            , _marker{ allocator.GetMarker() } {
        }

        ~StackAllocatorScope() {
            _allocator.FreeToMarker(_marker);
        }

        VE_DELETE_MOVE_AND_COPY(StackAllocatorScope);

    private:
        StackAllocator &_allocator;
        StackMarker _marker;
    };

} // namespace Vulkyrie
