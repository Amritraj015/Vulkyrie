#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "physics/constraint/hinge_joint.h"
#include "physics/components/component_store.h"

namespace Vulkyrie {

    /** @brief Initialisation data carried in AddComponent for a hinge joint. */
    struct HingeJointComponent final {
        /** @brief Minimum allowed rotation angle in radians. */
        f32 LowerLimit;

        /** @brief Maximum allowed rotation angle in radians. */
        f32 UpperLimit;

        /** @brief Target angular speed of the motor (rad/s). */
        f32 MotorSpeed;

        /** @brief Maximum torque the motor may exert to reach the target speed (N·m). */
        f32 MaxMotorTorque;

        /** @brief True if the angular limits are active. */
        bool LimitEnabled;

        /** @brief True if the rotational motor is active. */
        bool MotorEnabled;

        HingeJointComponent(bool limitEnabled, bool motorEnabled, f32 lowerLimit, f32 upperLimit, f32 motorSpeed, f32 maxMotorTorque)
            : LowerLimit(lowerLimit)
            , UpperLimit(upperLimit)
            , MotorSpeed(motorSpeed)
            , MaxMotorTorque(maxMotorTorque)
            , LimitEnabled(limitEnabled)
            , MotorEnabled(motorEnabled) {
        }
    };

    /** @brief Stores hinge joint components using a Structure-of-Arrays (SoA) layout.
     *
     * Each entity that represents a hinge joint owns exactly one component. The store holds all
     * constraint solver state needed per simulation step: anchor points in local space, world-space
     * lever arms, inertia tensors, translational (3-DOF) and rotational (2-DOF) impulses with their
     * inverse mass matrices and bias vectors, the initial inverse orientation difference, the hinge
     * axis in local and world space, precomputed helper cross products (b2 × a1, c2 × a1), and
     * limit/motor state. The dense active-zone invariant from ComponentStore is maintained: active
     * components occupy indices [0, _activeCount) and inactive ones [_activeCount, size). All
     * parallel arrays are kept in sync by swapComponents. */
    class HingeJointComponentStore : public ComponentStore {
    public:
        /** @brief Constructs an instance of HingeJointComponentStore and reserves initial storage for all parallel arrays. */
        HingeJointComponentStore();

        VE_DELETE_MOVE_AND_COPY(HingeJointComponentStore);

        /** @brief Destructor for HingeJointComponentStore. */
        ~HingeJointComponentStore() override = default;

        /** @brief Adds a component for the specified joint entity.
         * @param jointEntity The entity to which the component will be added. Must not already have a component.
         * @param component Initialisation data for the component.
         * @param active Whether the joint entity is currently active. */
        void AddComponent(Entity jointEntity, const HingeJointComponent &component, bool active);

        // ---- Joint pointer ----

