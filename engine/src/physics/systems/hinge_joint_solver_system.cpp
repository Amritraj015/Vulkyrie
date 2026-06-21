#include "physics/systems/hinge_joint_solver_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    HingeJointSolverSystem::HingeJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _hingeJointStore(world.GetHingeJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void HingeJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
    }

    void HingeJointSolverSystem::WarmStart() {
    }

    void HingeJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
    }

    void HingeJointSolverSystem::SolvePositionConstraint() {
    }

    f32 HingeJointSolverSystem::ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation) {
        return f32(0);
    }

    f32 HingeJointSolverSystem::ComputeNormalizedAngle(f32 angle) const {
        return f32(0);
    }

    f32 HingeJointSolverSystem::ComputeCorrespondingAngleNearLimits(f32 inputAngle, f32 lowerLimitAngle, f32 upperLimitAngle) const {
        return f32(0);
    }

} // namespace Vulkyrie
