#include "physics/constraint/slider_joint.h"
#include "physics/body/rigid_body.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    SliderJoint::SliderJoint(Entity entity, PhysicsWorld &world, const SliderJointData &jointData)
        : Joint(entity, world) {

        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();
        TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();

        // These values were set by AddComponent before this constructor ran.
        VASSERT(sliderJointStore.GetUpperLimit(_entity) >= f32(0.0), "Upper limit must be >= 0.");
        VASSERT(sliderJointStore.GetLowerLimit(_entity) <= f32(0.0), "Lower limit must be <= 0.");
        VASSERT(sliderJointStore.GetMaxMotorForce(_entity) >= f32(0.0), "Max motor force must be >= 0.");

        glm::vec3 anchorPointInBodyOneLocalSpace;
        glm::vec3 anchorPointInBodyTwoLocalSpace;
        glm::vec3 sliderAxisInBodyOneLocalSpace;

        const TransformComponent &bodyOneTransform = transformStore.GetTransform(jointData.BodyOne->GetEntity());
        const TransformComponent &bodyTwoTransform = transformStore.GetTransform(jointData.BodyTwo->GetEntity());
        const TransformComponent bodyTwoInverseTransform = bodyTwoTransform.Inverse();

        if (jointData.IsUsingLocalSpaceAnchors) {
            // Caller supplied pre-computed local-space values; use them directly.
            anchorPointInBodyOneLocalSpace = jointData.AnchorPointInBodyOneLocalSpace;
            anchorPointInBodyTwoLocalSpace = jointData.AnchorPointInBodyTwoLocalSpace;
            sliderAxisInBodyOneLocalSpace = jointData.SliderAxisInBodyOneLocalSpace;
        } else {
            // Transform world-space anchor and axis into each body's local space.
            const TransformComponent bodyOneInverseTransform = bodyOneTransform.Inverse();
            anchorPointInBodyOneLocalSpace = bodyOneInverseTransform * jointData.AnchorPointInWorldSpace;
            anchorPointInBodyTwoLocalSpace = bodyTwoInverseTransform * jointData.AnchorPointInWorldSpace;
            sliderAxisInBodyOneLocalSpace = glm::normalize(bodyOneInverseTransform.Rotation * jointData.SliderAxisInWorldSpace);
        }

        sliderJointStore.SetLocalSpaceAnchorPointOnBodyOne(_entity, anchorPointInBodyOneLocalSpace);
        sliderJointStore.SetLocalSpaceAnchorPointOnBodyTwo(_entity, anchorPointInBodyTwoLocalSpace);
        sliderJointStore.SetSliderAxisInBodyOneLocalSpace(_entity, sliderAxisInBodyOneLocalSpace);

        // Store q2^-1 * q1: derived from q2_0 = q1_0 * r0  =>  r0^-1 = q2_0^-1 * q1_0.
        // The solver uses this each frame to compute the rotational constraint error.
        sliderJointStore.SetInitialOrientationDifferenceInverse(_entity, bodyTwoInverseTransform.Rotation * bodyOneTransform.Rotation);
    }

    void SliderJoint::EnableOrDisableLimit(bool limitEnabled) {
        bool enabled = _physicsWorld.GetSliderJointComponentStore().IsLimitEnabled(_entity);

        // Only update when the state changes to avoid discarding warm-start data unnecessarily.
        if (enabled != limitEnabled) {
            _physicsWorld.GetSliderJointComponentStore().SetLimitEnabledFlag(_entity, limitEnabled);

            // Clear accumulated limit impulses so the solver doesn't warm-start with stale values.
            resetLimits();
        }
    }

    bool SliderJoint::LimitEnabled() const {
        return _physicsWorld.GetSliderJointComponentStore().IsLimitEnabled(_entity);
    }

    void SliderJoint::EnableOrDisableMotor(bool motorEnabled) {
        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();

        // Always reset the accumulated motor impulse so the solver doesn't warm-start with a stale
        // value when the motor is toggled, regardless of whether the flag actually changed.
        sliderJointStore.SetMotorEnabledFlag(_entity, motorEnabled);
        sliderJointStore.SetImpulseMotor(_entity, f32(0.0));

        awakeBodies();
    }

    bool SliderJoint::MotorEnabled() const {
        return _physicsWorld.GetSliderJointComponentStore().IsMotorEnabled(_entity);
    }

    f32 SliderJoint::GetTranslation() const {
        JointComponentStore &jointStore = _physicsWorld.GetJointComponentStore();
        TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();
        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();

        const Entity bodyOneEntity = jointStore.GetBodyOneEntity(_entity);
        const Entity bodyTwoEntity = jointStore.GetBodyTwoEntity(_entity);

        const TransformComponent &bodyOneTransform = transformStore.GetTransform(bodyOneEntity);
        const TransformComponent &bodyTwoTransform = transformStore.GetTransform(bodyTwoEntity);

        const glm::vec3 &bodyOnePosition = bodyOneTransform.Position;
        const glm::vec3 &bodyTwoPosition = bodyTwoTransform.Position;
        const glm::quat &bodyOneRotation = bodyOneTransform.Rotation;
        const glm::quat &bodyTwoRotation = bodyTwoTransform.Rotation;

        // Transform each body's local-space anchor to world space.
        const glm::vec3 anchorPointOnBodyOne = bodyOnePosition + bodyOneRotation * sliderJointStore.GetLocalSpaceAnchorPointOnBodyOne(_entity);
        const glm::vec3 anchorPointOnBodyTwo = bodyTwoPosition + bodyTwoRotation * sliderJointStore.GetLocalSpaceAnchorPointOnBodyTwo(_entity);

        // u is the vector from body one's anchor to body two's anchor in world space.
        const glm::vec3 u = anchorPointOnBodyTwo - anchorPointOnBodyOne;

        // Rotate the local-space slider axis into world space using body one's current orientation.
        glm::vec3 sliderAxisInWorldSpace = glm::normalize(bodyOneRotation * sliderJointStore.GetSliderAxisInBodyOneLocalSpace(_entity));

        // Signed translation: positive when body two is ahead of body one along the axis.
        return glm::dot(u, sliderAxisInWorldSpace);
    }

    f32 SliderJoint::GetMinTranslationLimit() const {
        return _physicsWorld.GetSliderJointComponentStore().GetLowerLimit(_entity);
    }

    void SliderJoint::SetMinTranslationLimit(f32 lowerLimit) {
        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();

        VASSERT(sliderJointStore.GetUpperLimit(_entity) >= lowerLimit, "lower Limit cannot be less than upper limit");

        // Only reset if the value changed to avoid discarding warm-start data on a no-op call.
        if (sliderJointStore.GetLowerLimit(_entity) != lowerLimit) {
            sliderJointStore.SetLowerLimit(_entity, lowerLimit);

            resetLimits();
        }
    }

    f32 SliderJoint::GetMaxTranslationLimit() const {
        return _physicsWorld.GetSliderJointComponentStore().GetUpperLimit(_entity);
    }

    void SliderJoint::SetMaxTranslationLimit(f32 upperLimit) {
        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();

        VASSERT(sliderJointStore.GetLowerLimit(_entity) <= upperLimit, "lower Limit cannot be less than upper limit");

        // Only reset if the value changed to avoid discarding warm-start data on a no-op call.
        if (sliderJointStore.GetUpperLimit(_entity) != upperLimit) {
            sliderJointStore.SetUpperLimit(_entity, upperLimit);

            resetLimits();
        }
    }

    f32 SliderJoint::GetMotorSpeed() const {
        return _physicsWorld.GetSliderJointComponentStore().GetMotorSpeed(_entity);
    }

    void SliderJoint::SetMotorSpeed(f32 motorSpeed) {
        if (_physicsWorld.GetSliderJointComponentStore().GetMotorSpeed(_entity) != motorSpeed) {
            _physicsWorld.GetSliderJointComponentStore().SetMotorSpeed(_entity, motorSpeed);

            awakeBodies();
        }
    }

    f32 SliderJoint::GetMaxMotorForce() const {
        return _physicsWorld.GetSliderJointComponentStore().GetMaxMotorForce(_entity);
    }

    void SliderJoint::SetMaxMotorForce(f32 maxMotorForce) {
        const f32 maxForce = _physicsWorld.GetSliderJointComponentStore().GetMaxMotorForce(_entity);

        if (maxForce != maxMotorForce) {
            VASSERT(maxMotorForce >= f32(0.0), "Max force must be greater than 0.");

            _physicsWorld.GetSliderJointComponentStore().SetMaxMotorForce(_entity, maxMotorForce);

            awakeBodies();
        }
    }

    f32 SliderJoint::GetMotorForce(Timestep timeStep) const {
        return _physicsWorld.GetSliderJointComponentStore().GetImpulseMotor(_entity) / timeStep.GetSeconds();
    }

    glm::vec3 SliderJoint::GetReactionForce(Timestep timestep) const {
        VASSERT(VE_K_MACHINE_EPSILON <= timestep.GetSeconds(), "timestep must be greater than machine epsilon.");

        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();

        // Use index-based accessors to avoid a second map lookup for each field.
        const size_t jointIndex = sliderJointStore.GetEntityIndex(_entity);

        // 2-DOF translational constraint impulse along n1 and n2 (perpendicular to the slider axis).
        const glm::vec2 translationImpulse = sliderJointStore.GetImpulseTranslationAtIndex(jointIndex);
        const glm::vec3 &n1 = sliderJointStore.GetN1AtIndex(jointIndex);
        const glm::vec3 &n2 = sliderJointStore.GetN2AtIndex(jointIndex);
        const glm::vec3 jointImpulse = n1 * translationImpulse.x + n2 * translationImpulse.y;

        // Limit and motor impulses act along the slider axis. Upper limit and motor are negated
        // because their constraint constraint Jacobian rows point in the opposite direction to the lower limit.
        const glm::vec3 &sliderAxisInWorldSpace = sliderJointStore.GetSliderAxisInWorldSpaceAtIndex(jointIndex);
        const glm::vec3 impulseLowerLimit = sliderJointStore.GetImpulseLowerLimitAtIndex(jointIndex) * sliderAxisInWorldSpace;
        const glm::vec3 impulseUpperLimit = -sliderJointStore.GetImpulseUpperLimitAtIndex(jointIndex) * sliderAxisInWorldSpace;
        const glm::vec3 motorImpulse = -sliderJointStore.GetImpulseMotorAtIndex(jointIndex) * sliderAxisInWorldSpace;

        // Convert accumulated impulse to force by dividing by the time step.
        return (jointImpulse + impulseLowerLimit + impulseUpperLimit + motorImpulse) / timestep.GetSeconds();
    }

    glm::vec3 SliderJoint::GetReactionTorque(Timestep timestep) const {
        VASSERT(VE_K_MACHINE_EPSILON <= timestep.GetSeconds(), "timestep must be greater than machine epsilon.");

        return _physicsWorld.GetSliderJointComponentStore().GetImpulseRotation(_entity) / timestep.GetSeconds();
    }

    void SliderJoint::resetLimits() {
        SliderJointComponentStore &sliderJointStore = _physicsWorld.GetSliderJointComponentStore();
        sliderJointStore.SetImpulseLowerLimit(_entity, f32(0.0));
        sliderJointStore.SetImpulseUpperLimit(_entity, f32(0.0));

        awakeBodies();
    }

} // namespace Vulkyrie
