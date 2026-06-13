#include "physics/constraint/hinge_joint.h"
#include "physics/body/rigid_body.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    HingeJoint::HingeJoint(Entity entity, PhysicsWorld &world, const HingeJointData &jointData)
        : Joint(entity, world) {

        TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();
        HingeJointComponentStore &hingeJointStore = _physicsWorld.GetHingeJointComponentStore();

        glm::vec3 anchorPointInBodyOneLocalSpace;
        glm::vec3 anchorPointInBodyTwoLocalSpace;
        glm::vec3 hingeAxisInBodyOneLocalSpace;
        glm::vec3 hingeAxisInBodyTwoLocalSpace;

        const TransformComponent &bodyOneTransform = transformStore.GetTransform(jointData.BodyOne->GetEntity());
        const TransformComponent &bodyTwoTransform = transformStore.GetTransform(jointData.BodyTwo->GetEntity());

        if (jointData.IsUsingLocalSpaceAnchors) {
            // Caller supplied pre-computed local-space values; use them directly.
            anchorPointInBodyOneLocalSpace = jointData.AnchorPointInBodyOneLocalSpace;
            anchorPointInBodyTwoLocalSpace = jointData.AnchorPointInBodyTwoLocalSpace;

            hingeAxisInBodyOneLocalSpace = jointData.RotationAxisInBodyOneLocalSpace;
            hingeAxisInBodyTwoLocalSpace = jointData.RotationAxisInBodyTwoLocalSpace;
        } else {
            // Transform world-space anchor and axis into each body's local space.
            anchorPointInBodyOneLocalSpace = bodyOneTransform.Inverse() * jointData.AnchorPointInWorldSpace;
            anchorPointInBodyTwoLocalSpace = bodyTwoTransform.Inverse() * jointData.AnchorPointInWorldSpace;

            hingeAxisInBodyOneLocalSpace = glm::normalize(glm::inverse(bodyOneTransform.Rotation) * jointData.RotationAxisInWorldSpace);
            hingeAxisInBodyTwoLocalSpace = glm::normalize(glm::inverse(bodyTwoTransform.Rotation) * jointData.RotationAxisInWorldSpace);
        }

        // These values were set by AddComponent before this constructor ran.
        const f32 lowerLimit = hingeJointStore.GetLowerLimit(_entity);
        const f32 upperLimit = hingeJointStore.GetUpperLimit(_entity);

        VASSERT(lowerLimit <= f32(0) && lowerLimit >= f32(-2.0) * std::numbers::pi_v<f32>, "Invalid lower limit.");
        VASSERT(upperLimit >= f32(0) && upperLimit <= f32(2.0) * std::numbers::pi_v<f32>, "Invalid upper limit.");

        hingeJointStore.SetLocalSpaceAnchorPointOnBodyOne(_entity, anchorPointInBodyOneLocalSpace);
        hingeJointStore.SetLocalSpaceAnchorPointOnBodyTwo(_entity, anchorPointInBodyTwoLocalSpace);

        hingeJointStore.SetHingeAxisInBodyOneLocalSpace(_entity, hingeAxisInBodyOneLocalSpace);
        hingeJointStore.SetHingeAxisInBodyTwoLocalSpace(_entity, hingeAxisInBodyTwoLocalSpace);

        // Store inverse(normalize(q2 * q1^-1)): derived from q2_0 = q1_0 * r0  =>  r0 = q1_0^-1 * q2_0  =>  r0^-1 = q2_0^-1 * q1_0.
        // The solver uses this each frame to compute the rotational constraint error relative to the rest pose.
        glm::quat initialOrientationDifferenceInverse = glm::inverse(glm::normalize(bodyTwoTransform.Rotation * glm::inverse(bodyOneTransform.Rotation)));
        hingeJointStore.SetInitialOrientationDifferenceInverse(_entity, initialOrientationDifferenceInverse);
    }

    bool HingeJoint::LimitEnabled() const {
        return _physicsWorld.GetHingeJointComponentStore().IsLimitEnabled(_entity);
    }

    void HingeJoint::EnableOrDisableLimit(bool isLimitEnabled) {
        // Only update when the state changes to avoid discarding warm-start data unnecessarily.
        if (isLimitEnabled != _physicsWorld.GetHingeJointComponentStore().IsLimitEnabled(_entity)) {
            _physicsWorld.GetHingeJointComponentStore().SetLimitEnabledFlag(_entity, isLimitEnabled);

            // Clear accumulated limit impulses so the solver doesn't warm-start with stale values.
            resetLimits();
        }
    }

    bool HingeJoint::MotorEnabled() const {
        return _physicsWorld.GetHingeJointComponentStore().IsMotorEnabled(_entity);
    }

    void HingeJoint::EnableOrDisableMotor(bool isMotorEnabled) {
        // Always reset the accumulated motor impulse so the solver doesn't warm-start with a stale
        // value when the motor is toggled, regardless of whether the flag actually changed.
        _physicsWorld.GetHingeJointComponentStore().SetMotorEnabledFlag(_entity, isMotorEnabled);
        _physicsWorld.GetHingeJointComponentStore().SetImpulseMotor(_entity, f32(0.0));

        awakeBodies();
    }

    f32 HingeJoint::GetMinAngleLimit() const {
        return _physicsWorld.GetHingeJointComponentStore().GetLowerLimit(_entity);
    }

    void HingeJoint::SetMinAngleLimit(f32 lowerLimit) {
        const f32 limit = _physicsWorld.GetHingeJointComponentStore().GetLowerLimit(_entity);

        VASSERT(lowerLimit <= f32(0.0) && lowerLimit >= f32(-2.0) * std::numbers::pi_v<f32>, "Invalid min angle limit.");

        // Only reset if the value changed to avoid discarding warm-start data on a no-op call.
        if (lowerLimit != limit) {
            _physicsWorld.GetHingeJointComponentStore().SetLowerLimit(_entity, lowerLimit);

            resetLimits();
        }
    }

    f32 HingeJoint::GetMaxAngleLimit() const {
        return _physicsWorld.GetHingeJointComponentStore().GetUpperLimit(_entity);
    }

    void HingeJoint::SetMaxAngleLimit(f32 upperLimit) {
        const f32 limit = _physicsWorld.GetHingeJointComponentStore().GetUpperLimit(_entity);

        VASSERT(upperLimit >= f32(0) && upperLimit <= f32(2.0) * std::numbers::pi_v<f32>, "Invalid max angle limit.");

        // Only reset if the value changed to avoid discarding warm-start data on a no-op call.
        if (upperLimit != limit) {
            _physicsWorld.GetHingeJointComponentStore().SetUpperLimit(_entity, upperLimit);

            resetLimits();
        }
    }

    f32 HingeJoint::GetMotorSpeed() const {
        return _physicsWorld.GetHingeJointComponentStore().GetMotorSpeed(_entity);
    }

    void HingeJoint::SetMotorSpeed(f32 motorSpeed) {
        if (motorSpeed != _physicsWorld.GetHingeJointComponentStore().GetMotorSpeed(_entity)) {
            _physicsWorld.GetHingeJointComponentStore().SetMotorSpeed(_entity, motorSpeed);

            awakeBodies();
        }
    }

    f32 HingeJoint::GetMaxMotorTorque() const {
        return _physicsWorld.GetHingeJointComponentStore().GetMaxMotorTorque(_entity);
    }

    void HingeJoint::SetMaxMotorTorque(f32 maxMotorTorque) {
        const f32 torque = _physicsWorld.GetHingeJointComponentStore().GetMaxMotorTorque(_entity);

        if (maxMotorTorque != torque) {
            VASSERT(maxMotorTorque >= f32(0.0), "Torque must be >= 0.");

            _physicsWorld.GetHingeJointComponentStore().SetMaxMotorTorque(_entity, maxMotorTorque);

            awakeBodies();
        }
    }

    f32 HingeJoint::GetMotorTorque(Timestep timestep) const {
        return _physicsWorld.GetHingeJointComponentStore().GetImpulseMotor(_entity) / timestep.GetSeconds();
    }

    f32 HingeJoint::GetAngle() const {
        JointComponentStore &jointStore = _physicsWorld.GetJointComponentStore();
        TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();

        const Entity bodyOneEntity = jointStore.GetBodyOneEntity(_entity);
        const Entity bodyTwoEntity = jointStore.GetBodyTwoEntity(_entity);

        const glm::quat &bodyOneOrientation = transformStore.GetTransform(bodyOneEntity).Rotation;
        const glm::quat &bodyTwoOrientation = transformStore.GetTransform(bodyTwoEntity).Rotation;

        // Delegate to the constraint solver, which computes the angle relative to the stored
        // initial orientation difference to avoid gimbal-lock discontinuities.
        return _physicsWorld.GetConstraintSolverSystem().ComputeCurrentHingeAngle(_entity, bodyOneOrientation, bodyTwoOrientation);
    }

    glm::vec3 HingeJoint::GetReactionForce(Timestep timestep) const {
        VASSERT(VE_MACHINE_EPSILON <= timestep.GetSeconds(), "timestep must be greater than machine epsilon.");

        return _physicsWorld.GetHingeJointComponentStore().GetImpulseTranslation(_entity) / timestep.GetSeconds();
    }

    glm::vec3 HingeJoint::GetReactionTorque(Timestep timestep) const {
        VASSERT(VE_MACHINE_EPSILON <= timestep.GetSeconds(), "timestep must be greater than machine epsilon.");

        // Use index-based accessors to avoid a second map lookup for each field.
        HingeJointComponentStore &hingeJointStore = _physicsWorld.GetHingeJointComponentStore();
        const size_t jointIndex = hingeJointStore.GetEntityIndex(_entity);

        // 2-DOF rotational constraint impulse that resists rotation perpendicular to the hinge axis.
        const glm::vec2 &impulseRotation = hingeJointStore.GetImpulseRotationAtIndex(jointIndex);
        const glm::vec3 &b2CrossA1 = hingeJointStore.GetB2CrossA1AtIndex(jointIndex);
        const glm::vec3 &c2CrossA1 = hingeJointStore.GetC2CrossA1AtIndex(jointIndex);
        const glm::vec3 jointImpulse = b2CrossA1 * impulseRotation.x + c2CrossA1 * impulseRotation.y;

        // Limit and motor impulses act along the hinge axis. Upper limit is negated because its
        // constraint Jacobian row points in the opposite direction to the lower limit.
        const glm::vec3 hingeJointRotationAxis = hingeJointStore.GetHingeAxisWorldSpaceAtIndex(jointIndex);
        const glm::vec3 impulseLowerLimit = hingeJointStore.GetImpulseLowerLimitAtIndex(jointIndex) * hingeJointRotationAxis;
        const glm::vec3 impulseUpperLimit = -hingeJointStore.GetImpulseUpperLimitAtIndex(jointIndex) * hingeJointRotationAxis;
        const glm::vec3 motorImpulse = hingeJointStore.GetImpulseMotorAtIndex(jointIndex) * hingeJointRotationAxis;

        // Convert accumulated impulse to torque by dividing by the time step.
        return (jointImpulse + impulseLowerLimit + impulseUpperLimit + motorImpulse) / timestep.GetSeconds();
    }

    void HingeJoint::resetLimits() {
        HingeJointComponentStore &hingeJointStore = _physicsWorld.GetHingeJointComponentStore();

        // Clear accumulated limit impulses so the solver doesn't warm-start with stale values.
        hingeJointStore.SetImpulseLowerLimit(_entity, f32(0.0));
        hingeJointStore.SetImpulseUpperLimit(_entity, f32(0.0));

        awakeBodies();
    }

} // namespace Vulkyrie
