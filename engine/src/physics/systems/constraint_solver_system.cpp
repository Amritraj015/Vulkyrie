#include "physics/systems/constraint_solver_system.h"

namespace Vulkyrie {

    ConstraintSolverSystem::ConstraintSolverSystem(PhysicsWorld &world,
                                                   RigidBodyComponentStore &rigidBodyStore,
                                                   TransformComponentStore &transformStore,
                                                   JointComponentStore &jointStore,
                                                   BallAndSocketJointComponentStore &ballAndSocketJointStore,
                                                   FixedJointComponentStore &fixedJointStore,
                                                   HingeJointComponentStore &hingeJointStore,
                                                   SliderJointComponentStore &sliderJointStore)
        : _ballAndSocketJointSolverSystem(world, rigidBodyStore, transformStore, jointStore, ballAndSocketJointStore)
        , _fixedJointSolverSystem(world, rigidBodyStore, transformStore, jointStore, fixedJointStore)
        , _hingeJointSolverSystem(world, rigidBodyStore, transformStore, jointStore, hingeJointStore)
        , _sliderJointSolverSystem(world, rigidBodyStore, transformStore, jointStore, sliderJointStore)
        , _enableWarmStartup(true) {
    }

    void ConstraintSolverSystem::Initialize(Timestep timestep) {
        _ballAndSocketJointSolverSystem.SetWarmStartFlag(_enableWarmStartup);
        _fixedJointSolverSystem.SetWarmStartFlag(_enableWarmStartup);
        _hingeJointSolverSystem.SetWarmStartFlag(_enableWarmStartup);
        _sliderJointSolverSystem.SetWarmStartFlag(_enableWarmStartup);

        const f32 biasFactor = BETA / timestep.GetSeconds();
        _ballAndSocketJointSolverSystem.InitializeBeforeSolve(biasFactor);
        _fixedJointSolverSystem.InitializeBeforeSolve(biasFactor);
        _hingeJointSolverSystem.InitializeBeforeSolve(biasFactor);
        _sliderJointSolverSystem.InitializeBeforeSolve(biasFactor);

        if (_enableWarmStartup) {
            _ballAndSocketJointSolverSystem.WarmStart();
            _fixedJointSolverSystem.WarmStart();
            _hingeJointSolverSystem.WarmStart();
            _sliderJointSolverSystem.WarmStart();
        }
    }

    void ConstraintSolverSystem::SolveVelocityConstraints(Timestep timestep) {
        _ballAndSocketJointSolverSystem.SolveVelocityConstraint();
        _fixedJointSolverSystem.SolveVelocityConstraint();
        _hingeJointSolverSystem.SolveVelocityConstraint(timestep);
        _sliderJointSolverSystem.SolveVelocityConstraint(timestep);
    }

    void ConstraintSolverSystem::SolvePositionConstraints() {
        _ballAndSocketJointSolverSystem.SolvePositionConstraint();
        _fixedJointSolverSystem.SolvePositionConstraint();
        _hingeJointSolverSystem.SolvePositionConstraint();
        _sliderJointSolverSystem.SolvePositionConstraint();
    }

} // namespace Vulkyrie
