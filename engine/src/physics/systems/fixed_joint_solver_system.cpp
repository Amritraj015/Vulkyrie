#include "physics/systems/fixed_joint_solver_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    FixedJointSolverSystem::FixedJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _fixedJointStore(world.GetFixedJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void FixedJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
    }

    void FixedJointSolverSystem::WarmStart() {
    }

    void FixedJointSolverSystem::SolveVelocityConstraint() {
    }

    void FixedJointSolverSystem::SolvePositionConstraint() {
    }

} // namespace Vulkyrie
