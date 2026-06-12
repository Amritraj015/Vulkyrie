#include "physics/constraint/hinge_joint.h"

namespace Vulkyrie {

    HingeJoint::HingeJoint(Entity entity, PhysicsWorld &world, const HingeJointData &data)
        : Joint(entity, world) {
    }

    bool HingeJoint::LimitEnabled() const {
    }

    bool HingeJoint::MotorEnabled() const {
    }

    void HingeJoint::EnableOrDisableLimit(bool isLimitEnabled) {
    }

    void HingeJoint::EnableOrDisableMotor(bool isMotorEnabled) {
    }

    f32 HingeJoint::GetMinAngleLimit() const {
    }

    void HingeJoint::SetMinAngleLimit(f32 lowerLimit) {
    }

    f32 HingeJoint::GetMaxAngleLimit() const {
    }

    void HingeJoint::SetMaxAngleLimit(f32 upperLimit) {
    }

    f32 HingeJoint::GetMotorSpeed() const {
    }

    void HingeJoint::SetMotorSpeed(f32 motorSpeed) {
    }

    f32 HingeJoint::GetMaxMotorTorque() const {
    }

    void HingeJoint::SetMaxMotorTorque(f32 maxMotorTorque) {
    }

    f32 HingeJoint::GetMotorTorque(f32 timeStep) const {
    }

    f32 HingeJoint::GetAngle() const {
    }

    glm::vec3 HingeJoint::GetReactionForce(Timestep timestep) const {
    }

    glm::vec3 HingeJoint::GetReactionTorque(Timestep timestep) const {
    }

    void HingeJoint::resetLimits() {
    }

} // namespace Vulkyrie
