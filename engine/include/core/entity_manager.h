#pragma once

#include "core/entity.h"

namespace Vulkyrie {

    /** @brief The EntityManager is responsible for generating unique entity identifiers, recycling destroyed entities, and keeping track of active entities. It
     * uses a combination of an index and generation system to ensure that entity identifiers are unique and can be safely reused after destruction. */
    class EntityManager final {
    public:
        /** @brief Constructs an instance of EntityManager. */
        EntityManager() = default;

        // Delete the copy constructor and copy assignment operator.
        EntityManager(const EntityManager &) = delete;
        EntityManager &operator=(const EntityManager &) = delete;

        // Delete the move constructor and move assignment operator.
        EntityManager(EntityManager &&) = delete;
        EntityManager &operator=(EntityManager &&) = delete;

        /** @brief Creates a new entity and returns it.
         * @returns The newly created entity.
         */
        [[nodiscard]] Entity CreateEntity();

        /** @brief Destroys the specified entity, making it inactive and available for reuse.
         * @param entity The entity to be destroyed.
         */
        void DestroyEntity(const Entity entity);

        /** @brief Checks if the specified entity is active (i.e., has been created and not destroyed).
         * @param entity The entity to check for activity.
         * @returns True if the entity is active, false otherwise.
         */
        [[nodiscard]] VE_INLINE bool IsActive(const Entity entity) const {
            return _generations[static_cast<size_t>(entity.GetIndex())] == entity.GetGeneration();
        }

    private:
        /** @brief The minimum number of free indices to keep in the deque before reusing them. This threshold helps to reduce fragmentation and improve
         * cache locality by allowing some reuse of recently destroyed entities before recycling older ones. Adjust this value based on the expected entity
         * creation/destruction patterns in your application for optimal performance. */
        constexpr static size_t MINIMUM_FREE_INDICES = 2048;

        /** @brief Vector storing the generation of each entity index.
         * The generation is used to determine if an entity is active or has been destroyed and potentially reused. */
        std::vector<u16> _generations;

        /// TODO: This is not a very good option, if the deque of free indices grows too large, it could lead to memory issues. Consider implementing a
        /// more efficient data structure for managing free indices if necessary.
        std::deque<size_t> _freeIndices;
    };

} // namespace Vulkyrie