        /** @brief Returns the HingeJoint pointer for the given entity.
         * @param jointEntity Must have a component.
         * @returns Non-owning pointer to the HingeJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE HingeJoint *GetJoint(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _joints[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the HingeJoint pointer at the given component index.
         * @param componentIndex Must be in bounds.
         * @returns Non-owning pointer to the HingeJoint, or nullptr if not yet assigned. */
        [[nodiscard]] VE_INLINE HingeJoint *GetJointAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _joints.size(), "componentIndex out of bounds of _joints.");
            return _joints[componentIndex];
        }

        /** @brief Sets the HingeJoint pointer for the given entity.
         * @param jointEntity Must have a component.
         * @param joint Non-owning pointer to associate with the entity. */
        VE_INLINE void SetJoint(Entity jointEntity, HingeJoint *joint) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _joints[_entityToComponentIndex.find(jointEntity)->second] = joint;
        }

        /** @brief Sets the HingeJoint pointer at the given component index.
         * @param componentIndex Must be in bounds.
         * @param joint Non-owning pointer to store at the index. */
        VE_INLINE void SetJointAtIndex(size_t componentIndex, HingeJoint *joint) {
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

        // ---- Translational impulse (3-DOF) ----

        /** @brief Returns a mutable reference to the accumulated 3-DOF translational impulse.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseTranslation(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseTranslations[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the accumulated 3-DOF translational impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetImpulseTranslationAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulseTranslations.size(), "componentIndex out of bounds of _impulseTranslations.");
            return _impulseTranslations[componentIndex];
        }

        /** @brief Sets the accumulated 3-DOF translational impulse.
         * @param jointEntity Must have a component.
         * @param impulseTranslation The new translational impulse. */
        VE_INLINE void SetImpulseTranslation(Entity jointEntity, const glm::vec3 &impulseTranslation) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseTranslations[_entityToComponentIndex.find(jointEntity)->second] = impulseTranslation;
        }

        /** @brief Sets the accumulated 3-DOF translational impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseTranslation The new translational impulse. */
        VE_INLINE void SetImpulseTranslationAtIndex(size_t componentIndex, const glm::vec3 &impulseTranslation) {
            VASSERT(componentIndex < _impulseTranslations.size(), "componentIndex out of bounds of _impulseTranslations.");
            _impulseTranslations[componentIndex] = impulseTranslation;
        }

        // ---- Rotational impulse (2-DOF) ----

        /** @brief Returns a mutable reference to the accumulated 2-DOF rotational impulse (constraining the two DOF perpendicular to the hinge axis).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetImpulseRotation(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseRotations[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the accumulated 2-DOF rotational impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetImpulseRotationAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _impulseRotations.size(), "componentIndex out of bounds of _impulseRotations.");
            return _impulseRotations[componentIndex];
        }

        /** @brief Sets the accumulated 2-DOF rotational impulse.
         * @param jointEntity Must have a component.
         * @param impulseRotation The new 2-DOF rotational impulse. */
        VE_INLINE void SetImpulseRotation(Entity jointEntity, const glm::vec2 &impulseRotation) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseRotations[_entityToComponentIndex.find(jointEntity)->second] = impulseRotation;
        }

        /** @brief Sets the accumulated 2-DOF rotational impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseRotation The new 2-DOF rotational impulse. */
        VE_INLINE void SetImpulseRotationAtIndex(size_t componentIndex, const glm::vec2 &impulseRotation) {
            VASSERT(componentIndex < _impulseRotations.size(), "componentIndex out of bounds of _impulseRotations.");
            _impulseRotations[componentIndex] = impulseRotation;
        }

        // ---- Translational inverse mass matrix (3x3) ----

        /** @brief Returns a mutable reference to the effective inverse mass matrix K for the 3-DOF translational constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassTranslationMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassTranslationMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the translational inverse mass matrix at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::mat3 &GetInverseMassTranslationMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassTranslationMatrices.size(), "componentIndex out of bounds of _inverseMassTranslationMatrices.");
            return _inverseMassTranslationMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix K for the 3-DOF translational constraint.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrix The new 3x3 translational inverse mass matrix. */
        VE_INLINE void SetInverseMassTranslationMatrix(Entity jointEntity, const glm::mat3 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassTranslationMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the translational inverse mass matrix at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrix The new 3x3 translational inverse mass matrix. */
        VE_INLINE void SetInverseMassTranslationMatrixAtIndex(size_t componentIndex, const glm::mat3 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassTranslationMatrices.size(), "componentIndex out of bounds of _inverseMassTranslationMatrices.");
            _inverseMassTranslationMatrices[componentIndex] = inverseMassMatrix;
        }

        // ---- Rotational inverse mass matrix (2x2) ----

        /** @brief Returns a mutable reference to the effective inverse mass matrix K for the 2-DOF rotational constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::mat2 &GetInverseMassRotationMatrix(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassRotationMatrices[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the rotational inverse mass matrix at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::mat2 &GetInverseMassRotationMatrixAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _inverseMassRotationMatrices.size(), "componentIndex out of bounds of _inverseMassRotationMatrices.");
            return _inverseMassRotationMatrices[componentIndex];
        }

        /** @brief Sets the effective inverse mass matrix K for the 2-DOF rotational constraint.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrix The new 2x2 rotational inverse mass matrix. */
        VE_INLINE void SetInverseMassRotationMatrix(Entity jointEntity, const glm::mat2 &inverseMassMatrix) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassRotationMatrices[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrix;
        }

        /** @brief Sets the rotational inverse mass matrix at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrix The new 2x2 rotational inverse mass matrix. */
        VE_INLINE void SetInverseMassRotationMatrixAtIndex(size_t componentIndex, const glm::mat2 &inverseMassMatrix) {
            VASSERT(componentIndex < _inverseMassRotationMatrices.size(), "componentIndex out of bounds of _inverseMassRotationMatrices.");
            _inverseMassRotationMatrices[componentIndex] = inverseMassMatrix;
        }

        // ---- Translation bias (Baumgarte) ----

        /** @brief Returns a mutable reference to the Baumgarte translation bias vector (3-DOF).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetTranslationBias(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _translationBiases[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the Baumgarte translation bias vector at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetTranslationBiasAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _translationBiases.size(), "componentIndex out of bounds of _translationBiases.");
            return _translationBiases[componentIndex];
        }

        /** @brief Sets the Baumgarte translation bias vector.
         * @param jointEntity Must have a component.
         * @param translationBias The new translation bias. */
        VE_INLINE void SetTranslationBias(Entity jointEntity, const glm::vec3 &translationBias) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _translationBiases[_entityToComponentIndex.find(jointEntity)->second] = translationBias;
        }

        /** @brief Sets the Baumgarte translation bias vector at the given index.
         * @param componentIndex Must be in bounds.
         * @param translationBias The new translation bias. */
        VE_INLINE void SetTranslationBiasAtIndex(size_t componentIndex, const glm::vec3 &translationBias) {
            VASSERT(componentIndex < _translationBiases.size(), "componentIndex out of bounds of _translationBiases.");
            _translationBiases[componentIndex] = translationBias;
        }

        // ---- Rotation bias (Baumgarte, 2-DOF) ----

        /** @brief Returns a mutable reference to the Baumgarte rotation bias vector (2-DOF).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetRotationBias(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _rotationBiases[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the Baumgarte rotation bias vector at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec2 &GetRotationBiasAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _rotationBiases.size(), "componentIndex out of bounds of _rotationBiases.");
            return _rotationBiases[componentIndex];
        }

        /** @brief Sets the Baumgarte rotation bias vector.
         * @param jointEntity Must have a component.
         * @param rotationBias The new rotation bias. */
        VE_INLINE void SetRotationBias(Entity jointEntity, const glm::vec2 &rotationBias) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _rotationBiases[_entityToComponentIndex.find(jointEntity)->second] = rotationBias;
        }

        /** @brief Sets the Baumgarte rotation bias vector at the given index.
         * @param componentIndex Must be in bounds.
         * @param rotationBias The new rotation bias. */
        VE_INLINE void SetRotationBiasAtIndex(size_t componentIndex, const glm::vec2 &rotationBias) {
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

        // ---- Hinge axis in body one's local space ----

        /** @brief Returns a mutable reference to the hinge rotation axis in body one's local space.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetHingeAxisInBodyOneLocalSpace(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _hingeAxisInBodyOneLocalSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the hinge axis in body one's local space at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetHingeAxisInBodyOneLocalSpaceAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _hingeAxisInBodyOneLocalSpace.size(), "componentIndex out of bounds of _hingeAxisInBodyOneLocalSpace.");
            return _hingeAxisInBodyOneLocalSpace[componentIndex];
        }

        /** @brief Sets the hinge rotation axis in body one's local space.
         * @param jointEntity Must have a component.
         * @param hingeAxisBody1 The new local-space hinge axis. */
        VE_INLINE void SetHingeAxisInBodyOneLocalSpace(Entity jointEntity, const glm::vec3 &hingeAxisBody1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _hingeAxisInBodyOneLocalSpace[_entityToComponentIndex.find(jointEntity)->second] = hingeAxisBody1;
        }

        /** @brief Sets the hinge axis in body one's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @param hingeAxisBody1 The new local-space hinge axis. */
        VE_INLINE void SetHingeAxisInBodyOneLocalSpaceAtIndex(size_t componentIndex, const glm::vec3 &hingeAxisBody1) {
            VASSERT(componentIndex < _hingeAxisInBodyOneLocalSpace.size(), "componentIndex out of bounds of _hingeAxisInBodyOneLocalSpace.");
            _hingeAxisInBodyOneLocalSpace[componentIndex] = hingeAxisBody1;
        }

        // ---- Hinge axis in body two's local space ----

        /** @brief Returns a mutable reference to the hinge rotation axis in body two's local space.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetHingeAxisInBodyTwoLocalSpace(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _hingeAxisInBodyTwoLocalSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the hinge axis in body two's local space at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetHingeAxisInBodyTwoLocalSpaceAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _hingeAxisInBodyTwoLocalSpace.size(), "componentIndex out of bounds of _hingeAxisInBodyTwoLocalSpace.");
            return _hingeAxisInBodyTwoLocalSpace[componentIndex];
        }

        /** @brief Sets the hinge rotation axis in body two's local space.
         * @param jointEntity Must have a component.
         * @param hingeAxisBody2 The new local-space hinge axis. */
        VE_INLINE void SetHingeAxisInBodyTwoLocalSpace(Entity jointEntity, const glm::vec3 &hingeAxisBody2) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _hingeAxisInBodyTwoLocalSpace[_entityToComponentIndex.find(jointEntity)->second] = hingeAxisBody2;
        }

        /** @brief Sets the hinge axis in body two's local space at the given index.
         * @param componentIndex Must be in bounds.
         * @param hingeAxisBody2 The new local-space hinge axis. */
        VE_INLINE void SetHingeAxisInBodyTwoLocalSpaceAtIndex(size_t componentIndex, const glm::vec3 &hingeAxisBody2) {
            VASSERT(componentIndex < _hingeAxisInBodyTwoLocalSpace.size(), "componentIndex out of bounds of _hingeAxisInBodyTwoLocalSpace.");
            _hingeAxisInBodyTwoLocalSpace[componentIndex] = hingeAxisBody2;
        }

        // ---- Hinge axis in world space (a1, derived from body one) ----

        /** @brief Returns a mutable reference to the hinge rotation axis in world space (computed from body one's current orientation).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetHingeAxisWorldSpace(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _hingeAxisWorldSpace[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to the world-space hinge axis at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetHingeAxisWorldSpaceAtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _hingeAxisWorldSpace.size(), "componentIndex out of bounds of _hingeAxisWorldSpace.");
            return _hingeAxisWorldSpace[componentIndex];
        }

        /** @brief Sets the hinge rotation axis in world space.
         * @param jointEntity Must have a component.
         * @param hingeAxisWorld The new world-space hinge axis (a1). */
        VE_INLINE void SetHingeAxisWorldSpace(Entity jointEntity, const glm::vec3 &hingeAxisWorld) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _hingeAxisWorldSpace[_entityToComponentIndex.find(jointEntity)->second] = hingeAxisWorld;
        }

        /** @brief Sets the world-space hinge axis at the given index.
         * @param componentIndex Must be in bounds.
         * @param hingeAxisWorld The new world-space hinge axis (a1). */
        VE_INLINE void SetHingeAxisWorldSpaceAtIndex(size_t componentIndex, const glm::vec3 &hingeAxisWorld) {
            VASSERT(componentIndex < _hingeAxisWorldSpace.size(), "componentIndex out of bounds of _hingeAxisWorldSpace.");
            _hingeAxisWorldSpace[componentIndex] = hingeAxisWorld;
        }

        // ---- b2 × a1 ----

        /** @brief Returns a mutable reference to the precomputed cross product of b2 and a1 (the hinge axis).
         * b2 is the first vector orthogonal to the hinge axis in body two's local frame.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetB2CrossA1(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _b2CrossA1Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to b2 × a1 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetB2CrossA1AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _b2CrossA1Vectors.size(), "componentIndex out of bounds of _b2CrossA1Vectors.");
            return _b2CrossA1Vectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of b2 and a1.
         * @param jointEntity Must have a component.
         * @param b2CrossA1 The new b2 × a1 vector. */
        VE_INLINE void SetB2CrossA1(Entity jointEntity, const glm::vec3 &b2CrossA1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _b2CrossA1Vectors[_entityToComponentIndex.find(jointEntity)->second] = b2CrossA1;
        }

        /** @brief Sets b2 × a1 at the given index.
         * @param componentIndex Must be in bounds.
         * @param b2CrossA1 The new b2 × a1 vector. */
        VE_INLINE void SetB2CrossA1AtIndex(size_t componentIndex, const glm::vec3 &b2CrossA1) {
            VASSERT(componentIndex < _b2CrossA1Vectors.size(), "componentIndex out of bounds of _b2CrossA1Vectors.");
            _b2CrossA1Vectors[componentIndex] = b2CrossA1;
        }

        // ---- c2 × a1 ----

        /** @brief Returns a mutable reference to the precomputed cross product of c2 and a1 (the hinge axis).
         * c2 is the second vector orthogonal to the hinge axis in body two's local frame.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetC2CrossA1(Entity jointEntity) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _c2CrossA1Vectors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns a mutable reference to c2 × a1 at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE glm::vec3 &GetC2CrossA1AtIndex(size_t componentIndex) {
            VASSERT(componentIndex < _c2CrossA1Vectors.size(), "componentIndex out of bounds of _c2CrossA1Vectors.");
            return _c2CrossA1Vectors[componentIndex];
        }

        /** @brief Sets the precomputed cross product of c2 and a1.
         * @param jointEntity Must have a component.
         * @param c2CrossA1 The new c2 × a1 vector. */
        VE_INLINE void SetC2CrossA1(Entity jointEntity, const glm::vec3 &c2CrossA1) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _c2CrossA1Vectors[_entityToComponentIndex.find(jointEntity)->second] = c2CrossA1;
        }

        /** @brief Sets c2 × a1 at the given index.
         * @param componentIndex Must be in bounds.
         * @param c2CrossA1 The new c2 × a1 vector. */
        VE_INLINE void SetC2CrossA1AtIndex(size_t componentIndex, const glm::vec3 &c2CrossA1) {
            VASSERT(componentIndex < _c2CrossA1Vectors.size(), "componentIndex out of bounds of _c2CrossA1Vectors.");
            _c2CrossA1Vectors[componentIndex] = c2CrossA1;
        }

        // ---- Lower limit impulse ----

        /** @brief Returns the accumulated impulse for the lower angular limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetImpulseLowerLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseLowerLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the accumulated lower limit impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetImpulseLowerLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _impulseLowerLimits.size(), "componentIndex out of bounds of _impulseLowerLimits.");
            return _impulseLowerLimits[componentIndex];
        }

        /** @brief Sets the accumulated impulse for the lower angular limit constraint.
         * @param jointEntity Must have a component.
         * @param impulseLowerLimit The new lower limit impulse. */
        VE_INLINE void SetImpulseLowerLimit(Entity jointEntity, f32 impulseLowerLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseLowerLimits[_entityToComponentIndex.find(jointEntity)->second] = impulseLowerLimit;
        }

        /** @brief Sets the accumulated lower limit impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseLowerLimit The new lower limit impulse. */
        VE_INLINE void SetImpulseLowerLimitAtIndex(size_t componentIndex, f32 impulseLowerLimit) {
            VASSERT(componentIndex < _impulseLowerLimits.size(), "componentIndex out of bounds of _impulseLowerLimits.");
            _impulseLowerLimits[componentIndex] = impulseLowerLimit;
        }

        // ---- Upper limit impulse ----

        /** @brief Returns the accumulated impulse for the upper angular limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetImpulseUpperLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseUpperLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the accumulated upper limit impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetImpulseUpperLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _impulseUpperLimits.size(), "componentIndex out of bounds of _impulseUpperLimits.");
            return _impulseUpperLimits[componentIndex];
        }

        /** @brief Sets the accumulated impulse for the upper angular limit constraint.
         * @param jointEntity Must have a component.
         * @param impulseUpperLimit The new upper limit impulse. */
        VE_INLINE void SetImpulseUpperLimit(Entity jointEntity, f32 impulseUpperLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseUpperLimits[_entityToComponentIndex.find(jointEntity)->second] = impulseUpperLimit;
        }

        /** @brief Sets the accumulated upper limit impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseUpperLimit The new upper limit impulse. */
        VE_INLINE void SetImpulseUpperLimitAtIndex(size_t componentIndex, f32 impulseUpperLimit) {
            VASSERT(componentIndex < _impulseUpperLimits.size(), "componentIndex out of bounds of _impulseUpperLimits.");
            _impulseUpperLimits[componentIndex] = impulseUpperLimit;
        }

        // ---- Motor impulse ----

        /** @brief Returns the accumulated impulse for the rotational motor constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetImpulseMotor(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _impulseMotors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the accumulated motor impulse at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetImpulseMotorAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _impulseMotors.size(), "componentIndex out of bounds of _impulseMotors.");
            return _impulseMotors[componentIndex];
        }

        /** @brief Sets the accumulated impulse for the rotational motor constraint.
         * @param jointEntity Must have a component.
         * @param impulseMotor The new motor impulse. */
        VE_INLINE void SetImpulseMotor(Entity jointEntity, f32 impulseMotor) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _impulseMotors[_entityToComponentIndex.find(jointEntity)->second] = impulseMotor;
        }

        /** @brief Sets the accumulated motor impulse at the given index.
         * @param componentIndex Must be in bounds.
         * @param impulseMotor The new motor impulse. */
        VE_INLINE void SetImpulseMotorAtIndex(size_t componentIndex, f32 impulseMotor) {
            VASSERT(componentIndex < _impulseMotors.size(), "componentIndex out of bounds of _impulseMotors.");
            _impulseMotors[componentIndex] = impulseMotor;
        }

        // ---- Inverse mass matrix for limits and motor (shared scalar) ----

        /** @brief Returns the inverse mass matrix K=JM^-1J^t shared by the angular limit and motor constraints.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixLimitMotor(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassMatrixLimitMotors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the limit/motor inverse mass at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixLimitMotorAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _inverseMassMatrixLimitMotors.size(), "componentIndex out of bounds of _inverseMassMatrixLimitMotors.");
            return _inverseMassMatrixLimitMotors[componentIndex];
        }

        /** @brief Sets the inverse mass matrix K shared by the angular limit and motor constraints.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrixLimitMotor The new limit/motor inverse mass. */
        VE_INLINE void SetInverseMassMatrixLimitMotor(Entity jointEntity, f32 inverseMassMatrixLimitMotor) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassMatrixLimitMotors[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrixLimitMotor;
        }

        /** @brief Sets the limit/motor inverse mass at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrixLimitMotor The new limit/motor inverse mass. */
        VE_INLINE void SetInverseMassMatrixLimitMotorAtIndex(size_t componentIndex, f32 inverseMassMatrixLimitMotor) {
            VASSERT(componentIndex < _inverseMassMatrixLimitMotors.size(), "componentIndex out of bounds of _inverseMassMatrixLimitMotors.");
            _inverseMassMatrixLimitMotors[componentIndex] = inverseMassMatrixLimitMotor;
        }

        // ---- Inverse mass matrix for motor (separate scalar) ----

        /** @brief Returns the inverse mass matrix K=JM^-1J^t for the rotational motor constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixMotor(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _inverseMassMatrixMotors[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the motor inverse mass at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetInverseMassMatrixMotorAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _inverseMassMatrixMotors.size(), "componentIndex out of bounds of _inverseMassMatrixMotors.");
            return _inverseMassMatrixMotors[componentIndex];
        }

        /** @brief Sets the inverse mass matrix K for the rotational motor constraint.
         * @param jointEntity Must have a component.
         * @param inverseMassMatrixMotor The new motor inverse mass. */
        VE_INLINE void SetInverseMassMatrixMotor(Entity jointEntity, f32 inverseMassMatrixMotor) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _inverseMassMatrixMotors[_entityToComponentIndex.find(jointEntity)->second] = inverseMassMatrixMotor;
        }

        /** @brief Sets the motor inverse mass at the given index.
         * @param componentIndex Must be in bounds.
         * @param inverseMassMatrixMotor The new motor inverse mass. */
        VE_INLINE void SetInverseMassMatrixMotorAtIndex(size_t componentIndex, f32 inverseMassMatrixMotor) {
            VASSERT(componentIndex < _inverseMassMatrixMotors.size(), "componentIndex out of bounds of _inverseMassMatrixMotors.");
            _inverseMassMatrixMotors[componentIndex] = inverseMassMatrixMotor;
        }

        // ---- Lower limit bias ----

        /** @brief Returns the Baumgarte bias for the lower angular limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetBiasLowerLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _biasLowerLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the lower limit bias at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetBiasLowerLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _biasLowerLimits.size(), "componentIndex out of bounds of _biasLowerLimits.");
            return _biasLowerLimits[componentIndex];
        }

        /** @brief Sets the Baumgarte bias for the lower angular limit constraint.
         * @param jointEntity Must have a component.
         * @param biasLowerLimit The new lower limit bias. */
        VE_INLINE void SetBiasLowerLimit(Entity jointEntity, f32 biasLowerLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _biasLowerLimits[_entityToComponentIndex.find(jointEntity)->second] = biasLowerLimit;
        }

        /** @brief Sets the lower limit bias at the given index.
         * @param componentIndex Must be in bounds.
         * @param biasLowerLimit The new lower limit bias. */
        VE_INLINE void SetBiasLowerLimitAtIndex(size_t componentIndex, f32 biasLowerLimit) {
            VASSERT(componentIndex < _biasLowerLimits.size(), "componentIndex out of bounds of _biasLowerLimits.");
            _biasLowerLimits[componentIndex] = biasLowerLimit;
        }

        // ---- Upper limit bias ----

        /** @brief Returns the Baumgarte bias for the upper angular limit constraint.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetBiasUpperLimit(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _biasUpperLimits[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the upper limit bias at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetBiasUpperLimitAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _biasUpperLimits.size(), "componentIndex out of bounds of _biasUpperLimits.");
            return _biasUpperLimits[componentIndex];
        }

        /** @brief Sets the Baumgarte bias for the upper angular limit constraint.
         * @param jointEntity Must have a component.
         * @param biasUpperLimit The new upper limit bias. */
        VE_INLINE void SetBiasUpperLimit(Entity jointEntity, f32 biasUpperLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _biasUpperLimits[_entityToComponentIndex.find(jointEntity)->second] = biasUpperLimit;
        }

        /** @brief Sets the upper limit bias at the given index.
         * @param componentIndex Must be in bounds.
         * @param biasUpperLimit The new upper limit bias. */
        VE_INLINE void SetBiasUpperLimitAtIndex(size_t componentIndex, f32 biasUpperLimit) {
            VASSERT(componentIndex < _biasUpperLimits.size(), "componentIndex out of bounds of _biasUpperLimits.");
            _biasUpperLimits[componentIndex] = biasUpperLimit;
        }

        // ---- Limit enabled flag ----

        /** @brief Returns true if the angular limits are enabled.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE bool GetIsLimitEnabled(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_limitEnabledFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns true if the angular limits are enabled at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE bool GetIsLimitEnabledAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _limitEnabledFlags.size(), "componentIndex out of bounds of _limitEnabledFlags.");
            return static_cast<bool>(_limitEnabledFlags[componentIndex]);
        }

        /** @brief Sets whether the angular limits are enabled.
         * @param jointEntity Must have a component.
         * @param isLimitEnabled True to enable limits. */
        VE_INLINE void SetIsLimitEnabled(Entity jointEntity, bool isLimitEnabled) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _limitEnabledFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isLimitEnabled);
        }

        /** @brief Sets whether the angular limits are enabled at the given index.
         * @param componentIndex Must be in bounds.
         * @param isLimitEnabled True to enable limits. */
        VE_INLINE void SetIsLimitEnabledAtIndex(size_t componentIndex, bool isLimitEnabled) {
            VASSERT(componentIndex < _limitEnabledFlags.size(), "componentIndex out of bounds of _limitEnabledFlags.");
            _limitEnabledFlags[componentIndex] = static_cast<u8>(isLimitEnabled);
        }

        // ---- Motor enabled flag ----

        /** @brief Returns true if the rotational motor is enabled.
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE bool GetIsMotorEnabled(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return static_cast<bool>(_motorEnabledFlags[_entityToComponentIndex.find(jointEntity)->second]);
        }

        /** @brief Returns true if the rotational motor is enabled at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE bool GetIsMotorEnabledAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _motorEnabledFlags.size(), "componentIndex out of bounds of _motorEnabledFlags.");
            return static_cast<bool>(_motorEnabledFlags[componentIndex]);
        }

        /** @brief Sets whether the rotational motor is enabled.
         * @param jointEntity Must have a component.
         * @param isMotorEnabled True to enable the motor. */
        VE_INLINE void SetIsMotorEnabled(Entity jointEntity, bool isMotorEnabled) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _motorEnabledFlags[_entityToComponentIndex.find(jointEntity)->second] = static_cast<u8>(isMotorEnabled);
        }

        /** @brief Sets whether the rotational motor is enabled at the given index.
         * @param componentIndex Must be in bounds.
         * @param isMotorEnabled True to enable the motor. */
        VE_INLINE void SetIsMotorEnabledAtIndex(size_t componentIndex, bool isMotorEnabled) {
            VASSERT(componentIndex < _motorEnabledFlags.size(), "componentIndex out of bounds of _motorEnabledFlags.");
            _motorEnabledFlags[componentIndex] = static_cast<u8>(isMotorEnabled);
        }

        // ---- Lower limit ----

        /** @brief Returns the minimum allowed rotation angle in radians.
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

        /** @brief Sets the minimum allowed rotation angle in radians.
         * @param jointEntity Must have a component.
         * @param lowerLimit The new lower limit in radians. */
        VE_INLINE void SetLowerLimit(Entity jointEntity, f32 lowerLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _lowerLimits[_entityToComponentIndex.find(jointEntity)->second] = lowerLimit;
        }

        /** @brief Sets the lower limit at the given index.
         * @param componentIndex Must be in bounds.
         * @param lowerLimit The new lower limit in radians. */
        VE_INLINE void SetLowerLimitAtIndex(size_t componentIndex, f32 lowerLimit) {
            VASSERT(componentIndex < _lowerLimits.size(), "componentIndex out of bounds of _lowerLimits.");
            _lowerLimits[componentIndex] = lowerLimit;
        }

        // ---- Upper limit ----

        /** @brief Returns the maximum allowed rotation angle in radians.
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

        /** @brief Sets the maximum allowed rotation angle in radians.
         * @param jointEntity Must have a component.
         * @param upperLimit The new upper limit in radians. */
        VE_INLINE void SetUpperLimit(Entity jointEntity, f32 upperLimit) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _upperLimits[_entityToComponentIndex.find(jointEntity)->second] = upperLimit;
        }

        /** @brief Sets the upper limit at the given index.
         * @param componentIndex Must be in bounds.
         * @param upperLimit The new upper limit in radians. */
        VE_INLINE void SetUpperLimitAtIndex(size_t componentIndex, f32 upperLimit) {
            VASSERT(componentIndex < _upperLimits.size(), "componentIndex out of bounds of _upperLimits.");
            _upperLimits[componentIndex] = upperLimit;
        }

        // ---- Lower limit violated flag ----

        /** @brief Returns true if the lower angular limit is currently violated.
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

        /** @brief Sets whether the lower angular limit is violated.
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

        /** @brief Returns true if the upper angular limit is currently violated.
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

        /** @brief Sets whether the upper angular limit is violated.
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

        /** @brief Returns the target angular speed of the motor (rad/s).
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

        /** @brief Sets the target angular speed of the motor.
         * @param jointEntity Must have a component.
         * @param motorSpeed The new motor speed in rad/s. */
        VE_INLINE void SetMotorSpeed(Entity jointEntity, f32 motorSpeed) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _motorSpeeds[_entityToComponentIndex.find(jointEntity)->second] = motorSpeed;
        }

        /** @brief Sets the motor speed at the given index.
         * @param componentIndex Must be in bounds.
         * @param motorSpeed The new motor speed in rad/s. */
        VE_INLINE void SetMotorSpeedAtIndex(size_t componentIndex, f32 motorSpeed) {
            VASSERT(componentIndex < _motorSpeeds.size(), "componentIndex out of bounds of _motorSpeeds.");
            _motorSpeeds[componentIndex] = motorSpeed;
        }

        // ---- Max motor torque ----

        /** @brief Returns the maximum torque the motor may exert to reach the target speed (N·m).
         * @param jointEntity Must have a component. */
        [[nodiscard]] VE_INLINE f32 GetMaxMotorTorque(Entity jointEntity) const {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            return _maxMotorTorques[_entityToComponentIndex.find(jointEntity)->second];
        }

        /** @brief Returns the maximum motor torque at the given index.
         * @param componentIndex Must be in bounds. */
        [[nodiscard]] VE_INLINE f32 GetMaxMotorTorqueAtIndex(size_t componentIndex) const {
            VASSERT(componentIndex < _maxMotorTorques.size(), "componentIndex out of bounds of _maxMotorTorques.");
            return _maxMotorTorques[componentIndex];
        }

        /** @brief Sets the maximum torque the motor may exert.
         * @param jointEntity Must have a component.
         * @param maxMotorTorque The new maximum motor torque in N·m. */
        VE_INLINE void SetMaxMotorTorque(Entity jointEntity, f32 maxMotorTorque) {
            VASSERT(HasComponent(jointEntity), "No joints registered for entity.");
            _maxMotorTorques[_entityToComponentIndex.find(jointEntity)->second] = maxMotorTorque;
        }

        /** @brief Sets the maximum motor torque at the given index.
         * @param componentIndex Must be in bounds.
         * @param maxMotorTorque The new maximum motor torque in N·m. */
        VE_INLINE void SetMaxMotorTorqueAtIndex(size_t componentIndex, f32 maxMotorTorque) {
            VASSERT(componentIndex < _maxMotorTorques.size(), "componentIndex out of bounds of _maxMotorTorques.");
            _maxMotorTorques[componentIndex] = maxMotorTorque;
        }

    protected:
        /** @brief Swaps all parallel data arrays at the two given indices and updates the entity-to-index map accordingly.
         * @param indexA Index of the first component to swap.
         * @param indexB Index of the second component to swap. */
        void swapComponents(size_t indexA, size_t indexB) override;

        /** @brief Removes the last element from every parallel data array. Called after the target component has been swapped to the back. */
        void removeLastComponentAndEntity() override;

    private:
        /** @brief Non-owning pointers to the runtime HingeJoint objects, one per joint entity. */
        std::vector<HingeJoint *> _joints;

        /** @brief Anchor points for each joint expressed in body one's local space. Used each frame to recompute the world-space lever arm r1. */
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyOne;

        /** @brief Anchor points for each joint expressed in body two's local space. Used each frame to recompute the world-space lever arm r2. */
        std::vector<glm::vec3> _localSpaceAnchorPointsOnBodyTwo;

        /** @brief World-space vectors from body one's centre of mass to the joint anchor (r1), recomputed each simulation step. */
        std::vector<glm::vec3> _r1WorldSpace;

        /** @brief World-space vectors from body two's centre of mass to the joint anchor (r2), recomputed each simulation step. */
        std::vector<glm::vec3> _r2WorldSpace;

        /** @brief World-space inertia tensors of body one, cached each simulation step for use in the constraint solver. */
        std::vector<glm::mat3> _bodyOneInertiaTensorsInWorldSpace;

        /** @brief World-space inertia tensors of body two, cached each simulation step for use in the constraint solver. */
        std::vector<glm::mat3> _bodyTwoInertiaTensorsInWorldSpace;

        /** @brief Accumulated 3-DOF translational constraint impulses (locking all translation at the anchor), warm-started each step. */
        std::vector<glm::vec3> _impulseTranslations;

        /** @brief Accumulated 2-DOF rotational constraint impulses (locking the two DOF perpendicular to the hinge axis), warm-started each step. */
        std::vector<glm::vec2> _impulseRotations;

        /** @brief Effective inverse mass matrices K for the 3-DOF translational constraint, computed each simulation step. */
        std::vector<glm::mat3> _inverseMassTranslationMatrices;

        /** @brief Effective inverse mass matrices K for the 2-DOF rotational constraint, computed each simulation step. */
        std::vector<glm::mat2> _inverseMassRotationMatrices;

        /** @brief Baumgarte position-correction bias vectors for the 3-DOF translational constraint, computed each step. */
        std::vector<glm::vec3> _translationBiases;

        /** @brief Baumgarte position-correction bias vectors for the 2-DOF rotational constraint, computed each step. */
        std::vector<glm::vec2> _rotationBiases;

        /** @brief Inverses of the initial relative orientations, computed once at joint creation and used to enforce the rotational constraint. */
        std::vector<glm::quat> _initialOrientationDifferenceInverses;

        /** @brief Hinge rotation axis expressed in body one's local space. Rotated to world space each step to obtain _hingeAxisWorldSpace. */
        std::vector<glm::vec3> _hingeAxisInBodyOneLocalSpace;

        /** @brief Hinge rotation axis expressed in body two's local space. Used to compute the alignment error each step. */
        std::vector<glm::vec3> _hingeAxisInBodyTwoLocalSpace;

        /** @brief World-space hinge rotation axis derived from body one's current orientation (a1), recomputed each simulation step. */
        std::vector<glm::vec3> _hingeAxisWorldSpace;

        /** @brief Precomputed cross products of b2 (first orthogonal to hinge axis in body two) and a1 (world-space hinge axis), cached each step. */
        std::vector<glm::vec3> _b2CrossA1Vectors;

        /** @brief Precomputed cross products of c2 (second orthogonal to hinge axis in body two) and a1 (world-space hinge axis), cached each step. */
        std::vector<glm::vec3> _c2CrossA1Vectors;

        /** @brief Accumulated impulses for the lower angular limit constraint, warm-started each step. */
        std::vector<f32> _impulseLowerLimits;

        /** @brief Accumulated impulses for the upper angular limit constraint, warm-started each step. */
        std::vector<f32> _impulseUpperLimits;

        /** @brief Accumulated impulses for the rotational motor constraint, warm-started each step. */
        std::vector<f32> _impulseMotors;

        /** @brief Scalar effective inverse masses K=JM^-1J^t shared by the angular limit and motor constraints (hinge axis projection). */
        std::vector<f32> _inverseMassMatrixLimitMotors;

        /** @brief Scalar effective inverse masses K=JM^-1J^t for the rotational motor constraint. */
        std::vector<f32> _inverseMassMatrixMotors;

        /** @brief Baumgarte position-correction biases for the lower angular limit, computed each step when the limit is active. */
        std::vector<f32> _biasLowerLimits;

        /** @brief Baumgarte position-correction biases for the upper angular limit, computed each step when the limit is active. */
        std::vector<f32> _biasUpperLimits;

        /** @brief Flags indicating whether the angular limits are enabled (stored as u8 for cache efficiency). */
        std::vector<u8> _limitEnabledFlags;

        /** @brief Flags indicating whether the rotational motor is enabled (stored as u8 for cache efficiency). */
        std::vector<u8> _motorEnabledFlags;

        /** @brief Minimum allowed rotation angles in radians. */
        std::vector<f32> _lowerLimits;

        /** @brief Maximum allowed rotation angles in radians. */
        std::vector<f32> _upperLimits;

        /** @brief Flags set to true when the joint angle is below the lower limit; used to select the correct constraint sign. */
        std::vector<u8> _lowerLimitViolatedFlags;

        /** @brief Flags set to true when the joint angle exceeds the upper limit; used to select the correct constraint sign. */
        std::vector<u8> _upperLimitViolatedFlags;

        /** @brief Target angular speeds for the rotational motor (rad/s). */
        std::vector<f32> _motorSpeeds;

        /** @brief Maximum torques the rotational motor may exert to reach the target speed (N·m). */
        std::vector<f32> _maxMotorTorques;
    };

} // namespace Vulkyrie
