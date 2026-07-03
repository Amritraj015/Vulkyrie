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
        // TODO: Implement this.

        (void)biasFactor;
    }

    void SliderJointSolverSystem::WarmStart() {
        // TODO: Implement this.
    }

    void SliderJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
        // TODO: Implement this.

        (void)timestep;
    }

    void SliderJointSolverSystem::SolvePositionConstraint() {
        // TODO: Implement this.
    }

} // namespace Vulkyrie
