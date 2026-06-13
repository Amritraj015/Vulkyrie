#pragma once

#include "vlkypch.h"
#include "core/time_step.h"
#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/systems/fixed_joint_solver_system.h"
#include "physics/systems/hinge_joint_solver_system.h"
#include "physics/systems/slider_joint_solver_system.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"

namespace Vulkyrie {

    struct ConstraintSolverData {
        Vulkyrie::Timestep Timestep;
        bool EnableWarmStartup;
        RigidBodyComponentStore &RigidBodyStore;
        JointComponentStore &JointStore;
    };

    class ConstraintSolverSystem final {
    public:
        ~ConstraintSolverSystem() = default;

        void SolvePositionConstraints();

        [[nodiscard]] VE_INLINE f32 ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation) {
            return _hingeJointSolverSystem.ComputeCurrentHingeAngle(jointEntity, bodyOneOrientation, bodyTwoOrientation);
        }

    private:
        BallAndSocketJointSolverSystem _ballAndSocketJointSolverSystem;
        FixedJointSolverSystem _fixedJointSolverSystem;
        HingeJointSolverSystem _hingeJointSolverSystem;
        SliderJointSolverSystem _sliderJointSolverSystem;
    };

} // namespace Vulkyrie
