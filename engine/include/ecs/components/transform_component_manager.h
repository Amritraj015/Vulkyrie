#pragma once

#include "ecs/entity.h"
#include "ecs/entity_manager.h"
#include "ecs/components/transform_component.h"

namespace Vulkyrie::ECS {

    class TransformComponentManager final {
        public:
            /** @brief Constructs a TransformComponentManager with a reference to the EntityManager.
             * @param entityManager A reference to the EntityManager used to manage entities and their activity status. The TransformComponentManager relies on
             * the EntityManager to ensure that components are only added to active entities.
             */
            TransformComponentManager(const EntityManager &entityManager)
                : _entityManager(entityManager) {};

            /** @brief Adds a TransformComponent to the specified entity.
             * @param entity The entity to which the TransformComponent will be added. The entity must be valid and active.
             * @param component The TransformComponent to be added to the entity.
             */
            void AddComponent(const Entity entity, TransformComponent component) {
                assert(_entityManager.IsActive(entity) && "Cannot add component to an inactive entity.");

                // TODO: Figure out what to do when the entity is in-active.
                _components.push_back(component);
            }

            void SetComponent(const Entity entity, TransformComponent component) {
                assert(_entityManager.IsActive(entity) && "Cannot set component for an inactive entity.");

                _components[entity.GetIndex()] = component;
            }

        private:
            /** @brief A vector of TransformComponents for entities. */
            std::vector<TransformComponent> _components;

            /** @brief A reference to EntityManager. */
            const EntityManager &_entityManager;
    };

} // namespace Vulkyrie::ECS
