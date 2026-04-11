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
                _transforms.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
            }

            /** @brief Destructor for TransformComponentStore. */
            ~TransformComponentStore() override = default;

            /** @brief Adds a TransformComponent to the specified entity. Active components are stored at the front of the vector and inactive ones at
             * the back to maintain dense packing for efficient iteration.
             * @param entity The entity to which the TransformComponent will be added. Must not already have a TransformComponent.
             * @param transformComponent The TransformComponent to be added to the entity.
             * @param active Whether the entity is currently active.
             */
            void AddComponent(Entity entity, const TransformComponent &transformComponent, bool active) {
                assert(!HasComponent(entity) && "Entity already has a TransformComponent.");

                size_t index = _transforms.size();

                // Append the new component and its associated entity to the end of their respective vectors.
                _transforms.push_back(transformComponent);
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

            /** @brief Sets the TransformComponent for the specified entity. The entity must already have a TransformComponent associated with it, and this
             * function will update the existing component with the new values provided.
             * @param entity The entity whose TransformComponent is to be updated. The entity must have a TransformComponent.
             * @param transformComponent The new TransformComponent values to be set for the specified entity.
             */
            VE_FORCE_INLINE void SetTransform(const Entity entity, const TransformComponent &transformComponent) {
                assert(HasComponent(entity) && "Entity does not have a TransformComponent.");

                _transforms[_entityToComponentIndex[entity]] = transformComponent;
            }

            /** @brief Retrieves a reference to the TransformComponent associated with the specified entity.
             * @param entity The entity whose TransformComponent is to be retrieved. The entity must have a TransformComponent.
             * @return A reference to the TransformComponent associated with the specified entity.
             */
            VE_FORCE_INLINE TransformComponent &GetTransform(const Entity entity) {
                assert(HasComponent(entity) && "Entity does not have a TransformComponent.");

                return _transforms[_entityToComponentIndex[entity]];
            }

            /** @brief Returns a contiguous view of the active TransformComponents.
             * @return A span over the densely packed active TransformComponents at the front of the storage.
             */
            VE_FORCE_INLINE std::span<const TransformComponent> GetActiveTransforms() const {
                return { _transforms.data(), _activeCount };
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

                std::swap(_transforms[indexA], _transforms[indexB]);
                std::swap(_entities[indexA], _entities[indexB]);

                _entityToComponentIndex[_entities[indexA]] = indexA;
                _entityToComponentIndex[_entities[indexB]] = indexB;
            }

            void removeLastComponentAndEntity() override {
                assert(!_transforms.empty() && "There are no TransformComponents available to be removed.");

                _transforms.pop_back();
                _entities.pop_back();
            }

        private:
            /** @brief A vector that stores the TransformComponents for all entities. The components are densely packed in memory, with active components stored
             * at the beginning of the vector and inactive components stored at the end. This allows for efficient iteration over active components while still
             * supporting inactive entities without fragmentation in memory. The index of a component in this vector corresponds to the index of its associated
             * entity in the _entities vector, allowing for efficient lookup and management of components based on their associated entities. */
            std::vector<TransformComponent> _transforms;
    };

} // namespace Vulkyrie
