#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    TransformComponentStore::TransformComponentStore() {
        _transforms.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void TransformComponentStore::AddComponent(Entity entity, const TransformComponent &transformComponent, bool active) {
        VASSERT_EXPR(!HasComponent(entity), "Entity already has a TransformComponent.");

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

    void TransformComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_transforms[indexA], _transforms[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void TransformComponentStore::removeLastComponentAndEntity() {
        VASSERT_EXPR(!_transforms.empty(), "There are no TransformComponents available to be removed.");

        _transforms.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
