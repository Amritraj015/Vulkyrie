#pragma once

#include "core/entity.h"

namespace Vulkyrie {

    /** @brief The EntityManager is responsible for generating unique entity identifiers, recycling destroyed entities, and keeping track of active entities. It
     * uses a combination of an index and generation system to ensure that entity identifiers are unique and can be safely reused after destruction. */
    class EntityManager {
        private:
            constexpr static size_t MINIMUM_FREE_INDICES = 2048;

        public:
            EntityManager() = default;

            /** @brief Creates a new entity and returns it.
             * @return The newly created entity.
             */
            [[nodiscard]] Entity CreateEntity() {
                u64 index;

                // If there are free indices available, we can reuse one of them.
                if (_freeIndices.size() > MINIMUM_FREE_INDICES) {
                    // Get the index from the front of the deque of free indices and remove it from the deque.
                    index = _freeIndices.front();
                    _freeIndices.pop_front();
                } else {
                    // The index for the new entity is the current size of the vector of generations, which represents the next available index.
                    index = static_cast<u64>(_generations.size());

                    // If there are no free indices available,
                    // we need to create a new one by adding a new generation entry to the vector of generations.
                    _generations.push_back(0);

                    // Assert that the index does not exceed the maximum allowed by the entity ID.
                    assert(index < (u64(1) << Entity::ENTITY_INDEX_BITS));
                }

                // Return a newly created entity.
                return Entity(index, _generations[index]);
            }

            /** @brief Destroys the specified entity, making it inactive and available for reuse.
             * @param entity The entity to be destroyed.
             */
            void DestroyEntity(const Entity entity) {
                // Get the index of the entity to be destroyed.
                const u64 index = entity.GetIndex();

                // Increment the generation of the entity index to invalidate any existing entities with the same index and make it available for reuse.
                _generations[index]++;

                // Add the index of the destroyed entity to the deque of free indices for future reuse.
                _freeIndices.push_back(index);
            }

            /** @brief Checks if the specified entity is active (i.e., has been created and not destroyed).
             * @param entity The entity to check for activity.
             * @return True if the entity is active, false otherwise.
             */
            [[nodiscard]] VE_FORCE_INLINE bool IsActive(const Entity entity) const {
                return _generations[entity.GetIndex()] == entity.GetGeneration();
            }

        private:
            /** @brief Vector storing the generation of each entity index.
             * The generation is used to determine if an entity is active or has been destroyed and potentially reused. */
            std::vector<u16> _generations;

            /// TODO: This is not a very good option, if the deque of free indices grows too large, it could lead to memory issues. Consider implementing a
            /// more efficient data structure for managing free indices if necessary.
            std::deque<u64> _freeIndices;
    };

} // namespace Vulkyrie
