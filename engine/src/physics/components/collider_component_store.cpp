#include "physics/components/collider_component_store.h"

namespace Vulkyrie {

    ColliderComponentStore::ColliderComponentStore() {
        _bodyEntities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _colliders.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _broadPhaseIDs.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localToBodyTransforms.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _collisionShapes.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _collisionCategoryBits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _collidesWithMaskBits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localToWorldTransforms.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _OverlappingPairs.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _collisionShapeChangedSizeFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _isTriggerFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _isSimulationColliderFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _isQueryColliderFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _materials.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void ColliderComponentStore::AddComponent(Entity entity, const ColliderComponent &component, bool active) {
        VASSERT(!HasComponent(entity), "Entity already has a ColliderComponent.");

        size_t index = _entities.size();

        // Append the new component and its associated entity to the end of their respective vectors.
        _bodyEntities.push_back(component.BodyEntity);
        _colliders.push_back(component.Collider);
        _broadPhaseIDs.push_back(static_cast<i32>(-1));
        _localToBodyTransforms.push_back(component.LocalToBodyTransform);
        _collisionShapes.push_back(component.CollisionShape);
        _collisionCategoryBits.push_back(component.CollisionCategoryBits);
        _collidesWithMaskBits.push_back(component.CollidesWithMaskBits);
        _localToWorldTransforms.push_back(component.LocalToWorldTransform);

        _OverlappingPairs.emplace_back();

        _collisionShapeChangedSizeFlags.push_back(false);
        _isTriggerFlags.push_back(false);
        _isSimulationColliderFlags.push_back(false);
        _isQueryColliderFlags.push_back(false);
        _materials.push_back(component.Material);
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

    void ColliderComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_bodyEntities[indexA], _bodyEntities[indexB]);
        std::swap(_colliders[indexA], _colliders[indexB]);
        std::swap(_broadPhaseIDs[indexA], _broadPhaseIDs[indexB]);
        std::swap(_localToBodyTransforms[indexA], _localToBodyTransforms[indexB]);
        std::swap(_collisionShapes[indexA], _collisionShapes[indexB]);
        std::swap(_collisionCategoryBits[indexA], _collisionCategoryBits[indexB]);
        std::swap(_collidesWithMaskBits[indexA], _collidesWithMaskBits[indexB]);
        std::swap(_localToWorldTransforms[indexA], _localToWorldTransforms[indexB]);
        std::swap(_OverlappingPairs[indexA], _OverlappingPairs[indexB]);
        std::swap(_collisionShapeChangedSizeFlags[indexA], _collisionShapeChangedSizeFlags[indexB]);
        std::swap(_isTriggerFlags[indexA], _isTriggerFlags[indexB]);
        std::swap(_isSimulationColliderFlags[indexA], _isSimulationColliderFlags[indexB]);
        std::swap(_isQueryColliderFlags[indexA], _isQueryColliderFlags[indexB]);
        std::swap(_materials[indexA], _materials[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void ColliderComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no ColliderComponent available to be removed.");

        _bodyEntities.pop_back();
        _colliders.pop_back();
        _broadPhaseIDs.pop_back();
        _localToBodyTransforms.pop_back();
        _collisionShapes.pop_back();
        _collisionCategoryBits.pop_back();
        _collidesWithMaskBits.pop_back();
        _localToWorldTransforms.pop_back();
        _OverlappingPairs.pop_back();
        _collisionShapeChangedSizeFlags.pop_back();
        _isTriggerFlags.pop_back();
        _isSimulationColliderFlags.pop_back();
        _isQueryColliderFlags.pop_back();
        _materials.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
