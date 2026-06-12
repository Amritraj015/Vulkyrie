#include "physics/components/hinge_joint_component_store.h"

namespace Vulkyrie {

    HingeJointComponentStore::HingeJointComponentStore() {
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
        _hingeAxisInBodyOneLocalSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _hingeAxisInBodyTwoLocalSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _hingeAxisWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _b2CrossA1Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _c2CrossA1Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseLowerLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseUpperLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseMotors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassMatrixLimitMotors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassMatrixMotors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _biasLowerLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _biasUpperLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _limitEnabledFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _motorEnabledFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _lowerLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _upperLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _lowerLimitViolatedFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _upperLimitViolatedFlags.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _motorSpeeds.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _maxMotorTorques.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void HingeJointComponentStore::AddComponent(Entity jointEntity, const HingeJointComponent &component, bool active) {
        VASSERT(!HasComponent(jointEntity), "Entity already has a HingeJointComponent.");

        size_t index = _entities.size();

        _joints.push_back(nullptr);
        _localSpaceAnchorPointsOnBodyOne.emplace_back(0.0f, 0.0f, 0.0f);
        _localSpaceAnchorPointsOnBodyTwo.emplace_back(0.0f, 0.0f, 0.0f);
        _r1WorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _r2WorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _bodyOneInertiaTensorsInWorldSpace.emplace_back();
        _bodyTwoInertiaTensorsInWorldSpace.emplace_back();
        _impulseTranslations.emplace_back(0.0f, 0.0f, 0.0f);
        _impulseRotations.emplace_back(0.0f, 0.0f);
        _inverseMassTranslationMatrices.emplace_back();
        _inverseMassRotationMatrices.emplace_back();
        _translationBiases.emplace_back(0.0f, 0.0f, 0.0f);
        _rotationBiases.emplace_back(0.0f, 0.0f);
        _initialOrientationDifferenceInverses.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
        _hingeAxisInBodyOneLocalSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _hingeAxisInBodyTwoLocalSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _hingeAxisWorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _b2CrossA1Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _c2CrossA1Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _impulseLowerLimits.push_back(0.0f);
        _impulseUpperLimits.push_back(0.0f);
        _impulseMotors.push_back(0.0f);
        _inverseMassMatrixLimitMotors.push_back(0.0f);
        _inverseMassMatrixMotors.push_back(0.0f);
        _biasLowerLimits.push_back(0.0f);
        _biasUpperLimits.push_back(0.0f);
        _limitEnabledFlags.push_back(static_cast<u8>(component.LimitEnabled));
        _motorEnabledFlags.push_back(static_cast<u8>(component.MotorEnabled));
        _lowerLimits.push_back(component.LowerLimit);
        _upperLimits.push_back(component.UpperLimit);
        _lowerLimitViolatedFlags.push_back(0);
        _upperLimitViolatedFlags.push_back(0);
        _motorSpeeds.push_back(component.MotorSpeed);
        _maxMotorTorques.push_back(component.MaxMotorTorque);
        _entities.push_back(jointEntity);

        if (active) {
            if (index != _activeCount) {
                swapComponents(index, _activeCount);
            } else {
                _entityToComponentIndex[jointEntity] = index;
            }
            _activeCount++;
        } else {
            _entityToComponentIndex[jointEntity] = index;
        }
    }

    void HingeJointComponentStore::swapComponents(size_t indexA, size_t indexB) {
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
        std::swap(_hingeAxisInBodyOneLocalSpace[indexA], _hingeAxisInBodyOneLocalSpace[indexB]);
        std::swap(_hingeAxisInBodyTwoLocalSpace[indexA], _hingeAxisInBodyTwoLocalSpace[indexB]);
        std::swap(_hingeAxisWorldSpace[indexA], _hingeAxisWorldSpace[indexB]);
        std::swap(_b2CrossA1Vectors[indexA], _b2CrossA1Vectors[indexB]);
        std::swap(_c2CrossA1Vectors[indexA], _c2CrossA1Vectors[indexB]);
        std::swap(_impulseLowerLimits[indexA], _impulseLowerLimits[indexB]);
        std::swap(_impulseUpperLimits[indexA], _impulseUpperLimits[indexB]);
        std::swap(_impulseMotors[indexA], _impulseMotors[indexB]);
        std::swap(_inverseMassMatrixLimitMotors[indexA], _inverseMassMatrixLimitMotors[indexB]);
        std::swap(_inverseMassMatrixMotors[indexA], _inverseMassMatrixMotors[indexB]);
        std::swap(_biasLowerLimits[indexA], _biasLowerLimits[indexB]);
        std::swap(_biasUpperLimits[indexA], _biasUpperLimits[indexB]);
        std::swap(_limitEnabledFlags[indexA], _limitEnabledFlags[indexB]);
        std::swap(_motorEnabledFlags[indexA], _motorEnabledFlags[indexB]);
        std::swap(_lowerLimits[indexA], _lowerLimits[indexB]);
        std::swap(_upperLimits[indexA], _upperLimits[indexB]);
        std::swap(_lowerLimitViolatedFlags[indexA], _lowerLimitViolatedFlags[indexB]);
        std::swap(_upperLimitViolatedFlags[indexA], _upperLimitViolatedFlags[indexB]);
        std::swap(_motorSpeeds[indexA], _motorSpeeds[indexB]);
        std::swap(_maxMotorTorques[indexA], _maxMotorTorques[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void HingeJointComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no HingeJointComponents available to be removed.");

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
        _hingeAxisInBodyOneLocalSpace.pop_back();
        _hingeAxisInBodyTwoLocalSpace.pop_back();
        _hingeAxisWorldSpace.pop_back();
        _b2CrossA1Vectors.pop_back();
        _c2CrossA1Vectors.pop_back();
        _impulseLowerLimits.pop_back();
        _impulseUpperLimits.pop_back();
        _impulseMotors.pop_back();
        _inverseMassMatrixLimitMotors.pop_back();
        _inverseMassMatrixMotors.pop_back();
        _biasLowerLimits.pop_back();
        _biasUpperLimits.pop_back();
        _limitEnabledFlags.pop_back();
        _motorEnabledFlags.pop_back();
        _lowerLimits.pop_back();
        _upperLimits.pop_back();
        _lowerLimitViolatedFlags.pop_back();
        _upperLimitViolatedFlags.pop_back();
        _motorSpeeds.pop_back();
        _maxMotorTorques.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
