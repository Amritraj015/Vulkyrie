#pragma once

#include "core/entity.h"
#include "core/asserts.h"

namespace Vulkyrie {

    /** @brief Base class for component stores that manage components associated with entities. The ComponentStore class provides common functionality for
     * managing the active status of components, as well as efficient storage and lookup of components based on their associated entities. Derived classes are
     * expected to implement the specific storage and management of their respective component types, while leveraging the common functionality provided by this
     * base class to maintain a consistent and efficient approach to component management across different types of components in the ECS architecture. */
    class ComponentStore {
        protected:
            /** @brief Constructs an instance of ComponentStore. Initializes the active component count to zero. */
            ComponentStore();

            // Delete the copy constructor and copy assignment operator.
            ComponentStore(const ComponentStore &) = delete;
            ComponentStore &operator=(const ComponentStore &) = delete;

            // Delete the move constructor and move assignment operator.
            ComponentStore(ComponentStore &&) = delete;
            ComponentStore &operator=(ComponentStore &&) = delete;

            /** @brief Virtual destructor for ComponentStore. Ensures proper cleanup of derived classes. */
            virtual ~ComponentStore() = default;

            /** @brief Initial reservation count for component storage vectors. */
            static constexpr u16 INITIAL_COMPONENT_RESERVATION_COUNT = 128;

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
            /** @brief Sets the active status of the component associated with the specified entity. If the entity is being activated, its component is swapped
             * into the active zone at the front of the vector. If the entity is being deactivated, its component is swapped out of the active zone to maintain
             * dense packing of active components. The method updates the _activeCount accordingly to reflect the new count of active components.
             * @param entity The entity whose component's active status is to be set. The entity must have a component associated with it.
             * @param active True to activate the entity's component, false to deactivate it.
             */
            void SetActiveStatus(Entity entity, bool active);

            /** @brief Removes the component associated with the specified entity. If the entity's component is active, it is first swapped with the
             * last active component to maintain the dense packing of active components. The component is then swapped to the end of the vector and popped off.
             * @param entity The entity whose component is to be removed. The entity must have a component.
             */
            void RemoveComponent(Entity entity);

            /** @brief Checks if the specified entity is currently disabled in this component store.
             * @param entity The entity to check for being disabled.
             * @return True if the entity has a component that is currently inactive, false otherwise.
             */
            [[nodiscard]] VE_FORCE_INLINE bool IsDisabled(Entity entity) const {
                VASSERT(HasComponent(entity), "Entity does not have a component.");

                return _entityToComponentIndex.find(entity)->second >= _activeCount;
            }

            /** @brief Checks if the specified entity has a component associated with it in this component store.
             * @param entity The entity to check for having a component.
             * @return True if the entity has a component associated with it, false otherwise.
             */
            [[nodiscard]] VE_FORCE_INLINE bool HasComponent(Entity entity) const {
                return _entityToComponentIndex.contains(entity);
            }

            /** @brief Gets the total number of components (active + inactive) currently managed.
             * @return The total number of entities that have a component associated with them.
             */
            [[nodiscard]] VE_FORCE_INLINE size_t GetTotalComponentCount() const {
                return _entities.size();
            }

            /** @brief Gets the total number of active components currently managed by this component store.
             * @return The total number of entities that currently have an active component associated with them.
             */
            [[nodiscard]] VE_FORCE_INLINE size_t GetActiveComponentCount() const {
                return _activeCount;
            }

            /** @brief Returns a contiguous view of the entities that have active components.
             * @return A span over the entities corresponding to the densely packed active components at the front of the storage.
             */
            [[nodiscard]] VE_FORCE_INLINE std::span<const Entity> GetActiveEntities() const {
                return { _entities.data(), _activeCount };
            }

            /** @brief Retrieves the entity associated with the component at the specified index in the component vector. The index must be a valid index within
             * the component vector.
             * @param index The index of the component whose associated entity is to be retrieved. Must be a valid index within the component vector.
             * @return The entity associated with the component at the specified index in the component vector. */
            [[nodiscard]] VE_FORCE_INLINE Entity GetEntityAtIndex(size_t index) const {
                VASSERT(index < _entities.size(), "Index out of bounds.");

                return _entities[index];
            }

            /** @brief Retrieves the index of the component in the component vector associated with the specified entity. The entity must have a component
             * associated with it.
             * @param entity The entity whose component index is to be retrieved. The entity must have a component associated with it.
             * @return The index of the component associated with the specified entity in the component vector.
             */
            [[nodiscard]] VE_FORCE_INLINE size_t GetEntityIndex(Entity entity) const {
                VASSERT(HasComponent(entity), "Entity does not have a component.");

                return _entityToComponentIndex.find(entity)->second;
            }
    };

} // namespace Vulkyrie
