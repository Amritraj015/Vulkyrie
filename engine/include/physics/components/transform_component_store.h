#pragma once

#include "core/entity.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    /** @brief Component that represents the position, rotation, and scale of an entity in 3D space. */
    struct TransformComponent final {
        public:
            /** Position of the entity in 3D space. */
            glm::vec3 Position;

            /** Rotation of the entity represented as a quaternion. */
            glm::quat Rotation;

            /** Scale factor for the entity in 3D space. */
            glm::vec3 Scale;
    };

    /** @brief The TransformComponentStore is responsible for managing TransformComponents associated with entities. It maintains a dense packing of active
     * components at the front of the storage vector for efficient iteration, while allowing for dynamic addition, removal, activation, and deactivation of
     * components without fragmentation. The manager uses a mapping from entities to component indices to enable fast lookups and updates. */
    class TransformComponentStore final : public ComponentStore {
        public:
            /** @brief Constructs an instance of TransformComponentStore. */
            TransformComponentStore() {
                _components.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
            }

            /** @brief Destructor for TransformComponentStore. */
            ~TransformComponentStore() override = default;

            /** @brief Adds a TransformComponent to the specified entity. Active components are stored at the front of the vector and inactive ones at
             * the back to maintain dense packing for efficient iteration.
             * @param entity The entity to which the TransformComponent will be added. Must not already have a TransformComponent.
             * @param component The TransformComponent to be added to the entity.
             * @param active Whether the entity is currently active.
             */
            void AddComponent(Entity entity, const TransformComponent &component, bool active) {
                assert(!HasComponent(entity) && "Entity already has a TransformComponent.");

                size_t index = _components.size();

                // Append the new component and its associated entity to the end of their respective vectors.
                _components.push_back(component);
                _entities.push_back(entity);

                if (active) {
                    // Swap the new component into the active zone if inactive components sit between it and _activeCount.
                    // swapComponents updates _entityToComponentIndex for both swapped entries;
                    // otherwise we set the mapping ourselves since no swap is needed.
                    if (index != _activeCount) {
                        swapComponents(index, _activeCount);
                    } else {
                        _entityToComponentIndex[entity] = index;
                    }

                    // Grow the active zone to include the newly added component.
                    _activeCount++;
                } else {
                    // Inactive components stay at the end; just record the mapping.
                    _entityToComponentIndex[entity] = index;
                }
            }

            /** @brief Activates the TransformComponent associated with the specified entity, making it active and included in the count of active components.
             * The entity must already have a TransformComponent associated with it, and this function will move the component to the appropriate index in the
             * component vector to maintain the dense packing of active components.
             * @param entity The entity whose TransformComponent is to be activated. The entity must have a TransformComponent.
             */
            void Activate(Entity entity) {
                assert(HasComponent(entity) && "Entity does not have a TransformComponent.");

                size_t index = _entityToComponentIndex[entity];

                // If entity is already active, we don't need to do anything.
                if (index < _activeCount) return;

                // Swap the component at the current index with the component at the index corresponding to
                // the current count of active components to maintain the dense packing of active components.
                swapComponents(index, _activeCount);

                // Increment the count of active components since we have activated a new component.
                _activeCount++;
            }

            /** @brief Deactivates the TransformComponent associated with the specified entity. The entity must already have a TransformComponent associated
             * with it, and this function will mark the component as inactive by swapping it with the last active component in the vector and decrementing the
             * count of active components. This maintains the dense packing of active components in memory while allowing for efficient deactivation of
             * components without fragmentation.
             * @param entity The entity whose TransformComponent is to be deactivated. The entity must have a TransformComponent.
             */
            void Deactivate(Entity entity) {
                assert(HasComponent(entity) && "Entity does not have a TransformComponent.");

                size_t index = _entityToComponentIndex[entity];

                // If entity is already inactive, we don't need to do anything.
                if (index >= _activeCount) return;

                // Decrement the count of active components since we will be deactivating a component.
                _activeCount--;

                // Swap the component at the current index with the component at the index corresponding to
                // the new count of active components to maintain the dense packing of active components.
                swapComponents(index, _activeCount);
            }

            /** @brief Sets the TransformComponent for the specified entity. The entity must already have a TransformComponent associated with it, and this
             * function will update the existing component with the new values provided.
             * @param entity The entity whose TransformComponent is to be updated. The entity must have a TransformComponent.
             * @param component The new TransformComponent values to be set for the specified entity.
             */
            VE_FORCE_INLINE void SetTransform(const Entity entity, const TransformComponent &component) {
                assert(HasComponent(entity) && "Entity does not have a TransformComponent.");

                _components[_entityToComponentIndex[entity]] = component;
            }

            /** @brief Retrieves a reference to the TransformComponent associated with the specified entity.
             * @param entity The entity whose TransformComponent is to be retrieved. The entity must have a TransformComponent.
             * @return A reference to the TransformComponent associated with the specified entity.
             */
            VE_FORCE_INLINE TransformComponent &GetTransform(const Entity entity) {
                assert(HasComponent(entity) && "Entity does not have a TransformComponent.");

                return _components[_entityToComponentIndex[entity]];
            }

            /** @brief Returns a contiguous view of the active TransformComponents.
             * @return A span over the densely packed active TransformComponents at the front of the storage.
             */
            VE_FORCE_INLINE std::span<const TransformComponent> GetActiveTransforms() const {
                return { _components.data(), _activeCount };
            }

            /** @brief Returns a contiguous view of the entities that have active TransformComponents.
             * @return A span over the entities corresponding to the densely packed active TransformComponents.
             */
            VE_FORCE_INLINE std::span<const Entity> GetActiveEntities() const {
                return { _entities.data(), _activeCount };
            }

        protected:
            void swapComponents(size_t indexA, size_t indexB) override {
                if (indexA == indexB) return;

                std::swap(_components[indexA], _components[indexB]);
                std::swap(_entities[indexA], _entities[indexB]);

                _entityToComponentIndex[_entities[indexA]] = indexA;
                _entityToComponentIndex[_entities[indexB]] = indexB;
            }

            void removeLastComponentAndEntity() override {
                assert(!_components.empty() && "No components to remove.");

                _components.pop_back();
                _entities.pop_back();
            }

        private:
            /** @brief A vector that stores the TransformComponents for all entities. The components are densely packed in memory, with active components stored
             * at the beginning of the vector and inactive components stored at the end. This allows for efficient iteration over active components while still
             * supporting inactive entities without fragmentation in memory. The index of a component in this vector corresponds to the index of its associated
             * entity in the _entities vector, allowing for efficient lookup and management of components based on their associated entities. */
            std::vector<TransformComponent> _components;
    };

} // namespace Vulkyrie
