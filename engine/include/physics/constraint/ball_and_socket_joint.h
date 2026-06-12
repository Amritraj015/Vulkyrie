#pragma once

#include "physics/constraint/joint.h"

namespace Vulkyrie {

    class RigidBody;

    /** @brief Initialisation struct passed to the world when creating a ball-and-socket joint.
     *
     * Provides two construction paths: one using a world-space anchor point (the joint will convert it to
     * local space at creation time), and one using pre-computed local-space anchor points on each body for
     * cases where the body transforms are already known or the world-space anchor is not available. */
    struct BallAndSocketJointData : public JointData {
        /** @brief True when the local-space anchor constructor was used; false when the world-space one was used. */
        bool IsUsingLocalSpaceAnchors;

        /** @brief Shared anchor point expressed in world space. Only valid when IsUsingLocalSpaceAnchors is false. */
        glm::vec3 AnchorPointInWorldSpace;

        /** @brief Anchor point on body one expressed in body one's local space. Only valid when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyOneLocalSpace;

        /** @brief Anchor point on body two expressed in body two's local space. Only valid when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyTwoLocalSpace;

        /** @brief Constructs joint data with a single world-space anchor point.
         * The joint will compute the equivalent local-space anchor for each body using their current transforms at creation time.
         * @param rigidBodyOne Non-owning pointer to the first body.
         * @param rigidBodyTwo Non-owning pointer to the second body.
         * @param anchorPointWorldSpace The anchor point shared by both bodies expressed in world space. */
        BallAndSocketJointData(RigidBody *rigidBodyOne, RigidBody *rigidBodyTwo, const glm::vec3 &anchorPointWorldSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::BallAndSocket)
            , IsUsingLocalSpaceAnchors(false)
            , AnchorPointInWorldSpace(anchorPointWorldSpace) {
        }

        /** @brief Constructs joint data with pre-computed local-space anchor points on each body.
         * Use this constructor when the local-space anchors are already known, or when the bodies'
         * world transforms are not yet in their final positions.
         * @param rigidBodyOne Non-owning pointer to the first body.
         * @param rigidBodyTwo Non-owning pointer to the second body.
         * @param anchorPointInBodyOneLocalSpace The anchor point expressed in body one's local space.
         * @param anchorPointInBodyTwoLocalSpace The anchor point expressed in body two's local space. */
        BallAndSocketJointData(RigidBody *rigidBodyOne,
                               RigidBody *rigidBodyTwo,
                               const glm::vec3 &anchorPointInBodyOneLocalSpace,
                               const glm::vec3 &anchorPointInBodyTwoLocalSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::BallAndSocket)
            , IsUsingLocalSpaceAnchors(true)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace) {
        }
    };

    /** @brief A ball-and-socket joint that allows arbitrary rotation between two bodies around a shared anchor point.
     *
     * Removes three translational degrees of freedom while leaving all three rotational degrees of freedom
     * unconstrained, effectively acting as a spherical pivot. An optional cone limit can be activated to
     * clamp the relative rotation to within a cone of a given half-angle, which is useful for simulating
     * anatomical joints or chain links with bounded swing. Constraint solver state (impulses, bias vectors,
     * inverse mass matrices, etc.) is stored externally in BallAndSocketJointComponentStore and accessed
     * through the owning PhysicsWorld. */
    class BallAndSocketJoint final : public Joint {
    public:
        /** @brief Constructs a BallAndSocketJoint for the given entity and initialises the local-space anchor points.
         * If the joint data uses a world-space anchor, the constructor converts it to local space using each body's
         * current transform. If local-space anchors are provided, they are stored directly.
         * @param entity The entity that identifies this joint in the component stores.
         * @param world Reference to the owning PhysicsWorld.
         * @param jointData Initialisation data containing the body pointers and anchor point specification. */
        BallAndSocketJoint(Entity entity, PhysicsWorld &world, const BallAndSocketJointData &jointData);

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJoint);

        /** @brief Destructor for BallAndSocketJoint. */
        ~BallAndSocketJoint() override = default;

        /** @brief Enables or disables the cone angular limit for this joint.
         * When enabled, the relative rotation between the bodies is constrained to a cone defined by the
         * current half-angle. Resets accumulated limit impulses and wakes both bodies on change.
         * @param enable True to activate the cone limit, false to deactivate it. */
        void EnableOrDisableConeLimit(bool enable);

        /** @brief Returns whether the cone angular limit is currently active for this joint.
         * @returns True if the cone limit is enabled, false otherwise. */
        bool ConeLimitEnabled() const;

        /** @brief Sets the half-angle of the cone limit in radians.
         * Defines the maximum angular deviation from the cone axis. Has no effect if the value is unchanged.
         * Resets accumulated limit impulses and wakes both bodies when the value does change.
         * @param coneHalfAngle The new cone limit half-angle in radians, expected in [0, PI]. */
        void SetConeLimitHalfAngle(f32 coneHalfAngle);

        /** @brief Returns the configured half-angle of the cone limit in radians.
         * @returns The cone limit half-angle in radians. */
        f32 GetConeLimitHalfAngle() const;

        /** @brief Computes and returns the current cone half-angle between the two bodies in radians.
         * Measures the actual angle between the joint axes in world space regardless of whether the cone
         * limit is active. Useful for monitoring or debugging joint state at runtime.
         * @returns The current cone half-angle in radians in [0, PI]. */
        f32 GetConeHalfAngle() const;

        /** @brief Returns the reaction force (in Newtons) on body two required to satisfy the position constraint.
         * Computed as the accumulated translational impulse divided by the timestep duration.
         * @param timestep The duration of the current simulation step. Must be greater than VE_MACHINE_EPSILON.
         * @returns The world-space reaction force vector in Newtons. */
        glm::vec3 GetReactionForce(Timestep timestep) const override;

        /** @brief Returns the reaction torque (in Newton-metres) on body two required to satisfy the constraint.
         * A ball-and-socket joint applies no torque, so this always returns zero.
         * @param timestep The duration of the current simulation step. Must be greater than VE_MACHINE_EPSILON.
         * @returns A zero vector — this constraint type generates no reaction torque. */
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
        /** @brief Baumgarte stabilization coefficient used by the constraint solver for position correction. */
        static constexpr f32 BETA = f32(0.2);

        /** @brief Zeroes the accumulated cone limit impulse and wakes both bodies.
         * Called whenever the cone limit parameters change to ensure the solver starts from a clean state. */
        void resetLimits();
    };

} // namespace Vulkyrie
