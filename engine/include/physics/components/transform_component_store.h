#pragma once

#include "core/entity.h"

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
    class TransformComponentStore final {
        public:
            /** @brief Constructs an instance of TransformComponentStore. */
            TransformComponentStore()
                : _activeCount(0) {
            }

            /** @brief Adds a TransformComponent to the specified entity. Active components are stored at the front of the vector and inactive ones at
             * the back to maintain dense packing for efficient iteration.
             * @param entity The entity to which the TransformComponent will be added. Must not already have a TransformComponent.
             * @param component The TransformComponent to be added to the entity.
             * @param active Whether the entity is currently active.
             */
            void AddComponent(Entity entity, const TransformComponent &component, bool active) {
                assert(!_entityToComponentIndex.contains(entity));

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

            /** @brief Removes the TransformComponent associated with the specified entity. If the entity's component is active, it is first swapped with the
             * last active component to maintain the dense packing of active components. The component is then swapped to the end of the vector and popped off.
             * @param entity The entity whose TransformComponent is to be removed. The entity must have a TransformComponent.
             */
            void RemoveComponent(Entity entity) {
                assert(_entityToComponentIndex.contains(entity) && "Entity does not have a TransformComponent.");

                size_t index = _entityToComponentIndex[entity];
                bool wasActive = index < _activeCount;
                size_t lastIndex = _components.size() - 1;

                if (wasActive) {
                    // Swap the removed component with the last active component to fill the gap in the active zone,
                    // then update index to point to where the removed component now sits (at the active/inactive boundary).
                    size_t lastActiveIndex = _activeCount - 1;
                    if (index != lastActiveIndex) {
                        swapComponents(index, lastActiveIndex);
                        index = lastActiveIndex;
                    }

                    // Shrink the active zone since the removed component is no longer active.
                    _activeCount--;
                }

                // Swap the removed component to the very end of the vector so it can be popped off.
                if (index != lastIndex) {
                    swapComponents(index, lastIndex);
                }

                _components.pop_back();
                _entities.pop_back();
                _entityToComponentIndex.erase(entity);
            }

            /** @brief Activates the TransformComponent associated with the specified entity, making it active and included in the count of active components.
             * The entity must already have a TransformComponent associated with it, and this function will move the component to the appropriate index in the
             * component vector to maintain the dense packing of active components.
             * @param entity The entity whose TransformComponent is to be activated. The entity must have a TransformComponent.
             */
            void Activate(Entity entity) {
                assert(_entityToComponentIndex.contains(entity) && "Entity does not have a TransformComponent.");

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
                assert(_entityToComponentIndex.contains(entity) && "Entity does not have a TransformComponent.");

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
            VE_FORCE_INLINE void SetComponent(const Entity entity, const TransformComponent &component) {
                assert(_entityToComponentIndex.contains(entity) && "Entity does not have a TransformComponent.");

                _components[_entityToComponentIndex[entity]] = component;
            }

            /** @brief Retrieves a reference to the TransformComponent associated with the specified entity.
             * @param entity The entity whose TransformComponent is to be retrieved. The entity must have a TransformComponent.
             * @return A reference to the TransformComponent associated with the specified entity.
             */
            VE_FORCE_INLINE TransformComponent &GetTransform(const Entity entity) {
                assert(_entityToComponentIndex.contains(entity) && "Entity does not have a TransformComponent.");

                return _components[_entityToComponentIndex[entity]];
            }

            /** @brief Gets the total number of active transform components currently managed by this TransformComponentStore.
             * @return The total number of entities that currently have an active TransformComponent associated with them.
             */
            VE_FORCE_INLINE size_t GetActiveComponentCount() const {
                return _activeCount;
            }

            /** @brief Gets the total number of transform components (active + inactive) currently managed.
             * @return The total number of entities that have a TransformComponent associated with them.
             */
            VE_FORCE_INLINE size_t GetTotalComponentCount() const {
                return _components.size();
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

        private:
            /** @brief A vector that stores the TransformComponents for all entities. The components are densely packed in memory, with active components stored
             * at the beginning of the vector and inactive components stored at the end. This allows for efficient iteration over active components while still
             * supporting inactive entities without fragmentation in memory. The index of a component in this vector corresponds to the index of its associated
             * entity in the _entities vector, allowing for efficient lookup and management of components based on their associated entities. */
            std::vector<TransformComponent> _components;

            /** @brief A parallel vector to _components that stores the corresponding entities for each TransformComponent. The index of an entity in this
             * vector corresponds to the index of its associated TransformComponent in the _components vector. This allows for efficient lookup and management
             * of components based on their associated entities while maintaining the dense packing of active components in memory. */
            std::vector<Entity> _entities;

            /** @brief Total count of active entities. */
            size_t _activeCount;

            /** @brief A hash map that maps each entity to the index of its associated TransformComponent in the _components vector. This allows for efficient
             * lookup of components based on their associated entities, enabling quick access and management of components without needing to search through the
             * entire component vector. The map is updated whenever components are added, removed, activated, or deactivated to ensure that it always reflects
             * the current state of the component storage. */
            std::unordered_map<Entity, size_t> _entityToComponentIndex;

            /** @brief Swaps the TransformComponents at the specified indices in the component vector. This is used to maintain the dense packing of active
             * components in memory when an entity becomes inactive or when components are removed.
             * @param indexA The index of the first TransformComponent to be swapped. Must be a valid index within the component vector.
             * @param indexB The index of the second TransformComponent to be swapped. Must be a valid index within the component vector.
             */
            void swapComponents(size_t indexA, size_t indexB) {
                if (indexA == indexB) return;

                std::swap(_components[indexA], _components[indexB]);
                std::swap(_entities[indexA], _entities[indexB]);

                _entityToComponentIndex[_entities[indexA]] = indexA;
                _entityToComponentIndex[_entities[indexB]] = indexB;
            }
    };

} // namespace Vulkyrie
