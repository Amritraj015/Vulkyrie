#pragma once

#include "core/entity.h"
#include "physics/components/hinge_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class HingeJointSolverSystem {
    public:
        explicit HingeJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(HingeJointSolverSystem);

        ~HingeJointSolverSystem() = default;

        void InitializeBeforeSolving(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint(Timestep timestep);
        void SolvePositionConstraint();
        f32 ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation);

    private:
        RigidBodyComponentStore &_rigidBodyStore;
        TransformComponentStore &_transformStore;
        JointComponentStore &_jointStore;
        HingeJointComponentStore &_hingeJointStore;
        [[maybe_unused]] bool &_enableWarmStartup;

        constexpr static f32 TWICE_PI = 2 * std::numbers::pi_v<f32>;

        f32 computeNormalizedAngle(f32 angle) const;
        f32 computeCorrespondingAngleNearLimits(f32 inputAngle, f32 lowerLimitAngle, f32 upperLimitAngle) const;
    };

} // namespace Vulkyrie
