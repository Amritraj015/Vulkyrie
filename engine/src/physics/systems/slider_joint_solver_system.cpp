#include "physics/systems/slider_joint_solver_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    SliderJointSolverSystem::SliderJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _sliderJointStore(world.GetSliderJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void SliderJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
    }

    void SliderJointSolverSystem::WarmStart() {
    }

    void SliderJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
    }

    void SliderJointSolverSystem::SolvePositionConstraint() {
    }

} // namespace Vulkyrie
