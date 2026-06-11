#include "physics/components/joint_component_store.h"

namespace Vulkyrie {

    JointComponentStore::JointComponentStore() {
        _bodyOneEntities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyTwoEntities.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _joints.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _jointTypes.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _positionCorrectionTechniques.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _collisionEnabledFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _jointInIslandFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void JointComponentStore::AddComponent(Entity entity, const JointComponent &component, bool active) {
        VASSERT(!HasComponent(entity), "Entity already has a BodyComponent.");

        size_t index = _entities.size();

        // Append the new component and its associated entity to the end of their respective vectors.
        _bodyOneEntities.push_back(component.BodyOneEntity);
        _bodyTwoEntities.emplace_back(component.BodyTwoEntity);
        _joints.push_back(component.Joint);
        _jointTypes.push_back(component.JointType);
        _positionCorrectionTechniques.push_back(component.PositionCorrectionTechnique);
        _collisionEnabledFlags.push_back(component.CollisionEnabled);
        _jointInIslandFlags.push_back(false);
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

    void JointComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_bodyOneEntities[indexA], _bodyOneEntities[indexB]);
        std::swap(_bodyTwoEntities[indexA], _bodyTwoEntities[indexB]);
        std::swap(_joints[indexA], _joints[indexB]);
        std::swap(_jointTypes[indexA], _jointTypes[indexB]);
        std::swap(_positionCorrectionTechniques[indexA], _positionCorrectionTechniques[indexB]);
        std::swap(_collisionEnabledFlags[indexA], _collisionEnabledFlags[indexB]);
        std::swap(_jointInIslandFlags[indexA], _jointInIslandFlags[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void JointComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no BodyComponents available to be removed.");

        _bodyOneEntities.pop_back();
        _bodyTwoEntities.pop_back();
        _joints.pop_back();
        _jointTypes.pop_back();
        _positionCorrectionTechniques.pop_back();
        _collisionEnabledFlags.pop_back();
        _jointInIslandFlags.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
