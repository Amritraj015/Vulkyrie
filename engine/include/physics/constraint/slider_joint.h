#pragma once

#include "core/entity.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    struct SliderJointData final : public JointData {
        glm::vec3 AnchorPointInWorldSpace;
        glm::vec3 AnchorPointInBodyOneLocalSpace;
        glm::vec3 AnchorPointInBodyTwoLocalSpace;
        glm::vec3 SliderAxisInWorldSpace;
        glm::vec3 SliderAxisInBodyOneLocalSpace;
        f32 MinTranslationLimit;
        f32 MaxTranslationLimit;
        f32 MotorSpeed;
        f32 MaxMotorForce;
        bool IsUsingLocalSpaceAnchors;
        bool LimitEnabled;
        bool MotorEnabled;

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

    class SliderJoint final : public Joint {
    public:
        SliderJoint(Entity entity, PhysicsWorld &world, const SliderJointData &data);

        VE_DELETE_MOVE_AND_COPY(SliderJoint);

        ~SliderJoint() override = default;

        bool LimitEnabled() const;
        bool MotorEnabled() const;
        void EnableOrDisableLimit(bool limitEnabled);
        void EnableOrDisableMotor(bool motorEnabled);
        f32 GetTranslation() const;
        f32 GetMinTranslationLimit() const;
        void SetMinTranslationLimit(f32 lowerLimit);
        f32 GetMaxTranslationLimit() const;
        void SetMaxTranslationLimit(f32 upperLimit);
        f32 GetMotorSpeed() const;
        void SetMotorSpeed(f32 motorSpeed);
        f32 GetMaxMotorForce() const;
        void SetMaxMotorForce(f32 maxMotorForce);
        f32 GetMotorForce(Timestep timeStep) const;

        glm::vec3 GetReactionForce(Timestep timestep) const override;
        glm::vec3 GetReactionTorque(Timestep timestep) const override;

    private:
        void resetLimits();
    };

} // namespace Vulkyrie
