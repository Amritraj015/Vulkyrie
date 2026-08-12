#pragma once

#include "core/ecs/entity.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    /** @brief Initialisation data passed to PhysicsWorld::CreateJoint when creating a SliderJoint.
     *
     *  Anchor points and the slider axis can be supplied either in world space
     *  (IsUsingLocalSpaceAnchors == false) or directly in each body's local space
     *  (IsUsingLocalSpaceAnchors == true).  The constructor chosen sets the flag accordingly. */
    struct SliderJointData final : public JointData {

        /** @brief Anchor point shared by both bodies, in world space.
         *  Only read when IsUsingLocalSpaceAnchors is false. */
        glm::vec3 AnchorPointInWorldSpace;

        /** @brief Anchor point on body one expressed in body one's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyOneLocalSpace;

        /** @brief Anchor point on body two expressed in body two's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. */
        glm::vec3 AnchorPointInBodyTwoLocalSpace;

        /** @brief Slider axis in world space.
         *  Only read when IsUsingLocalSpaceAnchors is false. Need not be normalized. */
        glm::vec3 SliderAxisInWorldSpace;

        /** @brief Slider axis in body one's local space.
         *  Only read when IsUsingLocalSpaceAnchors is true. Need not be normalized. */
        glm::vec3 SliderAxisInBodyOneLocalSpace;

        /** @brief Minimum allowed translation along the slider axis (metres). Must be <= 0. */
        f32 MinTranslationLimit;

        /** @brief Maximum allowed translation along the slider axis (metres). Must be >= 0. */
        f32 MaxTranslationLimit;

        /** @brief Target speed of the linear motor (m/s). */
        f32 MotorSpeed;

        /** @brief Maximum force the motor may exert to reach the target speed (N). Must be >= 0. */
        f32 MaxMotorForce;

        /** @brief When true the joint is initialised from local-space anchors and slider axis;
         *  when false they are in world space and converted to local space in the constructor. */
        bool IsUsingLocalSpaceAnchors;

        /** @brief Whether the translation limits are enabled at creation time. */
        bool LimitEnabled;

        /** @brief Whether the linear motor is enabled at creation time. */
        bool MotorEnabled;

        /** @brief World-space anchor, no limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInWorldSpace Shared anchor point in world space.
         *  @param sliderAxisInWorldSpace Slider axis in world space. */
        SliderJointData(RigidBody *rigidBodyOne, RigidBody *rigidBodyTwo, const glm::vec3 &anchorPointInWorldSpace, const glm::vec3 &sliderAxisInWorldSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Slider)
            , AnchorPointInWorldSpace(anchorPointInWorldSpace)
            , SliderAxisInWorldSpace(sliderAxisInWorldSpace)
            , MinTranslationLimit(-1.0)
            , MaxTranslationLimit(1.0)
            , MotorSpeed(0)
            , MaxMotorForce(0)
            , IsUsingLocalSpaceAnchors(false)
            , LimitEnabled(false)
            , MotorEnabled(false) {
        }

        /** @brief World-space anchor with limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInWorldSpace Shared anchor point in world space.
         *  @param sliderAxisInWorldSpace Slider axis in world space.
         *  @param minTranslationLimit Minimum translation (metres, must be <= 0).
         *  @param maxTranslationLimit Maximum translation (metres, must be >= 0). */
        SliderJointData(RigidBody *rigidBodyOne,
                        RigidBody *rigidBodyTwo,
                        const glm::vec3 &anchorPointInWorldSpace,
                        const glm::vec3 &sliderAxisInWorldSpace,
                        f32 minTranslationLimit,
                        f32 maxTranslationLimit)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Slider)
            , AnchorPointInWorldSpace(anchorPointInWorldSpace)
            , SliderAxisInWorldSpace(sliderAxisInWorldSpace)
            , MinTranslationLimit(minTranslationLimit)
            , MaxTranslationLimit(maxTranslationLimit)
            , MotorSpeed(0)
            , MaxMotorForce(0)
            , IsUsingLocalSpaceAnchors(false)
            , LimitEnabled(true)
            , MotorEnabled(false) {
        }

        /** @brief World-space anchor with limits and motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInWorldSpace Shared anchor point in world space.
         *  @param sliderAxisInWorldSpace Slider axis in world space.
         *  @param minTranslationLimit Minimum translation (metres, must be <= 0).
         *  @param maxTranslationLimit Maximum translation (metres, must be >= 0).
         *  @param motorSpeed Target linear motor speed (m/s).
         *  @param maxMotorForce Maximum force the motor may exert (N, must be >= 0). */
        SliderJointData(RigidBody *rigidBodyOne,
                        RigidBody *rigidBodyTwo,
                        const glm::vec3 &anchorPointInWorldSpace,
                        const glm::vec3 &sliderAxisInWorldSpace,
                        f32 minTranslationLimit,
                        f32 maxTranslationLimit,
                        f32 motorSpeed,
                        f32 maxMotorForce)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Slider)
            , AnchorPointInWorldSpace(anchorPointInWorldSpace)
            , SliderAxisInWorldSpace(sliderAxisInWorldSpace)
            , MinTranslationLimit(minTranslationLimit)
            , MaxTranslationLimit(maxTranslationLimit)
            , MotorSpeed(motorSpeed)
            , MaxMotorForce(maxMotorForce)
            , IsUsingLocalSpaceAnchors(false)
            , LimitEnabled(true)
            , MotorEnabled(true) {
        }

        /** @brief Local-space anchors and slider axis, no limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInBodyOneLocalSpace Anchor point in body one's local space.
         *  @param anchorPointInBodyTwoLocalSpace Anchor point in body two's local space.
         *  @param sliderAxisInBodyOneLocalSpace Slider axis in body one's local space. */
        SliderJointData(RigidBody *rigidBodyOne,
                        RigidBody *rigidBodyTwo,
                        const glm::vec3 &anchorPointInBodyOneLocalSpace,
                        const glm::vec3 &anchorPointInBodyTwoLocalSpace,
                        const glm::vec3 &sliderAxisInBodyOneLocalSpace)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Slider)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , SliderAxisInBodyOneLocalSpace(sliderAxisInBodyOneLocalSpace)
            , MinTranslationLimit(-1.0)
            , MaxTranslationLimit(1.0)
            , MotorSpeed(0)
            , MaxMotorForce(0)
            , IsUsingLocalSpaceAnchors(true)
            , LimitEnabled(false)
            , MotorEnabled(false) {
        }

        /** @brief Local-space anchors and slider axis with limits, no motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInBodyOneLocalSpace Anchor point in body one's local space.
         *  @param anchorPointInBodyTwoLocalSpace Anchor point in body two's local space.
         *  @param sliderAxisInBodyOneLocalSpace Slider axis in body one's local space.
         *  @param minTranslationLimit Minimum translation (metres, must be <= 0).
         *  @param maxTranslationLimit Maximum translation (metres, must be >= 0). */
        SliderJointData(RigidBody *rigidBodyOne,
                        RigidBody *rigidBodyTwo,
                        const glm::vec3 &anchorPointInBodyOneLocalSpace,
                        const glm::vec3 &anchorPointInBodyTwoLocalSpace,
                        const glm::vec3 &sliderAxisInBodyOneLocalSpace,
                        f32 minTranslationLimit,
                        f32 maxTranslationLimit)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Slider)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , SliderAxisInBodyOneLocalSpace(sliderAxisInBodyOneLocalSpace)
            , MinTranslationLimit(minTranslationLimit)
            , MaxTranslationLimit(maxTranslationLimit)
            , MotorSpeed(0)
            , MaxMotorForce(0)
            , IsUsingLocalSpaceAnchors(true)
            , LimitEnabled(true)
            , MotorEnabled(false) {
        }

        /** @brief Local-space anchors and slider axis with limits and motor.
         *  @param rigidBodyOne First body of the joint.
         *  @param rigidBodyTwo Second body of the joint.
         *  @param anchorPointInBodyOneLocalSpace Anchor point in body one's local space.
         *  @param anchorPointInBodyTwoLocalSpace Anchor point in body two's local space.
         *  @param sliderAxisInBodyOneLocalSpace Slider axis in body one's local space.
         *  @param minTranslationLimit Minimum translation (metres, must be <= 0).
         *  @param maxTranslationLimit Maximum translation (metres, must be >= 0).
         *  @param motorSpeed Target linear motor speed (m/s).
         *  @param maxMotorForce Maximum force the motor may exert (N, must be >= 0). */
        SliderJointData(RigidBody *rigidBodyOne,
                        RigidBody *rigidBodyTwo,
                        const glm::vec3 &anchorPointInBodyOneLocalSpace,
                        const glm::vec3 &anchorPointInBodyTwoLocalSpace,
                        const glm::vec3 &sliderAxisInBodyOneLocalSpace,
                        f32 minTranslationLimit,
                        f32 maxTranslationLimit,
                        f32 motorSpeed,
                        f32 maxMotorForce)
            : JointData(rigidBodyOne, rigidBodyTwo, JointType::Slider)
            , AnchorPointInBodyOneLocalSpace(anchorPointInBodyOneLocalSpace)
            , AnchorPointInBodyTwoLocalSpace(anchorPointInBodyTwoLocalSpace)
            , SliderAxisInBodyOneLocalSpace(sliderAxisInBodyOneLocalSpace)
            , MinTranslationLimit(minTranslationLimit)
            , MaxTranslationLimit(maxTranslationLimit)
            , MotorSpeed(motorSpeed)
            , MaxMotorForce(maxMotorForce)
            , IsUsingLocalSpaceAnchors(true)
            , LimitEnabled(true)
            , MotorEnabled(true) {
        }
    };

    /** @brief A slider joint (prismatic joint) that constrains two rigid bodies to a single
     *  translational degree of freedom along a shared axis.
     *
     *  Relative rotation about all three axes and relative translation perpendicular to the axis
     *  are locked, removing five degrees of freedom in total.  Optional translation limits and a
     *  linear motor can be layered on top of the base constraint. */
    class SliderJoint final : public Joint {
    public:
        /** @brief Constructs the joint and stores its local-space configuration in the component store.
         *
         *  If data.IsUsingLocalSpaceAnchors is false, the world-space anchor and axis are transformed
         *  into each body's local space here.  The initial inverse orientation difference (q2^-1 * q1)
         *  is computed once and stored so the rotational constraint can be enforced each frame.
         *  @param entity The ECS entity representing this joint.
         *  @param world  The physics world that owns this joint.
         *  @param jointData   Initialisation parameters (bodies, anchor, axis, limits, motor). */
        SliderJoint(Entity entity, PhysicsWorld &world, const SliderJointData &jointData);

        VE_DELETE_MOVE_AND_COPY(SliderJoint);

        /** @brief Destructor. */
        ~SliderJoint() override = default;

        /** @brief Returns true if the translation limits are currently enabled.
         *  @returns True when limits are active. */
        bool LimitEnabled() const;

        /** @brief Returns true if the linear motor is currently enabled.
         *  @returns True when the motor is active. */
        bool MotorEnabled() const;

        /** @brief Enables or disables the translation limits.
         *
         *  When the state changes, accumulated limit impulses are reset to zero and both bodies
         *  are awakened so the solver starts fresh without stale warm-start values.
         *  @param limitEnabled True to enable limits, false to disable. */
        void EnableOrDisableLimit(bool limitEnabled);

        /** @brief Enables or disables the linear motor.
         *
         *  The accumulated motor impulse is always reset to zero and both bodies are awakened,
         *  regardless of whether the enabled state actually changed.
         *  @param motorEnabled True to enable the motor, false to disable. */
        void EnableOrDisableMotor(bool motorEnabled);

        /** @brief Returns the current signed translation of body two relative to body one along the
         *  slider axis (metres).
         *
         *  Computed by projecting the vector between the two world-space anchor points onto the
         *  normalized world-space slider axis.  Positive values indicate translation in the axis
         *  direction.
         *  @returns Current translation in metres. */
        f32 GetTranslation() const;

        /** @brief Returns the minimum allowed translation along the slider axis (metres).
         *  @returns The lower limit stored in the component. */
        f32 GetMinTranslationLimit() const;

        /** @brief Sets the minimum allowed translation along the slider axis.
         *
         *  Resets accumulated limit impulses and awakens both bodies if the value changes.
         *  @param lowerLimit New lower limit (metres). Must be <= the current upper limit. */
        void SetMinTranslationLimit(f32 lowerLimit);

        /** @brief Returns the maximum allowed translation along the slider axis (metres).
         *  @returns The upper limit stored in the component. */
        f32 GetMaxTranslationLimit() const;

        /** @brief Sets the maximum allowed translation along the slider axis.
         *
         *  Resets accumulated limit impulses and awakens both bodies if the value changes.
         *  @param upperLimit New upper limit (metres). Must be >= the current lower limit. */
        void SetMaxTranslationLimit(f32 upperLimit);

        /** @brief Returns the target speed of the linear motor (m/s).
         *  @returns Motor speed in metres per second. */
        f32 GetMotorSpeed() const;

        /** @brief Sets the target speed of the linear motor.
         *
         *  Awakens both bodies if the value changes.
         *  @param motorSpeed New target speed in metres per second. */
        void SetMotorSpeed(f32 motorSpeed);

        /** @brief Returns the maximum force the motor may exert (N).
         *  @returns Maximum motor force in Newtons. */
        f32 GetMaxMotorForce() const;

        /** @brief Sets the maximum force the motor may exert.
         *
         *  Awakens both bodies if the value changes.
         *  @param maxMotorForce New maximum force (N). Must be >= 0. */
        void SetMaxMotorForce(f32 maxMotorForce);

        /** @brief Returns the force currently applied by the motor (N).
         *
         *  Computed as accumulated motor impulse / time step.
         *  @param timeStep Duration of the last simulation step (seconds).
         *  @returns Current motor force in Newtons. */
        f32 GetMotorForce(Timestep timeStep) const;

        /** @brief Returns the net reaction force exerted on body two to satisfy the joint constraint (N).
         *
         *  Sums the two perpendicular translational constraint impulses (along n1 and n2), the
         *  lower-limit impulse, the upper-limit impulse, and the motor impulse, then divides by the
         *  time step to convert from impulse to force.
         *  @param timestep Duration of the last simulation step.
         *  @returns Reaction force in world space (Newtons). */
        glm::vec3 GetReactionForce(Timestep timestep) const override;

        /** @brief Returns the net reaction torque exerted on body two to satisfy the rotational
         *  constraint (N·m).
         *
         *  The slider joint locks all three rotational degrees of freedom; this torque counteracts
         *  any external torques that would otherwise cause relative rotation.
         *  @param timestep Duration of the last simulation step.
         *  @returns Reaction torque in world space (Newton-metres). */
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
        /** @brief Zeroes the accumulated lower- and upper-limit impulses and awakens both bodies.
         *
         *  Called whenever the limit state changes so the solver begins the next step without
         *  stale warm-start values that would produce a spurious impulse. */
        void resetLimits();
    };

} // namespace Vulkyrie
