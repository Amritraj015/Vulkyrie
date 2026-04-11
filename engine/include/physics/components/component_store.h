#pragma once

#include "core/entity.h"

namespace Vulkyrie {

    class ComponentStore {
        protected:
            /** @brief Constructs an instance of ComponentStore. Initializes the active component count to zero. */
            ComponentStore()
                : _activeCount(0) {
                _entities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
                _entityToComponentIndex.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
            }

            /** @brief Virtual destructor for ComponentStore. Ensures proper cleanup of derived classes. */
            virtual ~ComponentStore() = default;

            /** @brief Initial reservation count for component storage vectors. */
            static constexpr u16 INITIAL_COMPONENT_RESERVATION_COUNT = 100;

            /** @brief A parallel vector to _components that stores the corresponding entities for each component. The index of an entity in this
             * vector corresponds to the index of its associated component in the component store vector. This allows for efficient lookup and management
             * of components based on their associated entities while maintaining the dense packing of active components in memory. */
            std::vector<Entity> _entities;

            /** @brief Total count of active entities. */
            size_t _activeCount;

            /** @brief A hash map that maps each entity to the index of its associated component in the component store vector. This allows for efficient
             * lookup of components based on their associated entities, enabling quick access and management of components without needing to search through the
             * entire component vector. The map is updated whenever components are added, removed, activated, or deactivated to ensure that it always reflects
             * the current state of the component storage. */
            std::unordered_map<Entity, size_t> _entityToComponentIndex;

            /** @brief Swaps the components at the specified indices in the component vector. This is used to maintain the dense packing of active components in
             * memory when an entity becomes inactive or when components are removed. The method also updates the corresponding entries in the _entities vector
             * and the _entityToComponentIndex map to ensure that they remain consistent with the new positions of the components after the swap.
             * @param indexA The index of the first component to be swapped. Must be a valid index within the component vector.
             * @param indexB The index of the second component to be swapped. Must be a valid index within the component vector.
             */
            virtual void swapComponents(size_t indexA, size_t indexB) = 0;

            /** @brief Removes the last component from the component and entity vector. This is called after a component has been swapped to the end of the
             * vector for removal. The method pops the last component off the vector and also removes the corresponding entity from the
             * _entities vector. It is important that this method is called after any necessary swaps have been made to ensure that the correct component is
             * removed and that the dense packing of active components is maintained.
             */
            virtual void removeLastComponentAndEntity() = 0;

        public:
            /** @brief Removes the component associated with the specified entity. If the entity's component is active, it is first swapped with the
             * last active component to maintain the dense packing of active components. The component is then swapped to the end of the vector and popped off.
             * @param entity The entity whose component is to be removed. The entity must have a component.
             */
            void RemoveComponent(Entity entity) {
                assert(HasComponent(entity) && "Entity does not have a component.");

                size_t index = _entityToComponentIndex[entity];
                bool wasActive = index < _activeCount;
                size_t lastIndex = _entities.size() - 1;

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

                // Pop the last component off the vector and remove the corresponding entity from the _entities vector.
                removeLastComponentAndEntity();

                // Remove the entity from the _entityToComponentIndex map since it no longer has an associated component.
                _entityToComponentIndex.erase(entity);
            }

            /** @brief Checks if the specified entity is currently disabled in this component store.
             * @param entity The entity to check for being disabled.
             * @return True if the entity has a component that is currently inactive, false otherwise.
             */
            VE_FORCE_INLINE bool IsDisabled(Entity entity) {
                assert(HasComponent(entity) && "Entity does not have a component.");

                return _entityToComponentIndex[entity] >= _activeCount;
            }

            /** @brief Checks if the specified entity has a component associated with it in this component store.
             * @param entity The entity to check for having a component.
             * @return True if the entity has a component associated with it, false otherwise.
             */
            VE_FORCE_INLINE bool HasComponent(Entity entity) const {
                return _entityToComponentIndex.contains(entity);
            }

            /** @brief Gets the total number of components (active + inactive) currently managed.
             * @return The total number of entities that have a component associated with them.
             */
            VE_FORCE_INLINE size_t GetTotalComponentCount() const {
                return _entities.size();
            }

            /** @brief Gets the total number of active components currently managed by this component store.
             * @return The total number of entities that currently have an active component associated with them.
             */
            VE_FORCE_INLINE size_t GetActiveComponentCount() const {
                return _activeCount;
            }

            /** @brief Retrieves the index of the component in the component vector associated with the specified entity. The entity must have a component
             * associated with it.
             * @param entity The entity whose component index is to be retrieved. The entity must have a component associated with it.
             * @return The index of the component associated with the specified entity in the component vector.
             */
            VE_FORCE_INLINE size_t GetEntityIndex(Entity entity) const {
                assert(HasComponent(entity) && "Entity does not have a component.");

                return _entityToComponentIndex.at(entity);
            }

            // void Enabled(Entity entity, bool enabled);
            //
            // bool HasComponentGetIndex(Entity entity, uint32 &entityIndex) const;
    };

} // namespace Vulkyrie
