#include "physics/constraint/fixed_joint.h"
#include "physics/body/rigid_body.h"
#include "core/asserts.h"
#include "core/constants.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    FixedJoint::FixedJoint(Entity entity, PhysicsWorld &world, const FixedJointData &data)
        : Joint(entity, world) {
        glm::vec3 anchorPointInBodyOneLocalSpace;
        glm::vec3 anchorPointInBodyTwoLocalSpace;

        // Transforms are always fetched regardless of the anchor path because they are needed below
        // to compute the initial orientation difference, even when local-space anchors are provided.
        TransformComponentStore &transformStore = _physicsWorld.GetTransformComponentStore();
        const TransformComponent &bodyOneTransform = transformStore.GetTransform(data.BodyOne->GetEntity());
        const TransformComponent &bodyTwoTransform = transformStore.GetTransform(data.BodyTwo->GetEntity());

        if (data.IsUsingLocalSpaceAnchors) {
            // Local-space anchors were provided directly — store them as-is.
            anchorPointInBodyOneLocalSpace = data.AnchorPointInBodyOneLocalSpace;
            anchorPointInBodyTwoLocalSpace = data.AnchorPointInBodyTwoLocalSpace;
        } else {
            // Convert the world-space anchor to each body's local space using the full inverse
            // transform (rotation + translation). Using only the rotation quaternion would produce
            // wrong results for bodies not centred at the world origin.
            anchorPointInBodyOneLocalSpace = bodyOneTransform.Inverse() * data.AnchorPointInWorldSpace;
            anchorPointInBodyTwoLocalSpace = bodyTwoTransform.Inverse() * data.AnchorPointInWorldSpace;
        }

        FixedJointComponentStore &fixedJointStore = _physicsWorld.GetFixedJointComponentStore();
        fixedJointStore.SetLocalSpaceAnchorPointOnBodyOne(_entity, anchorPointInBodyOneLocalSpace);
        fixedJointStore.SetLocalSpaceAnchorPointOnBodyTwo(_entity, anchorPointInBodyTwoLocalSpace);

        // Store the inverse of the initial relative orientation (r0^-1 = q2^-1 * q1).
        // The solver uses this each step to measure how far the current relative orientation
        // has drifted from the rest pose and compute a corrective rotational impulse.
        fixedJointStore.SetInitialOrientationDifferenceInverse(_entity, glm::inverse(bodyTwoTransform.Rotation) * bodyOneTransform.Rotation);
    }

    glm::vec3 FixedJoint::GetReactionForce(Timestep timestep) const {
        VASSERT(timestep.GetSeconds() > VE_MACHINE_EPSILON, "Timestep must be greater than VE_MACHINE_EPSILON.");

        // Convert the accumulated translational impulse (N·s) to a force (N) by dividing by dt.
        return _physicsWorld.GetFixedJointComponentStore().GetImpulseTranslation(_entity) / timestep.GetSeconds();
    }

    glm::vec3 FixedJoint::GetReactionTorque(Timestep timestep) const {
        VASSERT(timestep.GetSeconds() > VE_MACHINE_EPSILON, "Timestep must be greater than VE_MACHINE_EPSILON.");

        // Convert the accumulated rotational impulse (N·m·s) to a torque (N·m) by dividing by dt.
        return _physicsWorld.GetFixedJointComponentStore().GetImpulseRotation(_entity) / timestep.GetSeconds();
    }

} // namespace Vulkyrie
