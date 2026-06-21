#include "physics/systems/constraint_solver_system.h"

namespace Vulkyrie {

    ConstraintSolverSystem::ConstraintSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _ballAndSocketJointSolverSystem(world, enableWarmStartup)
        , _fixedJointSolverSystem(world, enableWarmStartup)
        , _hingeJointSolverSystem(world, enableWarmStartup)
        , _sliderJointSolverSystem(world, enableWarmStartup)
        , _enableWarmStartup(enableWarmStartup) {
    }

    void ConstraintSolverSystem::Initialize(Timestep timestep) {
        const f32 biasFactor = BETA / timestep.GetSeconds();
        _ballAndSocketJointSolverSystem.InitializeBeforeSolving(biasFactor);
        _fixedJointSolverSystem.InitializeBeforeSolving(biasFactor);
        _hingeJointSolverSystem.InitializeBeforeSolving(biasFactor);
        _sliderJointSolverSystem.InitializeBeforeSolving(biasFactor);

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
