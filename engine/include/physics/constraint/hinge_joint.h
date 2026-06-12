#pragma once

#include "vlkypch.h"
#include "physics/constraint/joint.h"

namespace Vulkyrie {

    struct HingeJointData final : public JointData {};

    class HingeJoint final : public Joint {
    public:
        HingeJoint(Entity entity, PhysicsWorld &world, const HingeJointData &data);

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
