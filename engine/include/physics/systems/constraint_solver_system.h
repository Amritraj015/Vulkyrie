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

    class ConstraintSolverSystem final {
    public:
        ConstraintSolverSystem(PhysicsWorld &world,
                               RigidBodyComponentStore &rigidBodyStore,
                               TransformComponentStore &transformStore,
                               JointComponentStore &jointStore,
                               BallAndSocketJointComponentStore &ballAndSocketJointStore,
                               FixedJointComponentStore &fixedJointStore,
                               HingeJointComponentStore &hingeJointStore,
                               SliderJointComponentStore &sliderJointStore);

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
        bool _enableWarmStartup;
    };

} // namespace Vulkyrie
