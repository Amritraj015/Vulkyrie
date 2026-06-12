#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/constraint/slider_joint.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    /** @brief Initialisation data carried in AddComponent for a slider joint. */
    struct SliderJointComponent {
        /** @brief Minimum allowed translation along the slider axis. */
        f32 LowerLimit;

        /** @brief Maximum allowed translation along the slider axis. */
        f32 UpperLimit;

        /** @brief Target speed of the linear motor (m/s). */
        f32 MotorSpeed;

        /** @brief Maximum force the motor may exert to reach the target speed (N). */
        f32 MaxMotorForce;

        /** @brief True if the translation limits are active. */
        bool LimitEnabled;

        /** @brief True if the linear motor is active. */
        bool MotorEnabled;

        SliderJointComponent(bool limitEnabled, bool motorEnabled, f32 lowerLimit, f32 upperLimit, f32 motorSpeed, f32 maxMotorForce)
            : LowerLimit(lowerLimit)
            , UpperLimit(upperLimit)
            , MotorSpeed(motorSpeed)
            , MaxMotorForce(maxMotorForce)
            , LimitEnabled(limitEnabled)
            , MotorEnabled(motorEnabled) {
        }
    };

    /** @brief Stores slider joint components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that represents a slider joint owns exactly one component. The store holds all
     * constraint solver state needed per simulation step: anchor points in local space, world-space
     * lever arms, inertia tensors, translational (2-DOF) and rotational (3-DOF) impulses with their
     * inverse mass matrices and bias vectors, the initial inverse orientation difference, the slider
     * axis in both local and world space, the two orthogonal constraint axes n1/n2, precomputed cross
     * products, and limit/motor state. The dense active-zone invariant from ComponentStore is
     * maintained: active components occupy indices [0, _activeCount) and inactive ones
     * [_activeCount, size). All parallel arrays are kept in sync by swapComponents. */
    class SliderJointComponentStore : public ComponentStore {
    public:
        /** @brief Constructs an instance of SliderJointComponentStore and reserves initial storage for all parallel arrays. */
        SliderJointComponentStore();

        VE_DELETE_MOVE_AND_COPY(SliderJointComponentStore);

        /** @brief Destructor for SliderJointComponentStore. */
        ~SliderJointComponentStore() override = default;

        /** @brief Adds a component for the specified joint entity.
         * @param jointEntity The entity to which the component will be added. Must not already have a component.
         * @param component Initialisation data for the component.
         * @param active Whether the joint entity is currently active. */
        void AddComponent(Entity jointEntity, const SliderJointComponent &component, bool active);

        // ---- Joint pointer ----

        /** @brief Returns the SliderJoint pointer for the given entity.
         * @param jointEntity Must have a component.
         * @returns Non-owning pointer to the SliderJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE SliderJoint *GetJoint(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _joints[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the SliderJoint pointer at the given component index.
         * @param componentIndex Must be in bounds.
         * @returns Non-owning pointer to the SliderJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE SliderJoint *GetJointAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            return _joints[componentIndex];
        }

        /** @brief Sets the SliderJoint pointer for the given entity.
         * @param jointEntity Must have a component.
         * @param joint Non-owning pointer to associate with the entity. */
        VE_INLINE void SetJoint(Entity jointEntity, SliderJoint *joint) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _joints[_entityToComponentIndex.find(jointEntity)->second] = joint;
        }

        /** @brief Sets the SliderJoint pointer at the given component index.
         * @param componentIndex Must be in bounds.
         * @param joint Non-owning pointer to store at the index. */
        VE_INLINE void SetJointAtIndex(size_t componentIndex, SliderJoint *joint) {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            _joints[componentIndex] = joint;
        }

        // ---- Local-space anchor point on body one ----

        /** @brief Returns the anchor point on body one in that body's local space.
         * @param jointEntity Must have a component.
         * @returns Const reference to the local-space anchor point. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _localSpaceAnchorPointsOnBodyOne[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the anchor point on body one in that body's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @returns Const reference to the local-space anchor point. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyOne.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyOne.");
            return _localSpaceAnchorPointsOnBodyOne[componentIndex];
        }

        /** @brief Sets the anchor point on body one in that body's local space.
         * @param jointEntity Must have a component.
         * @param localAnchorPointBody1 The new local-space anchor point. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOne(Entity jointEntity, const glm::vec3 &localAnchorPointBody1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _localSpaceAnchorPointsOnBodyOne[_entityToComponentIndex.find(jointEntity)->second] = localAnchorPointBody1;
        }

        /** @brief Sets the anchor point on body one in that body's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @param localAnchorPointBody1 The new local-space anchor point. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyOneAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBody1) {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyOne.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyOne.");
            _localSpaceAnchorPointsOnBodyOne[componentIndex] = localAnchorPointBody1;
        }

        // ---- Local-space anchor point on body two ----

        /** @brief Returns the anchor point on body two in that body's local space.
         * @param jointEntity Must have a component.
         * @returns Const reference to the local-space anchor point. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _localSpaceAnchorPointsOnBodyTwo[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the anchor point on body two in that body's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @returns Const reference to the local-space anchor point. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyTwo.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyTwo.");
            return _localSpaceAnchorPointsOnBodyTwo[componentIndex];
        }

        /** @brief Sets the anchor point on body two in that body's local space.
         * @param jointEntity Must have a component.
         * @param localAnchorPointBody2 The new local-space anchor point. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwo(Entity jointEntity, const glm::vec3 &localAnchorPointBody2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _localSpaceAnchorPointsOnBodyTwo[_entityToComponentIndex.find(jointEntity)->second] = localAnchorPointBody2;
        }

        /** @brief Sets the anchor point on body two in that body's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @param localAnchorPointBody2 The new local-space anchor point. */
        VE_INLINE void SetLocalSpaceAnchorPointOnBodyTwoAtIndex(size_t componentIndex, const glm::vec3 &localAnchorPointBody2) {
            VASSERT(componentIndex < _localSpaceAnchorPointsOnBodyTwo.size(), "componentIndex out of bounds of _localSpaceAnchorPointsOnBodyTwo.");
            _localSpaceAnchorPointsOnBodyTwo[componentIndex] = localAnchorPointBody2;
        }

        // ---- Inertia tensor of body one (world space) ----

        /** @brief Returns the world-space inertia tensor of body one.
         * @param jointEntity Must have a component.
         * @returns Const reference to the world-space inertia tensor. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyOneInWorldSpace(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _bodyOneInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the world-space inertia tensor of body one at the given index.
         * @param componentIndex Must be in bounds.
         * @returns Const reference to the world-space inertia tensor. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyOneInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyOneInertiaTensorsInWorldSpace.");
            return _bodyOneInertiaTensorsInWorldSpace[componentIndex];
        }

        /** @brief Sets the world-space inertia tensor of body one.
         * @param jointEntity Must have a component.
         * @param i1 The new world-space inertia tensor. */
        VE_INLINE void SetInertiaTensorOfBodyOneInWorldSpace(Entity jointEntity, const glm::mat3 &i1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _bodyOneInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second] = i1;
        }

        /** @brief Sets the world-space inertia tensor of body one at the given index.
         * @param componentIndex Must be in bounds.
         * @param i1 The new world-space inertia tensor. */
        VE_INLINE void SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(size_t componentIndex, const glm::mat3 &i1) {
            VASSERT(componentIndex < _bodyOneInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyOneInertiaTensorsInWorldSpace.");
            _bodyOneInertiaTensorsInWorldSpace[componentIndex] = i1;
        }

        // ---- Inertia tensor of body two (world space) ----

        /** @brief Returns the world-space inertia tensor of body two.
         * @param jointEntity Must have a component.
         * @returns Const reference to the world-space inertia tensor. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyTwoInWorldSpace(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _bodyTwoInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the world-space inertia tensor of body two at the given index.
         * @param componentIndex Must be in bounds.
         * @returns Const reference to the world-space inertia tensor. */
        [[nodiscard]] VE_INLINE const glm::mat3 &GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _bodyTwoInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyTwoInertiaTensorsInWorldSpace.");
            return _bodyTwoInertiaTensorsInWorldSpace[componentIndex];
        }

        /** @brief Sets the world-space inertia tensor of body two.
         * @param jointEntity Must have a component.
         * @param i2 The new world-space inertia tensor. */
        VE_INLINE void SetInertiaTensorOfBodyTwoInWorldSpace(Entity jointEntity, const glm::mat3 &i2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _bodyTwoInertiaTensorsInWorldSpace[_entityToComponentIndex.find(jointEntity)->second] = i2;
        }

        /** @brief Sets the world-space inertia tensor of body two at the given index.
         * @param componentIndex Must be in bounds.
         * @param i2 The new world-space inertia tensor. */
        VE_INLINE void SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(size_t componentIndex, const glm::mat3 &i2) {
            VASSERT(componentIndex < _bodyTwoInertiaTensorsInWorldSpace.size(), "componentIndex out of bounds of _bodyTwoInertiaTensorsInWorldSpace.");
            _bodyTwoInertiaTensorsInWorldSpace[componentIndex] = i2;
        }

        // ---- Translational impulse (2-DOF) ----

        /** @brief Returns a mutable reference to the accumulated 2-DOF translational impulse.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetImpulseTranslation(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseTranslations[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the accumulated 2-DOF translational impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetImpulseTranslationAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulseTranslations.size(), "componentIndex out of bounds of _impulseTranslations.");
            return _impulseTranslations[componentIndex];
        }

        /** @brief Sets the accumulated 2-DOF translational impulse.
         * @param jointEntity Must have a component.
         * @param impulseTranslation The new translational impulse. */
        VE_INLINE void SetImpulseTranslation(Entity jointEntity, const glm::vec2 &impulseTranslation) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseTranslations[_entityToComponentIndex.find(jointEntity)->second] = impulseTranslation;
        }

        /** @brief Sets the accumulated 2-DOF translational impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseTranslation The new translational impulse. */
        VE_INLINE void SetImpulseTranslationAtIndex(size_t componentIndex, const glm::vec2 &impulseTranslation) {
            VASSERT(componentIndex < _impulseTranslations.size(), "componentIndex out of bounds of _impulseTranslations.");
            _impulseTranslations[componentIndex] = impulseTranslation;
        }

        // ---- Rotational impulse (3-DOF) ----

        /** @brief Returns a mutable reference to the accumulated 3-DOF rotational impulse.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseRotation(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseRotations[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the accumulated 3-DOF rotational impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseRotationAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulseRotations.size(), "componentIndex out of bounds of _impulseRotations.");
            return _impulseRotations[componentIndex];
        }

        /** @brief Sets the accumulated 3-DOF rotational impulse.
         * @param jointEntity Must have a component.
         * @param impulseRotation The new rotational impulse. */
        VE_INLINE void SetImpulseRotation(Entity jointEntity, const glm::vec3 &impulseRotation) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseRotations[_entityToComponentIndex.find(jointEntity)->second] = impulseRotation;
        }

        /** @brief Sets the accumulated 3-DOF rotational impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseRotation The new rotational impulse. */
        VE_INLINE void SetImpulseRotationAtIndex(size_t componentIndex, const glm::vec3 &impulseRotation) {
            VASSERT(componentIndex < _impulseRotations.size(), "componentIndex out of bounds of _impulseRotations.");
            _impulseRotations[componentIndex] = impulseRotation;
        }

        // ---- Translational inverse mass matrix ----

        /** @brief Returns a mutable reference to the effective inverse mass matrix K for the translational constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassTranslationMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassTranslationMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the effective inverse mass matrix K for the translational constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassTranslationMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassTranslationMatrices.size(), "componentIndex out of bounds of _inverseMassTranslationMatrices.");
            return _inverseMassTranslationMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix K for the translational constraint.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrix The new translational inverse mass matrix. */
        VE_INLINE void SetInverseMassTranslationMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassTranslationMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the effective inverse mass matrix K for the translational constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrix The new translational inverse mass matrix. */
        VE_INLINE void SetInverseMassTranslationMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassTranslationMatrices.size(), "componentIndex out of bounds of _inverseMassTranslationMatrices.");
            _inverseMassTranslationMatrices[componentIndex] = inverseMassMatrix;
        }

        // ---- Rotational inverse mass matrix ----

        /** @brief Returns a mutable reference to the effective inverse mass matrix K for the rotational constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassRotationMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassRotationMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the effective inverse mass matrix K for the rotational constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassRotationMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassRotationMatrices.size(), "componentIndex out of bounds of _inverseMassRotationMatrices.");
            return _inverseMassRotationMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix K for the rotational constraint.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrix The new rotational inverse mass matrix. */
        VE_INLINE void SetInverseMassRotationMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassRotationMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the effective inverse mass matrix K for the rotational constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrix The new rotational inverse mass matrix. */
        VE_INLINE void SetInverseMassRotationMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassRotationMatrices.size(), "componentIndex out of bounds of _inverseMassRotationMatrices.");
            _inverseMassRotationMatrices[componentIndex] = inverseMassMatrix;
        }

        // ---- Translation bias (Baumgarte) ----

        /** @brief Returns a mutable reference to the Baumgarte translation bias vector (2-DOF).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetTranslationBias(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _translationBiases[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the Baumgarte translation bias vector at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetTranslationBiasAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _translationBiases.size(), "componentIndex out of bounds of _translationBiases.");
            return _translationBiases[componentIndex];
        }

        /** @brief Sets the Baumgarte translation bias vector.
         * @param jointEntity Must have a component.
         * @param translationBias The new translation bias. */
        VE_INLINE void SetTranslationBias(Entity jointEntity, const glm::vec2 &translationBias) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _translationBiases[_entityToComponentIndex.find(jointEntity)->second] = translationBias;
        }

        /** @brief Sets the Baumgarte translation bias vector at the given index.
         * @param componentIndex Must be in bounds.
         * @param translationBias The new translation bias. */
        VE_INLINE void SetTranslationBiasAtIndex(size_t componentIndex, const glm::vec2 &translationBias) {
            VASSERT(componentIndex < _translationBiases.size(), "componentIndex out of bounds of _translationBiases.");
            _translationBiases[componentIndex] = translationBias;
        }

        // ---- Rotation bias (Baumgarte) ----

        /** @brief Returns a mutable reference to the Baumgarte rotation bias vector (3-DOF).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetRotationBias(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _rotationBiases[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the Baumgarte rotation bias vector at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetRotationBiasAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _rotationBiases.size(), "componentIndex out of bounds of _rotationBiases.");
            return _rotationBiases[componentIndex];
        }

        /** @brief Sets the Baumgarte rotation bias vector.
         * @param jointEntity Must have a component.
         * @param rotationBias The new rotation bias. */
        VE_INLINE void SetRotationBias(Entity jointEntity, const glm::vec3 &rotationBias) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _rotationBiases[_entityToComponentIndex.find(jointEntity)->second] = rotationBias;
        }

        /** @brief Sets the Baumgarte rotation bias vector at the given index.
         * @param componentIndex Must be in bounds.
         * @param rotationBias The new rotation bias. */
        VE_INLINE void SetRotationBiasAtIndex(size_t componentIndex, const glm::vec3 &rotationBias) {
            VASSERT(componentIndex < _rotationBiases.size(), "componentIndex out of bounds of _rotationBiases.");
            _rotationBiases[componentIndex] = rotationBias;
        }

        // ---- Initial orientation difference inverse ----

        /** @brief Returns a mutable reference to the inverse of the initial relative orientation between the two bodies.
         * Computed once at joint creation; the solver uses it each step to enforce the rotational constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::quat &GetInitialOrientationDifferenceInverse(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _initialOrientationDifferenceInverses[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the inverse of the initial relative orientation at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::quat &GetInitialOrientationDifferenceInverseAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _initialOrientationDifferenceInverses.size(), "componentIndex out of bounds of _initialOrientationDifferenceInverses.");
            return _initialOrientationDifferenceInverses[componentIndex];
        }

        /** @brief Sets the inverse of the initial relative orientation between the two bodies.
         * @param jointEntity Must have a component.
         * @param initialOrientationDifferenceInverse The inverse of the initial relative orientation quaternion. */
        VE_INLINE void SetInitialOrientationDifferenceInverse(Entity jointEntity, const glm::quat &initialOrientationDifferenceInverse) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _initialOrientationDifferenceInverses[_entityToComponentIndex.find(jointEntity)->second] = initialOrientationDifferenceInverse;
        }

        /** @brief Sets the inverse of the initial relative orientation at the given index.
         * @param componentIndex Must be in bounds.
         * @param initialOrientationDifferenceInverse The inverse of the initial relative orientation quaternion. */
        VE_INLINE void SetInitialOrientationDifferenceInverseAtIndex(size_t componentIndex, const glm::quat &initialOrientationDifferenceInverse) {
            VASSERT(componentIndex < _initialOrientationDifferenceInverses.size(), "componentIndex out of bounds of _initialOrientationDifferenceInverses.");
            _initialOrientationDifferenceInverses[componentIndex] = initialOrientationDifferenceInverse;
        }

        // ---- Slider axis in body one's local space ----

        /** @brief Returns a mutable reference to the slider axis expressed in body one's local space.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetSliderAxisInBodyOneLocalSpace(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _sliderAxisInBodyOneLocalSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the slider axis in body one's local space at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetSliderAxisInBodyOneLocalSpaceAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _sliderAxisInBodyOneLocalSpace.size(), "componentIndex out of bounds of _sliderAxisInBodyOneLocalSpace.");
            return _sliderAxisInBodyOneLocalSpace[componentIndex];
        }

        /** @brief Sets the slider axis in body one's local space.
         * @param jointEntity Must have a component.
         * @param sliderAxisBody1 The new local-space slider axis. */
        VE_INLINE void SetSliderAxisInBodyOneLocalSpace(Entity jointEntity, const glm::vec3 &sliderAxisBody1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _sliderAxisInBodyOneLocalSpace[_entityToComponentIndex.find(jointEntity)->second] = sliderAxisBody1;
        }

        /** @brief Sets the slider axis in body one's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @param sliderAxisBody1 The new local-space slider axis. */
        VE_INLINE void SetSliderAxisInBodyOneLocalSpaceAtIndex(size_t componentIndex, const glm::vec3 &sliderAxisBody1) {
            VASSERT(componentIndex < _sliderAxisInBodyOneLocalSpace.size(), "componentIndex out of bounds of _sliderAxisInBodyOneLocalSpace.");
            _sliderAxisInBodyOneLocalSpace[componentIndex] = sliderAxisBody1;
        }

        // ---- Slider axis in world space ----

        /** @brief Returns a mutable reference to the slider axis in world space.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetSliderAxisInWorldSpace(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _sliderAxisInWorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the slider axis in world space at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetSliderAxisInWorldSpaceAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _sliderAxisInWorldSpace.size(), "componentIndex out of bounds of _sliderAxisInWorldSpace.");
            return _sliderAxisInWorldSpace[componentIndex];
        }

        /** @brief Sets the slider axis in world space.
         * @param jointEntity Must have a component.
         * @param sliderAxisWorld The new world-space slider axis. */
        VE_INLINE void SetSliderAxisInWorldSpace(Entity jointEntity, const glm::vec3 &sliderAxisWorld) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _sliderAxisInWorldSpace[_entityToComponentIndex.find(jointEntity)->second] = sliderAxisWorld;
        }

        /** @brief Sets the slider axis in world space at the given index.
         * @param componentIndex Must be in bounds.
         * @param sliderAxisWorld The new world-space slider axis. */
        VE_INLINE void SetSliderAxisInWorldSpaceAtIndex(size_t componentIndex, const glm::vec3 &sliderAxisWorld) {
            VASSERT(componentIndex < _sliderAxisInWorldSpace.size(), "componentIndex out of bounds of _sliderAxisInWorldSpace.");
            _sliderAxisInWorldSpace[componentIndex] = sliderAxisWorld;
        }

        // ---- r1 world space ----

        /** @brief Returns the world-space vector from body one's centre of mass to the joint anchor (r1).
         * @param jointEntity Must have a component.
         * @returns Const reference to the world-space r1 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR1World(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r1WorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the world-space r1 lever arm at the given index.
         * @param componentIndex Must be in bounds.
         * @returns Const reference to the world-space r1 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR1WorldAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _r1WorldSpace.size(), "componentIndex out of bounds of _r1WorldSpace.");
            return _r1WorldSpace[componentIndex];
        }

        /** @brief Sets the world-space r1 lever arm.
         * @param jointEntity Must have a component.
         * @param r1World The new world-space r1 vector. */
        VE_INLINE void SetR1World(Entity jointEntity, const glm::vec3 &r1World) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r1WorldSpace[_entityToComponentIndex.find(jointEntity)->second] = r1World;
        }

        /** @brief Sets the world-space r1 lever arm at the given index.
         * @param componentIndex Must be in bounds.
         * @param r1World The new world-space r1 vector. */
        VE_INLINE void SetR1WorldAtIndex(size_t componentIndex, const glm::vec3 &r1World) {
            VASSERT(componentIndex < _r1WorldSpace.size(), "componentIndex out of bounds of _r1WorldSpace.");
            _r1WorldSpace[componentIndex] = r1World;
        }

        // ---- r2 world space ----

        /** @brief Returns the world-space vector from body two's centre of mass to the joint anchor (r2).
         * @param jointEntity Must have a component.
         * @returns Const reference to the world-space r2 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR2World(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r2WorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the world-space r2 lever arm at the given index.
         * @param componentIndex Must be in bounds.
         * @returns Const reference to the world-space r2 lever arm. */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetR2WorldAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _r2WorldSpace.size(), "componentIndex out of bounds of _r2WorldSpace.");
            return _r2WorldSpace[componentIndex];
        }

        /** @brief Sets the world-space r2 lever arm.
         * @param jointEntity Must have a component.
         * @param r2World The new world-space r2 vector. */
        VE_INLINE void SetR2World(Entity jointEntity, const glm::vec3 &r2World) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r2WorldSpace[_entityToComponentIndex.find(jointEntity)->second] = r2World;
        }

        /** @brief Sets the world-space r2 lever arm at the given index.
         * @param componentIndex Must be in bounds.
         * @param r2World The new world-space r2 vector. */
        VE_INLINE void SetR2WorldAtIndex(size_t componentIndex, const glm::vec3 &r2World) {
            VASSERT(componentIndex < _r2WorldSpace.size(), "componentIndex out of bounds of _r2WorldSpace.");
            _r2WorldSpace[componentIndex] = r2World;
        }

        // ---- n1 — first orthogonal to slider axis ----

        /** @brief Returns a mutable reference to n1, the first vector orthogonal to the slider axis in body one's local space.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetN1(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _n1Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to n1 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetN1AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _n1Vectors.size(), "componentIndex out of bounds of _n1Vectors.");
            return _n1Vectors[componentIndex];
        }

        /** @brief Sets n1, the first vector orthogonal to the slider axis.
         * @param jointEntity Must have a component.
         * @param n1 The new n1 vector. */
        VE_INLINE void SetN1(Entity jointEntity, const glm::vec3 &n1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _n1Vectors[_entityToComponentIndex.find(jointEntity)->second] = n1;
        }

        /** @brief Sets n1 at the given index.
         * @param componentIndex Must be in bounds.
         * @param n1 The new n1 vector. */
        VE_INLINE void SetN1AtIndex(size_t componentIndex, const glm::vec3 &n1) {
            VASSERT(componentIndex < _n1Vectors.size(), "componentIndex out of bounds of _n1Vectors.");
            _n1Vectors[componentIndex] = n1;
        }

        // ---- n2 — second orthogonal to slider axis ----

        /** @brief Returns a mutable reference to n2, the second vector orthogonal to the slider axis and n1 in body one's local space.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetN2(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _n2Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to n2 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetN2AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _n2Vectors.size(), "componentIndex out of bounds of _n2Vectors.");
            return _n2Vectors[componentIndex];
        }

        /** @brief Sets n2, the second vector orthogonal to the slider axis and n1.
         * @param jointEntity Must have a component.
         * @param n2 The new n2 vector. */
        VE_INLINE void SetN2(Entity jointEntity, const glm::vec3 &n2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _n2Vectors[_entityToComponentIndex.find(jointEntity)->second] = n2;
        }

        /** @brief Sets n2 at the given index.
         * @param componentIndex Must be in bounds.
         * @param n2 The new n2 vector. */
        VE_INLINE void SetN2AtIndex(size_t componentIndex, const glm::vec3 &n2) {
            VASSERT(componentIndex < _n2Vectors.size(), "componentIndex out of bounds of _n2Vectors.");
            _n2Vectors[componentIndex] = n2;
        }

        // ---- Lower limit impulse ----

        /** @brief Returns the accumulated impulse for the lower limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetImpulseLowerLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseLowerLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the accumulated impulse for the lower limit constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetImpulseLowerLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _impulseLowerLimits.size(), "componentIndex out of bounds of _impulseLowerLimits.");
            return _impulseLowerLimits[componentIndex];
        }

        /** @brief Sets the accumulated impulse for the lower limit constraint.
         * @param jointEntity Must have a component.
         * @param impulseLowerLimit The new lower limit impulse. */
        VE_INLINE void SetImpulseLowerLimit(Entity jointEntity, f32 impulseLowerLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseLowerLimits[_entityToComponentIndex.find(jointEntity)->second] = impulseLowerLimit;
        }

        /** @brief Sets the accumulated impulse for the lower limit constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseLowerLimit The new lower limit impulse. */
        VE_INLINE void SetImpulseLowerLimitAtIndex(size_t componentIndex, f32 impulseLowerLimit) {
            VASSERT(componentIndex < _impulseLowerLimits.size(), "componentIndex out of bounds of _impulseLowerLimits.");
            _impulseLowerLimits[componentIndex] = impulseLowerLimit;
        }

        // ---- Upper limit impulse ----

        /** @brief Returns the accumulated impulse for the upper limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetImpulseUpperLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseUpperLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the accumulated impulse for the upper limit constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetImpulseUpperLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _impulseUpperLimits.size(), "componentIndex out of bounds of _impulseUpperLimits.");
            return _impulseUpperLimits[componentIndex];
        }

        /** @brief Sets the accumulated impulse for the upper limit constraint.
         * @param jointEntity Must have a component.
         * @param impulseUpperLimit The new upper limit impulse. */
        VE_INLINE void SetImpulseUpperLimit(Entity jointEntity, f32 impulseUpperLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseUpperLimits[_entityToComponentIndex.find(jointEntity)->second] = impulseUpperLimit;
        }

        /** @brief Sets the accumulated impulse for the upper limit constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseUpperLimit The new upper limit impulse. */
        VE_INLINE void SetImpulseUpperLimitAtIndex(size_t componentIndex, f32 impulseUpperLimit) {
            VASSERT(componentIndex < _impulseUpperLimits.size(), "componentIndex out of bounds of _impulseUpperLimits.");
            _impulseUpperLimits[componentIndex] = impulseUpperLimit;
        }

        // ---- Motor impulse ----

        /** @brief Returns the accumulated impulse for the motor constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetImpulseMotor(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseMotors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the accumulated impulse for the motor constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetImpulseMotorAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _impulseMotors.size(), "componentIndex out of bounds of _impulseMotors.");
            return _impulseMotors[componentIndex];
        }

        /** @brief Sets the accumulated impulse for the motor constraint.
         * @param jointEntity Must have a component.
         * @param impulseMotor The new motor impulse. */
        VE_INLINE void SetImpulseMotor(Entity jointEntity, f32 impulseMotor) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseMotors[_entityToComponentIndex.find(jointEntity)->second] = impulseMotor;
        }

        /** @brief Sets the accumulated impulse for the motor constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseMotor The new motor impulse. */
        VE_INLINE void SetImpulseMotorAtIndex(size_t componentIndex, f32 impulseMotor) {
            VASSERT(componentIndex < _impulseMotors.size(), "componentIndex out of bounds of _impulseMotors.");
            _impulseMotors[componentIndex] = impulseMotor;
        }

        // ---- Inverse mass matrix for limits ----

        /** @brief Returns the inverse mass matrix K=JM^-1J^t for the upper and lower limit constraints (scalar).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassMatrixLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the inverse mass matrix for the limit constraints at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _inverseMassMatrixLimits.size(), "componentIndex out of bounds of _inverseMassMatrixLimits.");
            return _inverseMassMatrixLimits[componentIndex];
        }

        /** @brief Sets the inverse mass matrix K for the limit constraints.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrixLimit The new limit inverse mass. */
        VE_INLINE void SetInverseMassMatrixLimit(Entity jointEntity, f32 inverseMassMatrixLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassMatrixLimits[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrixLimit;
        }

        /** @brief Sets the inverse mass matrix K for the limit constraints at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrixLimit The new limit inverse mass. */
        VE_INLINE void SetInverseMassMatrixLimitAtIndex(size_t componentIndex, f32 inverseMassMatrixLimit) {
            VASSERT(componentIndex < _inverseMassMatrixLimits.size(), "componentIndex out of bounds of _inverseMassMatrixLimits.");
            _inverseMassMatrixLimits[componentIndex] = inverseMassMatrixLimit;
        }

        // ---- Inverse mass matrix for motor ----

        /** @brief Returns the inverse mass matrix K=JM^-1J^t for the motor constraint (scalar).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixMotor(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassMatrixMotors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the inverse mass matrix for the motor at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixMotorAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _inverseMassMatrixMotors.size(), "componentIndex out of bounds of _inverseMassMatrixMotors.");
            return _inverseMassMatrixMotors[componentIndex];
        }

        /** @brief Sets the inverse mass matrix K for the motor.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrixMotor The new motor inverse mass. */
        VE_INLINE void SetInverseMassMatrixMotor(Entity jointEntity, f32 inverseMassMatrixMotor) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassMatrixMotors[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrixMotor;
        }

        /** @brief Sets the inverse mass matrix K for the motor at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrixMotor The new motor inverse mass. */
        VE_INLINE void SetInverseMassMatrixMotorAtIndex(size_t componentIndex, f32 inverseMassMatrixMotor) {
            VASSERT(componentIndex < _inverseMassMatrixMotors.size(), "componentIndex out of bounds of _inverseMassMatrixMotors.");
            _inverseMassMatrixMotors[componentIndex] = inverseMassMatrixMotor;
        }

        // ---- Lower limit bias ----

        /** @brief Returns the Baumgarte bias for the lower limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetBiasLowerLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _biasLowerLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the Baumgarte bias for the lower limit constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetBiasLowerLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _biasLowerLimits.size(), "componentIndex out of bounds of _biasLowerLimits.");
            return _biasLowerLimits[componentIndex];
        }

        /** @brief Sets the Baumgarte bias for the lower limit constraint.
         * @param jointEntity Must have a component.
         * @param biasLowerLimit The new lower limit bias. */
        VE_INLINE void SetBiasLowerLimit(Entity jointEntity, f32 biasLowerLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _biasLowerLimits[_entityToComponentIndex.find(jointEntity)->second] = biasLowerLimit;
        }

        /** @brief Sets the Baumgarte bias for the lower limit constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param biasLowerLimit The new lower limit bias. */
        VE_INLINE void SetBiasLowerLimitAtIndex(size_t componentIndex, f32 biasLowerLimit) {
            VASSERT(componentIndex < _biasLowerLimits.size(), "componentIndex out of bounds of _biasLowerLimits.");
            _biasLowerLimits[componentIndex] = biasLowerLimit;
        }

        // ---- Upper limit bias ----

        /** @brief Returns the Baumgarte bias for the upper limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetBiasUpperLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _biasUpperLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the Baumgarte bias for the upper limit constraint at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetBiasUpperLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _biasUpperLimits.size(), "componentIndex out of bounds of _biasUpperLimits.");
            return _biasUpperLimits[componentIndex];
        }

        /** @brief Sets the Baumgarte bias for the upper limit constraint.
         * @param jointEntity Must have a component.
         * @param biasUpperLimit The new upper limit bias. */
        VE_INLINE void SetBiasUpperLimit(Entity jointEntity, f32 biasUpperLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _biasUpperLimits[_entityToComponentIndex.find(jointEntity)->second] = biasUpperLimit;
        }

        /** @brief Sets the Baumgarte bias for the upper limit constraint at the given index.
         * @param componentIndex Must be in bounds.
         * @param biasUpperLimit The new upper limit bias. */
        VE_INLINE void SetBiasUpperLimitAtIndex(size_t componentIndex, f32 biasUpperLimit) {
            VASSERT(componentIndex < _biasUpperLimits.size(), "componentIndex out of bounds of _biasUpperLimits.");
            _biasUpperLimits[componentIndex] = biasUpperLimit;
        }

        // ---- Limit enabled flag ----

        /** @brief Returns true if the translation limits are enabled.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE bool GetIsLimitEnabled(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_limitEnabledFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns true if the translation limits are enabled at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE bool GetIsLimitEnabledAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _limitEnabledFlags.size(), "componentIndex out of bounds of _limitEnabledFlags.");
            return static_cast<bool>(_limitEnabledFlags[componentIndex]);
        }

        /** @brief Sets whether the translation limits are enabled.
         * @param jointEntity Must have a component.
         * @param isLimitEnabled True to enable limits. */
        VE_INLINE void SetIsLimitEnabled(Entity jointEntity, bool isLimitEnabled) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _limitEnabledFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isLimitEnabled);
        }

        /** @brief Sets whether the translation limits are enabled at the given index.
         * @param componentIndex Must be in bounds.
         * @param isLimitEnabled True to enable limits. */
        VE_INLINE void SetIsLimitEnabledAtIndex(size_t componentIndex, bool isLimitEnabled) {
            VASSERT(componentIndex < _limitEnabledFlags.size(), "componentIndex out of bounds of _limitEnabledFlags.");
            _limitEnabledFlags[componentIndex] = static_cast<u8>(isLimitEnabled);
        }

        // ---- Motor enabled flag ----

        /** @brief Returns true if the linear motor is enabled.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE bool GetIsMotorEnabled(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_motorEnabledFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns true if the linear motor is enabled at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE bool GetIsMotorEnabledAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _motorEnabledFlags.size(), "componentIndex out of bounds of _motorEnabledFlags.");
            return static_cast<bool>(_motorEnabledFlags[componentIndex]);
        }

        /** @brief Sets whether the linear motor is enabled.
         * @param jointEntity Must have a component.
         * @param isMotorEnabled True to enable the motor. */
        VE_INLINE void SetIsMotorEnabled(Entity jointEntity, bool isMotorEnabled) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _motorEnabledFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isMotorEnabled);
        }

        /** @brief Sets whether the linear motor is enabled at the given index.
         * @param componentIndex Must be in bounds.
         * @param isMotorEnabled True to enable the motor. */
        VE_INLINE void SetIsMotorEnabledAtIndex(size_t componentIndex, bool isMotorEnabled) {
            VASSERT(componentIndex < _motorEnabledFlags.size(), "componentIndex out of bounds of _motorEnabledFlags.");
            _motorEnabledFlags[componentIndex] = static_cast<u8>(isMotorEnabled);
        }

        // ---- Lower limit ----

        /** @brief Returns the minimum allowed translation distance along the slider axis.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetLowerLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _lowerLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the lower limit at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetLowerLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _lowerLimits.size(), "componentIndex out of bounds of _lowerLimits.");
            return _lowerLimits[componentIndex];
        }

        /** @brief Sets the minimum allowed translation distance along the slider axis.
         * @param jointEntity Must have a component.
         * @param lowerLimit The new lower limit value. */
        VE_INLINE void SetLowerLimit(Entity jointEntity, f32 lowerLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _lowerLimits[_entityToComponentIndex.find(jointEntity)->second] = lowerLimit;
        }

        /** @brief Sets the lower limit at the given index.
         * @param componentIndex Must be in bounds.
         * @param lowerLimit The new lower limit value. */
        VE_INLINE void SetLowerLimitAtIndex(size_t componentIndex, f32 lowerLimit) {
            VASSERT(componentIndex < _lowerLimits.size(), "componentIndex out of bounds of _lowerLimits.");
            _lowerLimits[componentIndex] = lowerLimit;
        }

        // ---- Upper limit ----

        /** @brief Returns the maximum allowed translation distance along the slider axis.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetUpperLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _upperLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the upper limit at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetUpperLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _upperLimits.size(), "componentIndex out of bounds of _upperLimits.");
            return _upperLimits[componentIndex];
        }

        /** @brief Sets the maximum allowed translation distance along the slider axis.
         * @param jointEntity Must have a component.
         * @param upperLimit The new upper limit value. */
        VE_INLINE void SetUpperLimit(Entity jointEntity, f32 upperLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _upperLimits[_entityToComponentIndex.find(jointEntity)->second] = upperLimit;
        }

        /** @brief Sets the upper limit at the given index.
         * @param componentIndex Must be in bounds.
         * @param upperLimit The new upper limit value. */
        VE_INLINE void SetUpperLimitAtIndex(size_t componentIndex, f32 upperLimit) {
            VASSERT(componentIndex < _upperLimits.size(), "componentIndex out of bounds of _upperLimits.");
            _upperLimits[componentIndex] = upperLimit;
        }

        // ---- Lower limit violated flag ----

        /** @brief Returns true if the lower translation limit is currently violated.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE bool GetIsLowerLimitViolated(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_lowerLimitViolatedFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns true if the lower limit is violated at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE bool GetIsLowerLimitViolatedAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _lowerLimitViolatedFlags.size(), "componentIndex out of bounds of _lowerLimitViolatedFlags.");
            return static_cast<bool>(_lowerLimitViolatedFlags[componentIndex]);
        }

        /** @brief Sets whether the lower limit is violated.
         * @param jointEntity Must have a component.
         * @param isLowerLimitViolated True if the lower limit is currently violated. */
        VE_INLINE void SetIsLowerLimitViolated(Entity jointEntity, bool isLowerLimitViolated) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _lowerLimitViolatedFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isLowerLimitViolated);
        }

        /** @brief Sets whether the lower limit is violated at the given index.
         * @param componentIndex Must be in bounds.
         * @param isLowerLimitViolated True if the lower limit is currently violated. */
        VE_INLINE void SetIsLowerLimitViolatedAtIndex(size_t componentIndex, bool isLowerLimitViolated) {
            VASSERT(componentIndex < _lowerLimitViolatedFlags.size(), "componentIndex out of bounds of _lowerLimitViolatedFlags.");
            _lowerLimitViolatedFlags[componentIndex] = static_cast<u8>(isLowerLimitViolated);
        }

        // ---- Upper limit violated flag ----

        /** @brief Returns true if the upper translation limit is currently violated.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE bool GetIsUpperLimitViolated(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_upperLimitViolatedFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns true if the upper limit is violated at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE bool GetIsUpperLimitViolatedAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _upperLimitViolatedFlags.size(), "componentIndex out of bounds of _upperLimitViolatedFlags.");
            return static_cast<bool>(_upperLimitViolatedFlags[componentIndex]);
        }

        /** @brief Sets whether the upper limit is violated.
         * @param jointEntity Must have a component.
         * @param isUpperLimitViolated True if the upper limit is currently violated. */
        VE_INLINE void SetIsUpperLimitViolated(Entity jointEntity, bool isUpperLimitViolated) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _upperLimitViolatedFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isUpperLimitViolated);
        }

        /** @brief Sets whether the upper limit is violated at the given index.
         * @param componentIndex Must be in bounds.
         * @param isUpperLimitViolated True if the upper limit is currently violated. */
        VE_INLINE void SetIsUpperLimitViolatedAtIndex(size_t componentIndex, bool isUpperLimitViolated) {
            VASSERT(componentIndex < _upperLimitViolatedFlags.size(), "componentIndex out of bounds of _upperLimitViolatedFlags.");
            _upperLimitViolatedFlags[componentIndex] = static_cast<u8>(isUpperLimitViolated);
        }

        // ---- Motor speed ----

        /** @brief Returns the target speed of the linear motor (m/s).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetMotorSpeed(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _motorSpeeds[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the motor speed at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetMotorSpeedAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _motorSpeeds.size(), "componentIndex out of bounds of _motorSpeeds.");
            return _motorSpeeds[componentIndex];
        }

        /** @brief Sets the target speed of the linear motor.
         * @param jointEntity Must have a component.
         * @param motorSpeed The new motor speed in m/s. */
        VE_INLINE void SetMotorSpeed(Entity jointEntity, f32 motorSpeed) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _motorSpeeds[_entityToComponentIndex.find(jointEntity)->second] = motorSpeed;
        }

        /** @brief Sets the motor speed at the given index.
         * @param componentIndex Must be in bounds.
         * @param motorSpeed The new motor speed in m/s. */
        VE_INLINE void SetMotorSpeedAtIndex(size_t componentIndex, f32 motorSpeed) {
            VASSERT(componentIndex < _motorSpeeds.size(), "componentIndex out of bounds of _motorSpeeds.");
            _motorSpeeds[componentIndex] = motorSpeed;
        }

        // ---- Max motor force ----

        /** @brief Returns the maximum force the motor may exert to reach the target speed (N).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetMaxMotorForce(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _maxMotorForces[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the maximum motor force at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetMaxMotorForceAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _maxMotorForces.size(), "componentIndex out of bounds of _maxMotorForces.");
            return _maxMotorForces[componentIndex];
        }

        /** @brief Sets the maximum force the motor may exert.
         * @param jointEntity Must have a component.
         * @param maxMotorForce The new maximum motor force in Newtons. */
        VE_INLINE void SetMaxMotorForce(Entity jointEntity, f32 maxMotorForce) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _maxMotorForces[_entityToComponentIndex.find(jointEntity)->second] = maxMotorForce;
        }

        /** @brief Sets the maximum motor force at the given index.
         * @param componentIndex Must be in bounds.
         * @param maxMotorForce The new maximum motor force in Newtons. */
        VE_INLINE void SetMaxMotorForceAtIndex(size_t componentIndex, f32 maxMotorForce) {
            VASSERT(componentIndex < _maxMotorForces.size(), "componentIndex out of bounds of _maxMotorForces.");
            _maxMotorForces[componentIndex] = maxMotorForce;
        }

        // ---- r2 × n1 ----

        /** @brief Returns a mutable reference to the precomputed cross product of r2 and n1.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR2CrossN1(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r2CrossN1Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to r2 × n1 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR2CrossN1AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _r2CrossN1Vectors.size(), "componentIndex out of bounds of _r2CrossN1Vectors.");
            return _r2CrossN1Vectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of r2 and n1.
         * @param jointEntity Must have a component.
         * @param r2CrossN1 The new r2 × n1 vector. */
        VE_INLINE void SetR2CrossN1(Entity jointEntity, const glm::vec3 &r2CrossN1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r2CrossN1Vectors[_entityToComponentIndex.find(jointEntity)->second] = r2CrossN1;
        }

        /** @brief Sets r2 × n1 at the given index.
         * @param componentIndex Must be in bounds.
         * @param r2CrossN1 The new r2 × n1 vector. */
        VE_INLINE void SetR2CrossN1AtIndex(size_t componentIndex, const glm::vec3 &r2CrossN1) {
            VASSERT(componentIndex < _r2CrossN1Vectors.size(), "componentIndex out of bounds of _r2CrossN1Vectors.");
            _r2CrossN1Vectors[componentIndex] = r2CrossN1;
        }

        // ---- r2 × n2 ----

        /** @brief Returns a mutable reference to the precomputed cross product of r2 and n2.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR2CrossN2(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r2CrossN2Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to r2 × n2 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR2CrossN2AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _r2CrossN2Vectors.size(), "componentIndex out of bounds of _r2CrossN2Vectors.");
            return _r2CrossN2Vectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of r2 and n2.
         * @param jointEntity Must have a component.
         * @param r2CrossN2 The new r2 × n2 vector. */
        VE_INLINE void SetR2CrossN2(Entity jointEntity, const glm::vec3 &r2CrossN2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r2CrossN2Vectors[_entityToComponentIndex.find(jointEntity)->second] = r2CrossN2;
        }

        /** @brief Sets r2 × n2 at the given index.
         * @param componentIndex Must be in bounds.
         * @param r2CrossN2 The new r2 × n2 vector. */
        VE_INLINE void SetR2CrossN2AtIndex(size_t componentIndex, const glm::vec3 &r2CrossN2) {
            VASSERT(componentIndex < _r2CrossN2Vectors.size(), "componentIndex out of bounds of _r2CrossN2Vectors.");
            _r2CrossN2Vectors[componentIndex] = r2CrossN2;
        }

        // ---- r2 × slider axis ----

        /** @brief Returns a mutable reference to the precomputed cross product of r2 and the slider axis.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR2CrossSliderAxis(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r2CrossSliderAxisVectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to r2 × slider axis at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR2CrossSliderAxisAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _r2CrossSliderAxisVectors.size(), "componentIndex out of bounds of _r2CrossSliderAxisVectors.");
            return _r2CrossSliderAxisVectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of r2 and the slider axis.
         * @param jointEntity Must have a component.
         * @param r2CrossSliderAxis The new r2 × slider axis vector. */
        VE_INLINE void SetR2CrossSliderAxis(Entity jointEntity, const glm::vec3 &r2CrossSliderAxis) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r2CrossSliderAxisVectors[_entityToComponentIndex.find(jointEntity)->second] = r2CrossSliderAxis;
        }

        /** @brief Sets r2 × slider axis at the given index.
         * @param componentIndex Must be in bounds.
         * @param r2CrossSliderAxis The new r2 × slider axis vector. */
        VE_INLINE void SetR2CrossSliderAxisAtIndex(size_t componentIndex, const glm::vec3 &r2CrossSliderAxis) {
            VASSERT(componentIndex < _r2CrossSliderAxisVectors.size(), "componentIndex out of bounds of _r2CrossSliderAxisVectors.");
            _r2CrossSliderAxisVectors[componentIndex] = r2CrossSliderAxis;
        }

        // ---- (r1 + u) × n1 ----

        /** @brief Returns a mutable reference to the precomputed cross product of (r1 + u) and n1.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR1PlusUCrossN1(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r1PlusUCrossN1Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to (r1 + u) × n1 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR1PlusUCrossN1AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _r1PlusUCrossN1Vectors.size(), "componentIndex out of bounds of _r1PlusUCrossN1Vectors.");
            return _r1PlusUCrossN1Vectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of (r1 + u) and n1.
         * @param jointEntity Must have a component.
         * @param r1PlusUCrossN1 The new (r1 + u) × n1 vector. */
        VE_INLINE void SetR1PlusUCrossN1(Entity jointEntity, const glm::vec3 &r1PlusUCrossN1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r1PlusUCrossN1Vectors[_entityToComponentIndex.find(jointEntity)->second] = r1PlusUCrossN1;
        }

        /** @brief Sets (r1 + u) × n1 at the given index.
         * @param componentIndex Must be in bounds.
         * @param r1PlusUCrossN1 The new (r1 + u) × n1 vector. */
        VE_INLINE void SetR1PlusUCrossN1AtIndex(size_t componentIndex, const glm::vec3 &r1PlusUCrossN1) {
            VASSERT(componentIndex < _r1PlusUCrossN1Vectors.size(), "componentIndex out of bounds of _r1PlusUCrossN1Vectors.");
            _r1PlusUCrossN1Vectors[componentIndex] = r1PlusUCrossN1;
        }

        // ---- (r1 + u) × n2 ----

        /** @brief Returns a mutable reference to the precomputed cross product of (r1 + u) and n2.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR1PlusUCrossN2(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r1PlusUCrossN2Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to (r1 + u) × n2 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR1PlusUCrossN2AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _r1PlusUCrossN2Vectors.size(), "componentIndex out of bounds of _r1PlusUCrossN2Vectors.");
            return _r1PlusUCrossN2Vectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of (r1 + u) and n2.
         * @param jointEntity Must have a component.
         * @param r1PlusUCrossN2 The new (r1 + u) × n2 vector. */
        VE_INLINE void SetR1PlusUCrossN2(Entity jointEntity, const glm::vec3 &r1PlusUCrossN2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r1PlusUCrossN2Vectors[_entityToComponentIndex.find(jointEntity)->second] = r1PlusUCrossN2;
        }

        /** @brief Sets (r1 + u) × n2 at the given index.
         * @param componentIndex Must be in bounds.
         * @param r1PlusUCrossN2 The new (r1 + u) × n2 vector. */
        VE_INLINE void SetR1PlusUCrossN2AtIndex(size_t componentIndex, const glm::vec3 &r1PlusUCrossN2) {
            VASSERT(componentIndex < _r1PlusUCrossN2Vectors.size(), "componentIndex out of bounds of _r1PlusUCrossN2Vectors.");
            _r1PlusUCrossN2Vectors[componentIndex] = r1PlusUCrossN2;
        }

        // ---- (r1 + u) × slider axis ----

        /** @brief Returns a mutable reference to the precomputed cross product of (r1 + u) and the slider axis.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR1PlusUCrossSliderAxis(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _r1PlusUCrossSliderAxisVectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to (r1 + u) × slider axis at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetR1PlusUCrossSliderAxisAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _r1PlusUCrossSliderAxisVectors.size(), "componentIndex out of bounds of _r1PlusUCrossSliderAxisVectors.");
            return _r1PlusUCrossSliderAxisVectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of (r1 + u) and the slider axis.
         * @param jointEntity Must have a component.
         * @param r1PlusUCrossSliderAxis The new (r1 + u) × slider axis vector. */
        VE_INLINE void SetR1PlusUCrossSliderAxis(Entity jointEntity, const glm::vec3 &r1PlusUCrossSliderAxis) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _r1PlusUCrossSliderAxisVectors[_entityToComponentIndex.find(jointEntity)->second] = r1PlusUCrossSliderAxis;
        }

        /** @brief Sets (r1 + u) × slider axis at the given index.
         * @param componentIndex Must be in bounds.
         * @param r1PlusUCrossSliderAxis The new (r1 + u) × slider axis vector. */
        VE_INLINE void SetR1PlusUCrossSliderAxisAtIndex(size_t componentIndex, const glm::vec3 &r1PlusUCrossSliderAxis) {
            VASSERT(componentIndex < _r1PlusUCrossSliderAxisVectors.size(), "componentIndex out of bounds of _r1PlusUCrossSliderAxisVectors.");
            _r1PlusUCrossSliderAxisVectors[componentIndex] = r1PlusUCrossSliderAxis;
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Non-owning pointers to the runtime SliderJoint objects, one per joint entity. */
        std::vector<SliderJoint *> _joints;

        /** @brief Anchor points for each joint expressed in body one's local space. Used each frame to recompute the world-space lever arm r1. */
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyOne;

        /** @brief Anchor points for each joint expressed in body two's local space. Used each frame to recompute the world-space lever arm r2. */
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyTwo;

        /** @brief World-space inertia tensors of body one, cached each simulation step for use in the constraint solver. */
        std::vector<glm::mat3> _bodyOneInertiaTensorsInWorldSpace;

        /** @brief World-space inertia tensors of body two, cached each simulation step for use in the constraint solver. */
        std::vector<glm::mat3> _bodyTwoInertiaTensorsInWorldSpace;

        /** @brief Accumulated 2-DOF translational constraint impulses (perpendicular to slider axis), warm-started each step. */
        std::vector<glm::vec2> _impulseTranslations;

        /** @brief Accumulated 3-DOF rotational constraint impulses, warm-started each step. */
        std::vector<glm::vec3> _impulseRotations;

        /** @brief Effective inverse mass matrices K for the 2-DOF translational constraint, computed each simulation step. */
        std::vector<glm::mat3> _inverseMassTranslationMatrices;

        /** @brief Effective inverse mass matrices K for the 3-DOF rotational constraint, computed each simulation step. */
        std::vector<glm::mat3> _inverseMassRotationMatrices;

        /** @brief Baumgarte position-correction bias vectors for the 2-DOF translational constraint, computed each step. */
        std::vector<glm::vec2> _translationBiases;

        /** @brief Baumgarte position-correction bias vectors for the 3-DOF rotational constraint, computed each step. */
        std::vector<glm::vec3> _rotationBiases;

        /** @brief Inverses of the initial relative orientations, computed once at joint creation and used to enforce the rotational constraint. */
        std::vector<glm::quat> _initialOrientationDifferenceInverses;

        /** @brief Slider axis expressed in body one's local space. Rotated to world space each step to obtain _sliderAxisInWorldSpace. */
        std::vector<glm::vec3> _sliderAxisInBodyOneLocalSpace;

        /** @brief Slider axis expressed in world space, recomputed each simulation step from body one's current orientation. */
        std::vector<glm::vec3> _sliderAxisInWorldSpace;

        /** @brief World-space vectors from body one's centre of mass to the joint anchor (r1), recomputed each simulation step. */
        std::vector<glm::vec3> _r1WorldSpace;

        /** @brief World-space vectors from body two's centre of mass to the joint anchor (r2), recomputed each simulation step. */
        std::vector<glm::vec3> _r2WorldSpace;

        /** @brief First vectors orthogonal to the slider axis in body one's local space, used to form the 2-DOF translational constraint Jacobian. */
        std::vector<glm::vec3> _n1Vectors;

        /** @brief Second vectors orthogonal to the slider axis and n1 in body one's local space, used to form the 2-DOF translational constraint Jacobian. */
        std::vector<glm::vec3> _n2Vectors;

        /** @brief Accumulated impulses for the lower translation limit constraint, warm-started each step. */
        std::vector<f32> _impulseLowerLimits;

        /** @brief Accumulated impulses for the upper translation limit constraint, warm-started each step. */
        std::vector<f32> _impulseUpperLimits;

        /** @brief Accumulated impulses for the linear motor constraint, warm-started each step. */
        std::vector<f32> _impulseMotors;

        /** @brief Scalar effective inverse masses K=JM^-1J^t shared by the lower and upper limit constraints. */
        std::vector<f32> _inverseMassMatrixLimits;

        /** @brief Scalar effective inverse masses K=JM^-1J^t for the linear motor constraint. */
        std::vector<f32> _inverseMassMatrixMotors;

        /** @brief Baumgarte position-correction biases for the lower translation limit, computed each step when the limit is active. */
        std::vector<f32> _biasLowerLimits;

        /** @brief Baumgarte position-correction biases for the upper translation limit, computed each step when the limit is active. */
        std::vector<f32> _biasUpperLimits;

        /** @brief Flags indicating whether the translation limits are enabled (stored as u8 for cache efficiency). */
        std::vector<u8> _limitEnabledFlags;

        /** @brief Flags indicating whether the linear motor is enabled (stored as u8 for cache efficiency). */
        std::vector<u8> _motorEnabledFlags;

        /** @brief Minimum allowed translation distances along the slider axis. */
        std::vector<f32> _lowerLimits;

        /** @brief Maximum allowed translation distances along the slider axis. */
        std::vector<f32> _upperLimits;

        /** @brief Flags set to true when the body has translated past the lower limit; used to select the correct constraint sign. */
        std::vector<u8> _lowerLimitViolatedFlags;

        /** @brief Flags set to true when the body has translated past the upper limit; used to select the correct constraint sign. */
        std::vector<u8> _upperLimitViolatedFlags;

        /** @brief Target speeds for the linear motor (m/s). */
        std::vector<f32> _motorSpeeds;

        /** @brief Maximum forces the linear motor may exert to reach the target speed (N). */
        std::vector<f32> _maxMotorForces;

        /** @brief Precomputed cross products of r2 and n1, cached each step for use in the constraint Jacobian. */
        std::vector<glm::vec3> _r2CrossN1Vectors;

        /** @brief Precomputed cross products of r2 and n2, cached each step for use in the constraint Jacobian. */
        std::vector<glm::vec3> _r2CrossN2Vectors;

        /** @brief Precomputed cross products of r2 and the world-space slider axis, used in the limit/motor Jacobian. */
        std::vector<glm::vec3> _r2CrossSliderAxisVectors;

        /** @brief Precomputed cross products of (r1 + u) and n1, cached each step for use in the constraint Jacobian. */
        std::vector<glm::vec3> _r1PlusUCrossN1Vectors;

        /** @brief Precomputed cross products of (r1 + u) and n2, cached each step for use in the constraint Jacobian. */
        std::vector<glm::vec3> _r1PlusUCrossN2Vectors;

        /** @brief Precomputed cross products of (r1 + u) and the world-space slider axis, used in the limit/motor Jacobian. */
        std::vector<glm::vec3> _r1PlusUCrossSliderAxisVectors;
    };

} // namespace Vulkyrie
