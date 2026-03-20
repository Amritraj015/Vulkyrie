#pragma once

#include "vlkypch.h"
#include "ecs/entity.h"

namespace Vulkyrie::ECS {

    class EntityManager {
        public:
            /** @brief Creates a new entity and returns it.
             * @return The newly created entity.
             */
            [[nodiscard]] Entity CreateEntity();

            /** @brief Destroys the specified entity, making it inactive and available for reuse.
             * @param entity The entity to be destroyed.
             */
            void DestroyEntity(const Entity entity);

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
            std::vector<u8> _generations;

            /// TODO: Probably needs to be a deque.
            std::vector<u32> _freeIndices;
    };

} // namespace Vulkyrie::ECS
