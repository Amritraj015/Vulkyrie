#include "physics/constraint/slider_joint.h"

namespace Vulkyrie {

    SliderJoint::SliderJoint(Entity entity, PhysicsWorld &world, const SliderJointData &data)
        : Joint(entity, world) {
    }

    bool SliderJoint::LimitEnabled() const {
    }

    bool SliderJoint::MotorEnabled() const {
    }

    void SliderJoint::EnableOrDisableLimit(bool limitEnabled) {
    }

    void SliderJoint::EnableOrDisableMotor(bool motorEnabled) {
    }

    f32 SliderJoint::GetTranslation() const {
    }

    f32 SliderJoint::GetMinTranslationLimit() const {
    }

    void SliderJoint::SetMinTranslationLimit(f32 lowerLimit) {
    }

    f32 SliderJoint::GetMaxTranslationLimit() const {
    }

    void SliderJoint::SetMaxTranslationLimit(f32 upperLimit) {
    }

    f32 SliderJoint::GetMotorSpeed() const {
    }

    void SliderJoint::SetMotorSpeed(f32 motorSpeed) {
    }

    f32 SliderJoint::GetMaxMotorForce() const {
    }

    void SliderJoint::SetMaxMotorForce(f32 maxMotorForce) {
    }

    f32 SliderJoint::GetMotorForce(f32 timeStep) const {
    }

    glm::vec3 SliderJoint::GetReactionForce(Timestep timestep) const {
    }

    glm::vec3 SliderJoint::GetReactionTorque(Timestep timestep) const {
    }

} // namespace Vulkyrie
