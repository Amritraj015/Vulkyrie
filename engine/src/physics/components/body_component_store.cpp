#include "physics/components/body_component_store.h"

namespace Vulkyrie {

    BodyComponentStore::BodyComponentStore() {
        _bodies.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _colliders.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyActiveFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _simulationColliderFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void BodyComponentStore::AddComponent(Entity entity, const BodyComponent &component, bool active) {
        VASSERT(!HasComponent(entity), "Entity already has a BodyComponent.");

        size_t index = _entities.size();

        // Append the new component and its associated entity to the end of their respective vectors.
        _bodies.push_back(component.Body);
        _colliders.emplace_back();
        _bodyActiveFlags.push_back(active);
        _simulationColliderFlags.push_back(false);
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

    void BodyComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_bodies[indexA], _bodies[indexB]);
        std::swap(_colliders[indexA], _colliders[indexB]);
        std::swap(_bodyActiveFlags[indexA], _bodyActiveFlags[indexB]);
        std::swap(_simulationColliderFlags[indexA], _simulationColliderFlags[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void BodyComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no BodyComponents available to be removed.");

        _bodies.pop_back();
        _colliders.pop_back();
        _bodyActiveFlags.pop_back();
        _simulationColliderFlags.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
