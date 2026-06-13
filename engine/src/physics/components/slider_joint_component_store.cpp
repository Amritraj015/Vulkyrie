#include "physics/components/slider_joint_component_store.h"

namespace Vulkyrie {

    SliderJointComponentStore::SliderJointComponentStore() {
        _joints.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localSpaceAnchorPointsOnBodyOne.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _localSpaceAnchorPointsOnBodyTwo.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyOneInertiaTensorsInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _bodyTwoInertiaTensorsInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseTranslations.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseRotations.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassTranslationMatrices.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassRotationMatrices.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _translationBiases.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _rotationBiases.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _initialOrientationDifferenceInverses.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _sliderAxisInBodyOneLocalSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _sliderAxisInWorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r1WorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r2WorldSpace.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _n1Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _n2Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseLowerLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseUpperLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _impulseMotors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _inverseMassMatrixLimits.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
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
        _maxMotorForces.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r2CrossN1Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r2CrossN2Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r2CrossSliderAxisVectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r1PlusUCrossN1Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r1PlusUCrossN2Vectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
        _r1PlusUCrossSliderAxisVectors.reserve(INITIAL_COMPONENT_RESERVATION_COUNT);
    }

    void SliderJointComponentStore::AddComponent(Entity jointEntity, const SliderJointComponent &component, bool active) {
        VASSERT(!HasComponent(jointEntity), "Entity already has a SliderJointComponent.");

        size_t index = _entities.size();

        _joints.push_back(nullptr);
        _localSpaceAnchorPointsOnBodyOne.emplace_back(0.0f, 0.0f, 0.0f);
        _localSpaceAnchorPointsOnBodyTwo.emplace_back(0.0f, 0.0f, 0.0f);
        _bodyOneInertiaTensorsInWorldSpace.emplace_back();
        _bodyTwoInertiaTensorsInWorldSpace.emplace_back();
        _impulseTranslations.emplace_back(0.0f, 0.0f);
        _impulseRotations.emplace_back(0.0f, 0.0f, 0.0f);
        _inverseMassTranslationMatrices.emplace_back();
        _inverseMassRotationMatrices.emplace_back();
        _translationBiases.emplace_back(0.0f, 0.0f);
        _rotationBiases.emplace_back(0.0f, 0.0f, 0.0f);
        _initialOrientationDifferenceInverses.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);
        _sliderAxisInBodyOneLocalSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _sliderAxisInWorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _r1WorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _r2WorldSpace.emplace_back(0.0f, 0.0f, 0.0f);
        _n1Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _n2Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _impulseLowerLimits.push_back(0.0f);
        _impulseUpperLimits.push_back(0.0f);
        _impulseMotors.push_back(0.0f);
        _inverseMassMatrixLimits.push_back(0.0f);
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
        _maxMotorForces.push_back(component.MaxMotorForce);
        _r2CrossN1Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _r2CrossN2Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _r2CrossSliderAxisVectors.emplace_back(0.0f, 0.0f, 0.0f);
        _r1PlusUCrossN1Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _r1PlusUCrossN2Vectors.emplace_back(0.0f, 0.0f, 0.0f);
        _r1PlusUCrossSliderAxisVectors.emplace_back(0.0f, 0.0f, 0.0f);
        _entities.push_back(jointEntity);

