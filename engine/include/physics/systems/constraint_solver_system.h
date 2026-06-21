#pragma once

#include "vlkypch.h"
#include "core/time_step.h"
#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/systems/fixed_joint_solver_system.h"
#include "physics/systems/hinge_joint_solver_system.h"
#include "physics/systems/slider_joint_solver_system.h"

namespace Vulkyrie {

    class ConstraintSolverSystem final {
    public:
        ConstraintSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(ConstraintSolverSystem);

        ~ConstraintSolverSystem() = default;

        [[nodiscard]] VE_INLINE f32 ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation) {
            return _hingeJointSolverSystem.ComputeCurrentHingeAngle(jointEntity, bodyOneOrientation, bodyTwoOrientation);
        }

        void Initialize(Timestep timestep);
        void SolveVelocityConstraints(Timestep timestep);
        void SolvePositionConstraints();

    private:
        static constexpr f32 BETA = f32(0.2);

        BallAndSocketJointSolverSystem _ballAndSocketJointSolverSystem;
        FixedJointSolverSystem _fixedJointSolverSystem;
        HingeJointSolverSystem _hingeJointSolverSystem;
        SliderJointSolverSystem _sliderJointSolverSystem;
        bool &_enableWarmStartup;
    };

} // namespace Vulkyrie
