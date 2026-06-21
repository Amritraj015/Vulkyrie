#include "physics/components/ball_and_socket_joint_component_store.h"

namespace Vulkyrie {

    BallAndSocketJointComponentStore::BallAndSocketJointComponentStore() {
        _joints.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localSpaceAnchorPointsOnBodyOne.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localSpaceAnchorPointsOnBodyTwo.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r1WorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r2WorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyOneInertiaTensorsInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyTwoInertiaTensorsInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _biasVectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassMatrices.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _coneLimitEnabledFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _coneLimitImpulses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _coneLimitHalfAngles.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassMatrixConeLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _coneLimitBiases.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _coneLimitViolatedFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _coneLimitAxesCrossProducts.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void BallAndSocketJointComponentStore::AddComponent(Entity jointEntity, const BallAndSocketJointComponent &component, bool active) {
        VASSERT(!HasComponent(jointEntity), "Entity already has a BodyComponent.");

        size_t index = _entities.size();

        // Append the new component and its associated entity to the end of their respective vectors.
        _joints.push_back(nullptr);
        _localSpaceAnchorPointsOnBodyOne.emplace_back(0.0f, 0.0f, 0.0f);
        _localSpaceAnchorPointsOnBodyTwo.emplace_back(0.0f, 0.0f, 0.0f);
        _r1WorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _r2WorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _bodyOneInertiaTensorsInWorldSpace.emplace_back();
        _bodyTwoInertiaTensorsInWorldSpace.emplace_back();
        _biasVectors.emplace_back(0.0f, 0.0f, 0.0f);
        _inverseMassMatrices.emplace_back();
        _impulses.emplace_back(0.0f, 0.0f, 0.0f);
        _coneLimitEnabledFlags.emplace_back(static_cast<u8>(component.ConeLimitEnabled));
        _coneLimitImpulses.emplace_back(0.0f);
        _coneLimitHalfAngles.emplace_back(component.ConeLimitHalfAngle);
        _inverseMassMatrixConeLimits.emplace_back(0.0f);
        _coneLimitBiases.emplace_back(0.0f);
        _coneLimitViolatedFlags.emplace_back(static_cast<u8>(false));
        _coneLimitAxesCrossProducts.emplace_back(0.0f, 0.0f, 0.0f);
        _entities.push_back(jointEntity);

        if (active) {
            // Swap the new component into the active zone if inactive components sit between it and _activeCount.
            // swapComponents updates _entityToComponentIndex for both swapped entries;
            // otherwise we set the mapping ourselves since no swap is needed.
            if (index != _activeCount) {
                swapComponents(index, _activeCount);
            } else {
                _entityToComponentIndex[jointEntity] = index;
            }

            // Grow the active zone to include the newly added component.
            _activeCount++;
        } else {
            // Inactive components stay at the end; just record the mapping.
            _entityToComponentIndex[jointEntity] = index;
        }
    }

    void BallAndSocketJointComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_joints[indexA], _joints[indexB]);
        std::swap(_localSpaceAnchorPointsOnBodyOne[indexA], _localSpaceAnchorPointsOnBodyOne[indexB]);
        std::swap(_localSpaceAnchorPointsOnBodyTwo[indexA], _localSpaceAnchorPointsOnBodyTwo[indexB]);
        std::swap(_r1WorldSpace[indexA], _r1WorldSpace[indexB]);
        std::swap(_r2WorldSpace[indexA], _r2WorldSpace[indexB]);
        std::swap(_bodyOneInertiaTensorsInWorldSpace[indexA], _bodyOneInertiaTensorsInWorldSpace[indexB]);
        std::swap(_bodyTwoInertiaTensorsInWorldSpace[indexA], _bodyTwoInertiaTensorsInWorldSpace[indexB]);
        std::swap(_biasVectors[indexA], _biasVectors[indexB]);
        std::swap(_inverseMassMatrices[indexA], _inverseMassMatrices[indexB]);
        std::swap(_impulses[indexA], _impulses[indexB]);
        std::swap(_coneLimitEnabledFlags[indexA], _coneLimitEnabledFlags[indexB]);
        std::swap(_coneLimitImpulses[indexA], _coneLimitImpulses[indexB]);
        std::swap(_coneLimitHalfAngles[indexA], _coneLimitHalfAngles[indexB]);
        std::swap(_inverseMassMatrixConeLimits[indexA], _inverseMassMatrixConeLimits[indexB]);
        std::swap(_coneLimitBiases[indexA], _coneLimitBiases[indexB]);
        std::swap(_coneLimitViolatedFlags[indexA], _coneLimitViolatedFlags[indexB]);
        std::swap(_coneLimitAxesCrossProducts[indexA], _coneLimitAxesCrossProducts[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void BallAndSocketJointComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no BodyComponents available to be removed.");

        _joints.pop_back();
        _localSpaceAnchorPointsOnBodyOne.pop_back();
        _localSpaceAnchorPointsOnBodyTwo.pop_back();
        _r1WorldSpace.pop_back();
        _r2WorldSpace.pop_back();
        _bodyOneInertiaTensorsInWorldSpace.pop_back();
        _bodyTwoInertiaTensorsInWorldSpace.pop_back();
        _biasVectors.pop_back();
        _inverseMassMatrices.pop_back();
        _impulses.pop_back();
        _coneLimitEnabledFlags.pop_back();
        _coneLimitImpulses.pop_back();
        _coneLimitHalfAngles.pop_back();
        _inverseMassMatrixConeLimits.pop_back();
        _coneLimitBiases.pop_back();
        _coneLimitViolatedFlags.pop_back();
        _coneLimitAxesCrossProducts.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
