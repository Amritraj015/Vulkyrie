#include "physics/components/fixed_joint_component_store.h"

namespace Vulkyrie {

    FixedJointComponentStore::FixedJointComponentStore() {
        _joints.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localSpaceAnchorPointsOnBodyOne.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localSpaceAnchorPointsOnBodyTwo.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r1WorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r2WorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyOneInertiaTensorsInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyTwoInertiaTensorsInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseTranslations.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseRotations.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassTranslationMatrices.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassRotationMatrices.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _translationBiases.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _rotationBiases.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _initialOrientationDifferenceInverses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void FixedJointComponentStore::AddComponent(Entity jointEntity, bool active) {
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
        _impulseTranslations.emplace_back(0.0f, 0.0f, 0.0f);
        _impulseRotations.emplace_back(0.0f, 0.0f, 0.0f);
        _inverseMassTranslationMatrices.emplace_back();
        _inverseMassRotationMatrices.emplace_back();
        _translationBiases.emplace_back(0.0f, 0.0f, 0.0f);
        _rotationBiases.emplace_back(0.0f, 0.0f, 0.0f);
        _initialOrientationDifferenceInverses.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
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

    void FixedJointComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_joints[indexA], _joints[indexB]);
        std::swap(_localSpaceAnchorPointsOnBodyOne[indexA], _localSpaceAnchorPointsOnBodyOne[indexB]);
        std::swap(_localSpaceAnchorPointsOnBodyTwo[indexA], _localSpaceAnchorPointsOnBodyTwo[indexB]);
        std::swap(_r1WorldSpace[indexA], _r1WorldSpace[indexB]);
        std::swap(_r2WorldSpace[indexA], _r2WorldSpace[indexB]);
        std::swap(_bodyOneInertiaTensorsInWorldSpace[indexA], _bodyOneInertiaTensorsInWorldSpace[indexB]);
        std::swap(_bodyTwoInertiaTensorsInWorldSpace[indexA], _bodyTwoInertiaTensorsInWorldSpace[indexB]);
        std::swap(_impulseTranslations[indexA], _impulseTranslations[indexB]);
        std::swap(_impulseRotations[indexA], _impulseRotations[indexB]);
        std::swap(_inverseMassTranslationMatrices[indexA], _inverseMassTranslationMatrices[indexB]);
        std::swap(_inverseMassRotationMatrices[indexA], _inverseMassRotationMatrices[indexB]);
        std::swap(_translationBiases[indexA], _translationBiases[indexB]);
        std::swap(_rotationBiases[indexA], _rotationBiases[indexB]);
        std::swap(_initialOrientationDifferenceInverses[indexA], _initialOrientationDifferenceInverses[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void FixedJointComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no BodyComponents available to be removed.");

        _joints.pop_back();
        _localSpaceAnchorPointsOnBodyOne.pop_back();
        _localSpaceAnchorPointsOnBodyTwo.pop_back();
        _r1WorldSpace.pop_back();
        _r2WorldSpace.pop_back();
        _bodyOneInertiaTensorsInWorldSpace.pop_back();
        _bodyTwoInertiaTensorsInWorldSpace.pop_back();
        _impulseTranslations.pop_back();
        _impulseRotations.pop_back();
        _inverseMassTranslationMatrices.pop_back();
        _inverseMassRotationMatrices.pop_back();
        _translationBiases.pop_back();
        _rotationBiases.pop_back();
        _initialOrientationDifferenceInverses.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
