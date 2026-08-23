#include "physics/constraint/ball_and_socket_joint.h"
#include "physics/physics_world.h"
#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/body/rigid_body.h"
#include "core/constants.h"

namespace Vulkyrie {

    BallAndSocketJoint::BallAndSocketJoint(Entity entity, PhysicsWorld &world, const BallAndSocketJointData &jointData)
        : Joint(entity, world) {
        glm::vec3 anchorPointOnBodyOneLocalSpace;
        glm::vec3 anchorPointOnBodyTwoLocalSpace;

        if (jointData.IsUsingLocalSpaceAnchors) {
            // Local-space anchors were provided directly — store them as-is.
            anchorPointOnBodyOneLocalSpace = jointData.AnchorPointInBodyOneLocalSpace;
            anchorPointOnBodyTwoLocalSpace = jointData.AnchorPointInBodyTwoLocalSpace;
        } else {
            // Convert the single world-space anchor to each body's local space using the
            // inverse of their current world transform so the constraint point tracks correctly
            // as the bodies move.
            TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();
            const TransformComponent &bodyOneTransform = transformStore.GetTransform(jointData.BodyOne->GetEntity());
            const TransformComponent &bodyTwoTransform = transformStore.GetTransform(jointData.BodyTwo->GetEntity());

            anchorPointOnBodyOneLocalSpace = bodyOneTransform.Inverse() * jointData.AnchorPointInWorldSpace;
            anchorPointOnBodyTwoLocalSpace = bodyTwoTransform.Inverse() * jointData.AnchorPointInWorldSpace;
        }

        BallAndSocketJointComponentStore &ballAndSocketJointStore = _physicsWorld.GetBallAndSocketJointComponentStore();
        ballAndSocketJointStore.SetLocalSpaceAnchorPointOnBodyOne(entity, anchorPointOnBodyOneLocalSpace);
        ballAndSocketJointStore.SetLocalSpaceAnchorPointOnBodyTwo(entity, anchorPointOnBodyTwoLocalSpace);
    }

    void BallAndSocketJoint::EnableOrDisableConeLimit(bool enable) {
        _physicsWorld.GetBallAndSocketJointComponentStore().SetConeLimitEnabledFlag(_entity, enable);

        // Resetting the accumulated impulse prevents stale limit forces from carrying over
        // into the first step after the limit state changes.
        resetLimits();
    }

    bool BallAndSocketJoint::ConeLimitEnabled() const {
        return _physicsWorld.GetBallAndSocketJointComponentStore().ConeLimitEnabled(_entity);
    }

    void BallAndSocketJoint::SetConeLimitHalfAngle(f32 coneHalfAngle) {
        // Guard against redundant resets when the angle hasn't actually changed.
        if (_physicsWorld.GetBallAndSocketJointComponentStore().GetConeLimitHalfAngle(_entity) != coneHalfAngle) {
            _physicsWorld.GetBallAndSocketJointComponentStore().SetConeLimitHalfAngle(_entity, coneHalfAngle);

            resetLimits();
        }
    }

    f32 BallAndSocketJoint::GetConeLimitHalfAngle() const {
        return _physicsWorld.GetBallAndSocketJointComponentStore().GetConeLimitHalfAngle(_entity);
    }

    f32 BallAndSocketJoint::GetConeHalfAngle() const {
        JointComponentStore &jointStore = _physicsWorld.GetJointComponentStore();
        const Entity bodyOneEntity = jointStore.GetBodyOneEntity(_entity);
        const Entity bodyTwoEntity = jointStore.GetBodyTwoEntity(_entity);

        TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();
        const TransformComponent &bodyOneTransform = transformStore.GetTransform(bodyOneEntity);
        const TransformComponent &bodyTwoTransform = transformStore.GetTransform(bodyTwoEntity);

        RigidBodyComponentStore &rigidBodyStore = _physicsWorld.GetRigidBodyComponentStore();
        BallAndSocketJointComponentStore &ballAndSocketJointStore = _physicsWorld.GetBallAndSocketJointComponentStore();

        // Compute the lever arms from each body's centre of mass to the joint anchor in local space.
        // Subtracting the local CoM accounts for bodies whose centre of mass doesn't coincide with their origin.
        const glm::vec3 r1LocalSpace = ballAndSocketJointStore.GetLocalSpaceAnchorPointOnBodyOne(_entity) - rigidBodyStore.GetLocalCenterOfMass(bodyOneEntity);
        const glm::vec3 r2LocalSpace = ballAndSocketJointStore.GetLocalSpaceAnchorPointOnBodyTwo(_entity) - rigidBodyStore.GetLocalCenterOfMass(bodyTwoEntity);

        // Rotate the local lever arms into world space using only the orientation (no translation needed
        // for direction vectors), then normalize to obtain unit cone axes for angle computation.
        const glm::vec3 r1WorldSpace = glm::normalize(bodyOneTransform.Rotation * r1LocalSpace);
        const glm::vec3 r2WorldSpace = glm::normalize(bodyTwoTransform.Rotation * r2LocalSpace);

        // Negate r2 so both axes point away from the anchor, giving the half-angle between them.
        return BallAndSocketJointSolverSystem::ComputeCurrentConeHalfAngle(r1WorldSpace, -r2WorldSpace);
    }

    glm::vec3 BallAndSocketJoint::GetReactionForce(Timestep timestep) const {
        VASSERT(timestep.GetSeconds() > VE_K_MACHINE_EPSILON, "Timestep must be greater than VE_MACHINE_EPSILON.");

        // Convert the accumulated translational impulse (N·s) to a force (N) by dividing by dt.
        return _physicsWorld.GetBallAndSocketJointComponentStore().GetImpulse(_entity) / timestep.GetSeconds();
    }

    glm::vec3 BallAndSocketJoint::GetReactionTorque([[maybe_unused]] Timestep timestep) const {
        VASSERT(timestep.GetSeconds() > VE_K_MACHINE_EPSILON, "Timestep must be greater than VE_MACHINE_EPSILON.");

        // A ball-and-socket joint constrains only translation, so it produces no reaction torque.
        return glm::vec3(0.0f, 0.0f, 0.0f);
    }

    void BallAndSocketJoint::resetLimits() {
        // Zero the accumulated cone limit impulse so the solver doesn't warm-start with an
        // impulse that was computed under the old limit configuration.
        _physicsWorld.GetBallAndSocketJointComponentStore().SetConeLimitImpulse(_entity, f32(0.0));

        awakeBodies();
    }

} // namespace Vulkyrie
