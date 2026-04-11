#include "physics/components/component_store.h"
#include "core/asserts.h"

namespace Vulkyrie {

    ComponentStore::ComponentStore()
        : _activeCount(0) {
        _entities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _entityToComponentIndex.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void ComponentStore::SetActiveStatus(Entity entity, bool active) {
        VASSERT_EXPR(HasComponent(entity), "Entity does not have a component.");

        size_t index = _entityToComponentIndex[entity];

        if (active && index >= _activeCount) {
            // Swap the component at the current index with the component at the index corresponding to
            // the current count of active components to maintain the dense packing of active components.
            swapComponents(index, _activeCount);

            // Increment the count of active components since we have activated a new component.
            _activeCount++;
        } else if (!active && index < _activeCount) {
            // Decrement the count of active components since we will be deactivating a component.
            _activeCount--;

            // Swap the component at the current index with the component at the index corresponding to
            // the new count of active components to maintain the dense packing of active components.
            swapComponents(index, _activeCount);
        }
    }

    void ComponentStore::RemoveComponent(Entity entity) {
        VASSERT_EXPR(HasComponent(entity), "Entity does not have a component.");

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

} // namespace Vulkyrie
