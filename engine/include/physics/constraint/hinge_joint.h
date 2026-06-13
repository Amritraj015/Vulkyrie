#pragma once

#include "vlkypch.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    struct HingeJointData final : public JointData {
        glm::vec3 AnchorPointInWorldSpace;
        glm::vec3 AnchorPointInBodyOneLocalSpace;
        glm::vec3 AnchorPointInBodyTwoLocalSpace;
        glm::vec3 RotationAxisInWorldSpace;
        glm::vec3 RotationAxisInBodyOneLocalSpace;
        glm::vec3 RotationAxisInBodyTwoLocalSpace;
        f32 MinAngleLimit;
        f32 MaxAngleLimit;
        f32 MotorSpeed;
        f32 MaxMotorTorque;
        bool IsUsingLocalSpaceAnchors;
        bool LimitEnabled;
        bool MotorEnabled;

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

    class HingeJoint final : public Joint {
    public:
        HingeJoint(Entity entity, PhysicsWorld &world, const HingeJointData &jointData);

        VE_DELETE_MOVE_AND_COPY(HingeJoint);

        ~HingeJoint() override = default;

        bool LimitEnabled() const;
        bool MotorEnabled() const;
        void EnableOrDisableLimit(bool isLimitEnabled);
        void EnableOrDisableMotor(bool isMotorEnabled);
        f32 GetMinAngleLimit() const;
        void SetMinAngleLimit(f32 lowerLimit);
        f32 GetMaxAngleLimit() const;
        void SetMaxAngleLimit(f32 upperLimit);
        f32 GetMotorSpeed() const;
        void SetMotorSpeed(f32 motorSpeed);
        f32 GetMaxMotorTorque() const;
        void SetMaxMotorTorque(f32 maxMotorTorque);
        f32 GetMotorTorque(f32 timeStep) const;
        f32 GetAngle() const;

        glm::vec3 GetReactionForce(Timestep timestep) const override;
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
        void resetLimits();
    };

} // namespace Vulkyrie
