#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "memory/allocators/arena_allocator.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    /** @brief Type-keyed storage for data shared between passes, so a pass can publish its outputs (a G-buffer's
     * handles, the shadow atlas) without the graph having to know about them.
     *
     * A blackboard holds a handful of entries, so a flat array with a linear search beats a hash map on every axis
     * that matters here: `unordered_map<type_index, any>` allocated a node per `Set` plus a second allocation
     * whenever the payload outgrew `std::any`'s small buffer - which most pass-data structs do. Entries live in an
     * internal bump arena instead, whose chunked storage keeps references stable as the blackboard grows. */
    class FrameGraphBlackboard final {
    public:
        /** @brief Constructs an empty blackboard.
         * @param arenaBytes Initial size of the internal arena; it grows by chunking if a frame needs more. */
        explicit FrameGraphBlackboard(size_t arenaBytes = 4096)
            : _arena{ arenaBytes, MemoryTag::Rendering } {
            _entries.reserve(INITIAL_ENTRY_CAPACITY);
        }

        ~FrameGraphBlackboard() {
            runDestructors();
        }

        VE_DELETE_COPY(FrameGraphBlackboard);

        FrameGraphBlackboard(FrameGraphBlackboard &&) = default;
        FrameGraphBlackboard &operator=(FrameGraphBlackboard &&) = default;

        /** @brief Stores a value of type T in the blackboard, constructed in place. It must not already exist.
         * @returns A reference to the stored value, valid until the blackboard is cleared or destroyed. */
        template <typename T, typename... TArgs> T &Set(TArgs &&...args) {
            VASSERT(!Contains<T>(), "Blackboard already contains an entry for this type.");

            T *value = _arena.Emplace<T>(std::forward<TArgs>(args)...);

            Entry entry{ .TypeID = FrameGraphTypeID<T>(), .Data = value, .Destroy = nullptr };

            if constexpr (!std::is_trivially_destructible_v<T>) {
                entry.Destroy = [](void *data) { std::destroy_at(static_cast<T *>(data)); };
            }

            _entries.push_back(entry);

            return *value;
        }

        /** @brief Retrieves the value of type T. It must exist in the blackboard. */
        template <typename T> [[nodiscard]] T &Get() {
            return const_cast<T &>(std::as_const(*this).template Get<T>());
        }

        /** @brief Retrieves the value of type T. It must exist in the blackboard. */
        template <typename T> [[nodiscard]] const T &Get() const {
            const T *value = TryGet<T>();

            VASSERT(nullptr != value, "Blackboard does not contain an entry for this type.");

            return *value;
        }

        /** @brief Retrieves a pointer to the value of type T stored in the blackboard, or nullptr if absent. */
        template <typename T> [[nodiscard]] T *TryGet() {
            return const_cast<T *>(std::as_const(*this).template TryGet<T>());
        }

        // TODO: This needs to be re-written, this sucks!
        /** @brief Retrieves a pointer to the value of type T stored in the blackboard, or nullptr if absent. */
        template <typename T> [[nodiscard]] const T *TryGet() const {
            const u16 typeID = FrameGraphTypeID<T>();

            for (const Entry &entry : _entries) {
                if (entry.TypeID == typeID) {
                    return static_cast<const T *>(entry.Data);
                }
            }

            return nullptr;
        }

        /** @brief Checks if the blackboard contains a value of type T. */
        template <typename T> [[nodiscard]] bool Contains() const {
            return TryGet<T>() != nullptr;
        }

        /** @brief Destroys every entry and rewinds the arena, keeping its chunks so the next frame allocates
         * nothing. */
        void Clear() {
            runDestructors();
            _entries.clear();
            _arena.Reset();
        }

        /** @brief Returns the number of entries currently stored. */
        [[nodiscard]] VE_INLINE size_t Size() const {
            return _entries.size();
        }

    private:
        /** @brief Entries expected before the vector needs to grow; a blackboard with more than this is unusual. */
        static constexpr size_t INITIAL_ENTRY_CAPACITY = 16;

        /** @brief One stored value: its type id, its arena address, and how to destroy it. */
        struct Entry {
        public:
            u16 TypeID = 0;
            void *Data = nullptr;
            void (*Destroy)(void *) = nullptr;
        };

        /** @brief Destroys every stored value. The arena storage is released separately. */
        void runDestructors() {
            for (Entry &entry : _entries) {
                if (entry.Destroy != nullptr) {
                    entry.Destroy(entry.Data);
                }
            }
        }

        /** @brief The stored entries, searched linearly. */
        std::vector<Entry> _entries;

        /** @brief Bump storage for the entry payloads; chunked, so references stay valid as it grows. */
        ArenaAllocator _arena;
    };

} // namespace Vulkyrie