        if (active) {
            if (index != _activeCount) {
                // New entry landed beyond the active zone (inactive entities exist between
                // _activeCount and index). Swap it into _activeCount; swapComponents also
                // updates _entityToComponentIndex for both affected entities.
                swapComponents(index, _activeCount);
            } else {
                _entityToComponentIndex[jointEntity] = index;
            }
            _activeCount++;
        } else {
            _entityToComponentIndex[jointEntity] = index;
        }
    }

    void SliderJointComponentStore::swapComponents(size_t indexA, size_t indexB) {
        if (indexA == indexB) return;

        std::swap(_joints[indexA], _joints[indexB]);
        std::swap(_localSpaceAnchorPointsOnBodyOne[indexA], _localSpaceAnchorPointsOnBodyOne[indexB]);
        std::swap(_localSpaceAnchorPointsOnBodyTwo[indexA], _localSpaceAnchorPointsOnBodyTwo[indexB]);
        std::swap(_bodyOneInertiaTensorsInWorldSpace[indexA], _bodyOneInertiaTensorsInWorldSpace[indexB]);
        std::swap(_bodyTwoInertiaTensorsInWorldSpace[indexA], _bodyTwoInertiaTensorsInWorldSpace[indexB]);
        std::swap(_impulseTranslations[indexA], _impulseTranslations[indexB]);
        std::swap(_impulseRotations[indexA], _impulseRotations[indexB]);
        std::swap(_inverseMassTranslationMatrices[indexA], _inverseMassTranslationMatrices[indexB]);
        std::swap(_inverseMassRotationMatrices[indexA], _inverseMassRotationMatrices[indexB]);
        std::swap(_translationBiases[indexA], _translationBiases[indexB]);
        std::swap(_rotationBiases[indexA], _rotationBiases[indexB]);
        std::swap(_initialOrientationDifferenceInverses[indexA], _initialOrientationDifferenceInverses[indexB]);
        std::swap(_sliderAxisInBodyOneLocalSpace[indexA], _sliderAxisInBodyOneLocalSpace[indexB]);
        std::swap(_sliderAxisInWorldSpace[indexA], _sliderAxisInWorldSpace[indexB]);
        std::swap(_r1WorldSpace[indexA], _r1WorldSpace[indexB]);
        std::swap(_r2WorldSpace[indexA], _r2WorldSpace[indexB]);
        std::swap(_n1Vectors[indexA], _n1Vectors[indexB]);
        std::swap(_n2Vectors[indexA], _n2Vectors[indexB]);
        std::swap(_impulseLowerLimits[indexA], _impulseLowerLimits[indexB]);
        std::swap(_impulseUpperLimits[indexA], _impulseUpperLimits[indexB]);
        std::swap(_impulseMotors[indexA], _impulseMotors[indexB]);
        std::swap(_inverseMassMatrixLimits[indexA], _inverseMassMatrixLimits[indexB]);
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
        std::swap(_maxMotorForces[indexA], _maxMotorForces[indexB]);
        std::swap(_r2CrossN1Vectors[indexA], _r2CrossN1Vectors[indexB]);
        std::swap(_r2CrossN2Vectors[indexA], _r2CrossN2Vectors[indexB]);
        std::swap(_r2CrossSliderAxisVectors[indexA], _r2CrossSliderAxisVectors[indexB]);
        std::swap(_r1PlusUCrossN1Vectors[indexA], _r1PlusUCrossN1Vectors[indexB]);
        std::swap(_r1PlusUCrossN2Vectors[indexA], _r1PlusUCrossN2Vectors[indexB]);
        std::swap(_r1PlusUCrossSliderAxisVectors[indexA], _r1PlusUCrossSliderAxisVectors[indexB]);
        std::swap(_entities[indexA], _entities[indexB]);

        _entityToComponentIndex[_entities[indexA]] = indexA;
        _entityToComponentIndex[_entities[indexB]] = indexB;
    }

    void SliderJointComponentStore::removeLastComponentAndEntity() {
        VASSERT(!_entities.empty(), "There are no SliderJointComponents available to be removed.");

        _joints.pop_back();
        _localSpaceAnchorPointsOnBodyOne.pop_back();
        _localSpaceAnchorPointsOnBodyTwo.pop_back();
        _bodyOneInertiaTensorsInWorldSpace.pop_back();
        _bodyTwoInertiaTensorsInWorldSpace.pop_back();
        _impulseTranslations.pop_back();
        _impulseRotations.pop_back();
        _inverseMassTranslationMatrices.pop_back();
        _inverseMassRotationMatrices.pop_back();
        _translationBiases.pop_back();
        _rotationBiases.pop_back();
        _initialOrientationDifferenceInverses.pop_back();
        _sliderAxisInBodyOneLocalSpace.pop_back();
        _sliderAxisInWorldSpace.pop_back();
        _r1WorldSpace.pop_back();
        _r2WorldSpace.pop_back();
        _n1Vectors.pop_back();
        _n2Vectors.pop_back();
        _impulseLowerLimits.pop_back();
        _impulseUpperLimits.pop_back();
        _impulseMotors.pop_back();
        _inverseMassMatrixLimits.pop_back();
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
        _maxMotorForces.pop_back();
        _r2CrossN1Vectors.pop_back();
        _r2CrossN2Vectors.pop_back();
        _r2CrossSliderAxisVectors.pop_back();
        _r1PlusUCrossN1Vectors.pop_back();
        _r1PlusUCrossN2Vectors.pop_back();
        _r1PlusUCrossSliderAxisVectors.pop_back();
        _entities.pop_back();
    }

} // namespace Vulkyrie
