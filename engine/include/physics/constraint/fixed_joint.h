#pragma once

#include "vlkypch.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    /** @brief Initialisation struct passed to the world when creating a fixed joint.
     *
     * Provides two construction paths: one using a world-space anchor point (the joint will convert
     * it to local space at creation time), and one using pre-computed local-space anchor points on
     * each body for cases where the body transforms are already known or the world-space anchor is
     * not available. */
    struct FixedJointData final : public JointData {
        /** @brief Shared anchor point expressed in world space. Only valid when IsUsingLocalSpaceAnchors is false. */
        glm::vec3 AnchorPointInWorldSpace;

        /** @brief Anchor point on body one expressed in body one's local space. Only valid when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyOneLocalSpace;

        /** @brief Anchor point on body two expressed in body two's local space. Only valid when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyTwoLocalSpace;

        /** @brief True when the local-space anchor constructor was used; false when the world-space one was used. */
        bool IsUsingLocalSpaceAnchors;

        /** @brief Constructs joint data with a single world-space anchor point.
         * The joint will compute the equivalent local-space anchor for each body using their current transforms at creation time.
         * @param rigidBodyOne Non-owning pointer to the first body.
         * @param rigidBodyTwo Non-owning pointer to the second body.
         * @param anchorPointInWorldSpace The anchor point shared by both bodies expressed in world space. */
        FixedJointData(RigidBody *rigidBodyOne, RigidBody *rigidBodyTwo, const glm::vec3 &anchorPointInWorldSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Fixed)
            , AnchorPointInWorldSpace(anchorPointInWorldSpace)
            , IsUsingLocalSpaceAnchors(false) {
        }

        /** @brief Constructs joint data with pre-computed local-space anchor points on each body.
         * Use this constructor when the local-space anchors are already known, or when the bodies'
         * world transforms are not yet in their final positions.
         * @param rigidBodyOne Non-owning pointer to the first body.
         * @param rigidBodyTwo Non-owning pointer to the second body.
         * @param anchorPointInBodyOneLocalSpace The anchor point expressed in body one's local space.
         * @param anchorPointInBodyTwoLocalSpace The anchor point expressed in body two's local space. */
        FixedJointData(RigidBody *rigidBodyOne,
                       RigidBody *rigidBodyTwo,
                       const glm::vec3 &anchorPointInBodyOneLocalSpace,
                       const glm::vec3 &anchorPointInBodyTwoLocalSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Fixed)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , IsUsingLocalSpaceAnchors(true) {
        }
    };

    /** @brief A fixed joint that forbids all relative translation and rotation between two bodies.
     *
     * Removes all six degrees of freedom, locking the two bodies together as if they were a single
     * rigid object. The constraint is enforced via separate translational and rotational impulses each
     * simulation step. The initial relative orientation between the two bodies is recorded at creation
     * time and used throughout the joint's lifetime to drive the rotational constraint back to its
     * rest configuration. Constraint solver state is stored externally in FixedJointComponentStore
     * and accessed through the owning PhysicsWorld. */
    class FixedJoint final : public Joint {
    public:
        /** @brief Constructs a FixedJoint for the given entity, initialises the local-space anchor points,
         * and records the initial inverse orientation difference between the two bodies.
         * If the joint data uses a world-space anchor, the constructor converts it to local space using
         * each body's current transform. If local-space anchors are provided, they are stored directly.
         * @param entity The entity that identifies this joint in the component stores.
         * @param world Reference to the owning PhysicsWorld.
         * @param jointData Initialisation data containing the body pointers and anchor point specification. */
        FixedJoint(Entity entity, PhysicsWorld &world, const FixedJointData &jointData);

        VE_DELETE_MOVE_AND_COPY(FixedJoint);

        /** @brief Destructor for FixedJoint. */
        ~FixedJoint() override = default;

        /** @brief Returns the reaction force (in Newtons) on body two required to satisfy the translational constraint.
         * Computed as the accumulated translational impulse divided by the timestep duration.
         * @param timestep The duration of the current simulation step. Must be greater than VE_MACHINE_EPSILON.
         * @returns The world-space reaction force vector in Newtons. */
        glm::vec3 GetReactionForce(Timestep timestep) const override;

        /** @brief Returns the reaction torque (in Newton-metres) on body two required to satisfy the rotational constraint.
         * Computed as the accumulated rotational impulse divided by the timestep duration.
         * @param timestep The duration of the current simulation step. Must be greater than VE_MACHINE_EPSILON.
         * @returns The world-space reaction torque vector in Newton-metres. */
        glm::vec3 GetReactionTorque(Timestep timestep) const override;
    };

} // namespace Vulkyrie
