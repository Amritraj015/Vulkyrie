#pragma once

#include "vlkypch.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    /** @brief Initialisation data passed to PhysicsWorld::CreateJoint when creating a HingeJoint.
     *
     *  Anchor points and the rotation axis can be supplied either in world space
     *  (IsUsingLocalSpaceAnchors == false) or directly in each body's local space
     *  (IsUsingLocalSpaceAnchors == true).  The constructor chosen sets the flag accordingly. */
    struct HingeJointData final : public JointData {

        /** @brief Anchor point shared by both bodies, in world space.
         *  Only read when IsUsingLocalSpaceAnchors is false. */
        glm::vec3 AnchorPointInWorldSpace;

        /** @brief Anchor point on body one expressed in body one's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyOneLocalSpace;

        /** @brief Anchor point on body two expressed in body two's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyTwoLocalSpace;

        /** @brief Hinge rotation axis in world space.
         *  Only read when IsUsingLocalSpaceAnchors is false. Need not be normalized. */
        glm::vec3 RotationAxisInWorldSpace;

        /** @brief Hinge rotation axis in body one's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. Need not be normalized. */
        glm::vec3 RotationAxisInBodyOneLocalSpace;

        /** @brief Hinge rotation axis in body two's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. Need not be normalized. */
        glm::vec3 RotationAxisInBodyTwoLocalSpace;

        /** @brief Minimum allowed rotation angle about the hinge axis (radians). Must be in [-2π, 0]. */
        f32 MinAngleLimit;

        /** @brief Maximum allowed rotation angle about the hinge axis (radians). Must be in [0, 2π]. */
        f32 MaxAngleLimit;

        /** @brief Target angular speed of the rotational motor (rad/s). */
        f32 MotorSpeed;

        /** @brief Maximum torque the motor may exert to reach the target speed (N·m). Must be >= 0. */
        f32 MaxMotorTorque;

        /** @brief When true the joint is initialised from local-space anchors and rotation axis;
         *  when false they are in world space and converted to local space in the constructor. */
        bool IsUsingLocalSpaceAnchors;

        /** @brief Whether the rotation limits are enabled at creation time. */
        bool LimitEnabled;

        /** @brief Whether the rotational motor is enabled at creation time.
         *  Note: constructors that accept motor parameters leave this false by default;
         *  call EnableOrDisableMotor to activate the motor after creating the joint. */
        bool MotorEnabled;

        /** @brief World-space anchor, no limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param initAnchorPointWorldSpace Shared anchor point in world space.
         *  @param initRotationAxisWorld Hinge rotation axis in world space. */
        HingeJointData(RigidBody *rigidBodyOne, RigidBody *rigidBodyTwo, const glm::vec3 &initAnchorPointWorldSpace, const glm::vec3 &initRotationAxisWorld)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Hinge)
            , AnchorPointInWorldSpace(initAnchorPointWorldSpace)
            , RotationAxisInWorldSpace(initRotationAxisWorld)
            , MinAngleLimit(-1)
            , MaxAngleLimit(1)
            , MotorSpeed(0)
            , MaxMotorTorque(0)
            , IsUsingLocalSpaceAnchors(false)
            , LimitEnabled(false)
            , MotorEnabled(false) {
        }

        /** @brief World-space anchor with limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param initAnchorPointWorldSpace Shared anchor point in world space.
         *  @param initRotationAxisWorld Hinge rotation axis in world space.
         *  @param minAngleLimit Minimum angle (radians, must be in [-2π, 0]).
         *  @param maxAngleLimit Maximum angle (radians, must be in [0, 2π]). */
        HingeJointData(RigidBody *rigidBodyOne,
                       RigidBody *rigidBodyTwo,
                       const glm::vec3 &initAnchorPointWorldSpace,
                       const glm::vec3 &initRotationAxisWorld,
                       f32 minAngleLimit,
                       f32 maxAngleLimit)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Hinge)
            , AnchorPointInWorldSpace(initAnchorPointWorldSpace)
            , RotationAxisInWorldSpace(initRotationAxisWorld)
            , MinAngleLimit(minAngleLimit)
            , MaxAngleLimit(maxAngleLimit)
            , MotorSpeed(0)
            , MaxMotorTorque(0)
            , IsUsingLocalSpaceAnchors(false)
            , LimitEnabled(true)
            , MotorEnabled(false) {
        }

        /** @brief World-space anchor with limits and motor parameters (motor starts disabled).
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param initAnchorPointWorldSpace Shared anchor point in world space.
         *  @param initRotationAxisWorld Hinge rotation axis in world space.
         *  @param minAngleLimit Minimum angle (radians, must be in [-2π, 0]).
         *  @param maxAngleLimit Maximum angle (radians, must be in [0, 2π]).
         *  @param motorSpeed Target angular motor speed (rad/s).
         *  @param maxMotorTorque Maximum torque the motor may exert (N·m, must be >= 0). */
        HingeJointData(RigidBody *rigidBodyOne,
                       RigidBody *rigidBodyTwo,
                       const glm::vec3 &initAnchorPointWorldSpace,
                       const glm::vec3 &initRotationAxisWorld,
                       f32 minAngleLimit,
                       f32 maxAngleLimit,
                       f32 motorSpeed,
                       f32 maxMotorTorque)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Hinge)
            , AnchorPointInWorldSpace(initAnchorPointWorldSpace)
            , RotationAxisInWorldSpace(initRotationAxisWorld)
            , MinAngleLimit(minAngleLimit)
            , MaxAngleLimit(maxAngleLimit)
            , MotorSpeed(motorSpeed)
            , MaxMotorTorque(maxMotorTorque)
            , IsUsingLocalSpaceAnchors(false)
            , LimitEnabled(true)
            , MotorEnabled(false) {
        }

        /** @brief Local-space anchors and rotation axis, no limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInBodyOneLocalSpace Anchor point in body one's local space.
         *  @param anchorPointInBodyTwoLocalSpace Anchor point in body two's local space.
         *  @param rotationAxisInBodyOneLocalSpace Hinge axis in body one's local space.
         *  @param rotationAxisInBodyTwoLocalSpace Hinge axis in body two's local space. */
        HingeJointData(RigidBody *rigidBodyOne,
                       RigidBody *rigidBodyTwo,
                       const glm::vec3 &anchorPointInBodyOneLocalSpace,
                       const glm::vec3 &anchorPointInBodyTwoLocalSpace,
                       const glm::vec3 &rotationAxisInBodyOneLocalSpace,
                       const glm::vec3 &rotationAxisInBodyTwoLocalSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Hinge)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , RotationAxisInBodyOneLocalSpace(rotationAxisInBodyOneLocalSpace)
            , RotationAxisInBodyTwoLocalSpace(rotationAxisInBodyTwoLocalSpace)
            , MinAngleLimit(-1)
            , MaxAngleLimit(1)
            , MotorSpeed(0)
            , MaxMotorTorque(0)
            , IsUsingLocalSpaceAnchors(true)
            , LimitEnabled(false)
            , MotorEnabled(false) {
        }

        /** @brief Local-space anchors and rotation axis with limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInBodyOneLocalSpace Anchor point in body one's local space.
         *  @param anchorPointInBodyTwoLocalSpace Anchor point in body two's local space.
         *  @param rotationAxisInBodyOneLocalSpace Hinge axis in body one's local space.
         *  @param rotationAxisInBodyTwoLocalSpace Hinge axis in body two's local space.
         *  @param minAngleLimit Minimum angle (radians, must be in [-2π, 0]).
         *  @param maxAngleLimit Maximum angle (radians, must be in [0, 2π]). */
        HingeJointData(RigidBody *rigidBodyOne,
                       RigidBody *rigidBodyTwo,
                       const glm::vec3 &anchorPointInBodyOneLocalSpace,
                       const glm::vec3 &anchorPointInBodyTwoLocalSpace,
                       const glm::vec3 &rotationAxisInBodyOneLocalSpace,
                       const glm::vec3 &rotationAxisInBodyTwoLocalSpace,
                       f32 minAngleLimit,
                       f32 maxAngleLimit)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Hinge)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , RotationAxisInBodyOneLocalSpace(rotationAxisInBodyOneLocalSpace)
            , RotationAxisInBodyTwoLocalSpace(rotationAxisInBodyTwoLocalSpace)
            , MinAngleLimit(minAngleLimit)
            , MaxAngleLimit(maxAngleLimit)
            , MotorSpeed(0)
            , MaxMotorTorque(0)
            , IsUsingLocalSpaceAnchors(true)
            , LimitEnabled(true)
            , MotorEnabled(false) {
        }

        /** @brief Local-space anchors and rotation axis with limits and motor parameters (motor starts disabled).
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInBodyOneLocalSpace Anchor point in body one's local space.
         *  @param anchorPointInBodyTwoLocalSpace Anchor point in body two's local space.
         *  @param rotationAxisInBodyOneLocalSpace Hinge axis in body one's local space.
         *  @param rotationAxisInBodyTwoLocalSpace Hinge axis in body two's local space.
         *  @param minAngleLimit Minimum angle (radians, must be in [-2π, 0]).
         *  @param maxAngleLimit Maximum angle (radians, must be in [0, 2π]).
         *  @param motorSpeed Target angular motor speed (rad/s).
         *  @param maxMotorTorque Maximum torque the motor may exert (N·m, must be >= 0). */
        HingeJointData(RigidBody *rigidBodyOne,
                       RigidBody *rigidBodyTwo,
                       const glm::vec3 &anchorPointInBodyOneLocalSpace,
                       const glm::vec3 &anchorPointInBodyTwoLocalSpace,
                       const glm::vec3 &rotationAxisInBodyOneLocalSpace,
                       const glm::vec3 &rotationAxisInBodyTwoLocalSpace,
                       f32 minAngleLimit,
                       f32 maxAngleLimit,
                       f32 motorSpeed,
                       f32 maxMotorTorque)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Hinge)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , RotationAxisInBodyOneLocalSpace(rotationAxisInBodyOneLocalSpace)
            , RotationAxisInBodyTwoLocalSpace(rotationAxisInBodyTwoLocalSpace)
            , MinAngleLimit(minAngleLimit)
            , MaxAngleLimit(maxAngleLimit)
            , MotorSpeed(motorSpeed)
            , MaxMotorTorque(maxMotorTorque)
            , IsUsingLocalSpaceAnchors(true)
            , LimitEnabled(true)
            , MotorEnabled(false) {
        }
    };

    /** @brief A hinge joint (revolute joint) that constrains two rigid bodies to a single rotational
     *  degree of freedom about a shared axis.
     *
     *  Relative translation along all three axes and relative rotation about the two axes perpendicular
     *  to the hinge axis are locked, removing five degrees of freedom in total.  Optional angular limits
     *  and a rotational motor can be layered on top of the base constraint. */
    class HingeJoint final : public Joint {
    public:
        /** @brief Constructs the joint and stores its local-space configuration in the component store.
         *
         *  If jointData.IsUsingLocalSpaceAnchors is false, the world-space anchor point and rotation
         *  axis are transformed into each body's local space here.  The initial inverse orientation
         *  difference is computed as inverse(normalize(q2 * q1^-1)) and stored so the rotational
         *  constraint can compute the error relative to the rest pose each frame.
         *  @param entity    The ECS entity representing this joint.
         *  @param world     The physics world that owns this joint.
         *  @param jointData Initialisation parameters (bodies, anchor, axis, limits, motor). */
        HingeJoint(Entity entity, PhysicsWorld &world, const HingeJointData &jointData);

        VE_DELETE_MOVE_AND_COPY(HingeJoint);

        /** @brief Destructor. */
        ~HingeJoint() override = default;

        /** @brief Returns true if the angular limits are currently enabled.
         *  @returns True when limits are active. */
        bool LimitEnabled() const;

        /** @brief Enables or disables the angular limits.
         *
         *  When the state changes, accumulated limit impulses are reset to zero and both bodies
         *  are awakened so the solver starts fresh without stale warm-start values.
         *  @param isLimitEnabled True to enable limits, false to disable. */
        void EnableOrDisableLimit(bool isLimitEnabled);

        /** @brief Returns true if the rotational motor is currently enabled.
         *  @returns True when the motor is active. */
        bool MotorEnabled() const;

        /** @brief Enables or disables the rotational motor.
         *
         *  The accumulated motor impulse is always reset to zero and both bodies are awakened,
         *  regardless of whether the enabled state actually changed.
         *  @param isMotorEnabled True to enable the motor, false to disable. */
        void EnableOrDisableMotor(bool isMotorEnabled);

        /** @brief Returns the minimum allowed rotation angle about the hinge axis (radians).
         *  @returns The lower angle limit stored in the component. */
        f32 GetMinAngleLimit() const;

        /** @brief Sets the minimum allowed rotation angle about the hinge axis.
         *
         *  Resets accumulated limit impulses and awakens both bodies if the value changes.
         *  @param lowerLimit New lower angle limit (radians). Must be in [-2π, 0]. */
        void SetMinAngleLimit(f32 lowerLimit);

        /** @brief Returns the maximum allowed rotation angle about the hinge axis (radians).
         *  @returns The upper angle limit stored in the component. */
        f32 GetMaxAngleLimit() const;

        /** @brief Sets the maximum allowed rotation angle about the hinge axis.
         *
         *  Resets accumulated limit impulses and awakens both bodies if the value changes.
         *  @param upperLimit New upper angle limit (radians). Must be in [0, 2π]. */
        void SetMaxAngleLimit(f32 upperLimit);

        /** @brief Returns the target angular speed of the rotational motor (rad/s).
         *  @returns Motor speed in radians per second. */
        f32 GetMotorSpeed() const;

        /** @brief Sets the target angular speed of the rotational motor.
         *
         *  Awakens both bodies if the value changes.
         *  @param motorSpeed New target speed in radians per second. */
        void SetMotorSpeed(f32 motorSpeed);

        /** @brief Returns the maximum torque the motor may exert (N·m).
         *  @returns Maximum motor torque in Newton-metres. */
        f32 GetMaxMotorTorque() const;

        /** @brief Sets the maximum torque the motor may exert.
         *
         *  Awakens both bodies if the value changes.
         *  @param maxMotorTorque New maximum torque (N·m). Must be >= 0. */
        void SetMaxMotorTorque(f32 maxMotorTorque);

        /** @brief Returns the torque currently applied by the motor (N·m).
         *
         *  Computed as accumulated motor impulse / time step.
         *  @param timestep Duration of the last simulation step.
         *  @returns Current motor torque in Newton-metres. */
        f32 GetMotorTorque(Timestep timestep) const;

        /** @brief Returns the current rotation angle of body two relative to body one about the
         *  hinge axis (radians).
         *
         *  Delegates the angle computation to the constraint solver system, which computes it from
         *  the current body orientations relative to the stored initial orientation difference.
         *  @returns Current hinge angle in radians. */
        f32 GetAngle() const;

        /** @brief Returns the net reaction force exerted on body two to satisfy the translational
         *  constraint (N).
         *
         *  The hinge joint locks all three translational degrees of freedom; this force counteracts
         *  any external forces that would otherwise cause relative translation.
         *  @param timestep Duration of the last simulation step.
         *  @returns Reaction force in world space (Newtons). */
        glm::vec3 GetReactionForce(Timestep timestep) const override;

        /** @brief Returns the net reaction torque exerted on body two to satisfy the rotational
         *  constraint (N·m).
         *
         *  Sums the two perpendicular rotational constraint impulses (b2×a1 and c2×a1 components),
         *  the lower-limit impulse, the upper-limit impulse, and the motor impulse, all projected
         *  onto the hinge axis where applicable, then divides by the time step.
         *  @param timestep Duration of the last simulation step.
         *  @returns Reaction torque in world space (Newton-metres). */
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
        /** @brief Zeroes the accumulated lower- and upper-limit impulses and awakens both bodies.
         *
         *  Called whenever the limit state or limit values change so the solver begins the next
         *  step without stale warm-start values that would produce a spurious impulse. */
        void resetLimits();
    };

} // namespace Vulkyrie
