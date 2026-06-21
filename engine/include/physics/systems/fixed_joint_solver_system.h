#pragma once

#include "vlkypch.h"
#include "physics/components/fixed_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class FixedJointSolverSystem {
    public:
        explicit FixedJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(FixedJointSolverSystem);

        ~FixedJointSolverSystem() = default;

        void InitializeBeforeSolving(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint();
        void SolvePositionConstraint();

    private:
        RigidBodyComponentStore &_rigidBodyStore;
        TransformComponentStore &_transformStore;
        JointComponentStore &_jointStore;
        FixedJointComponentStore &_fixedJointStore;
        bool &_enableWarmStartup;
    };

} // namespace Vulkyrie
