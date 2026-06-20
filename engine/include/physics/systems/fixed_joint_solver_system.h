#pragma once

#include "vlkypch.h"
#include "physics/components/fixed_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class FixedJointSolverSystem {
    public:
        FixedJointSolverSystem(PhysicsWorld &world,
                               RigidBodyComponentStore &rigidBodyStore,
                               TransformComponentStore &transformStore,
                               JointComponentStore &jointStore,
                               FixedJointComponentStore &fixedJointStore);

        VE_DELETE_MOVE_AND_COPY(FixedJointSolverSystem);

        ~FixedJointSolverSystem() = default;

        VE_INLINE void SetWarmStartFlag(bool enableWarmStart) {
            _enableWarmStart = enableWarmStart;
        }

        void InitializeBeforeSolving(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint();
        void SolvePositionConstraint();

    private:
        PhysicsWorld &_physicsWorld;
        RigidBodyComponentStore &_rigidBodyStore;
        TransformComponentStore &_transformStore;
        JointComponentStore &_jointStore;
        FixedJointComponentStore &_fixedJointStore;
        bool _enableWarmStart;
    };

} // namespace Vulkyrie
