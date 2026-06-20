#pragma once

#include "vlkypch.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/slider_joint_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class SliderJointSolverSystem {
    public:
        SliderJointSolverSystem(PhysicsWorld &world,
                                RigidBodyComponentStore &rigidBodyComponents,
                                TransformComponentStore &transformComponents,
                                JointComponentStore &jointComponents,
                                SliderJointComponentStore &sliderJointComponents);

        VE_DELETE_MOVE_AND_COPY(SliderJointSolverSystem);

        ~SliderJointSolverSystem() = default;

        VE_INLINE void SetWarmStartFlag(bool enableWarmStart) {
            _enableWarmStart = enableWarmStart;
        }

        void InitializeBeforeSolving(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint(Timestep timestep);
        void SolvePositionConstraint();

    private:
        PhysicsWorld &_physicsWorld;
        RigidBodyComponentStore &_rigidBodyStore;
        TransformComponentStore &_transformStore;
        JointComponentStore &_jointStore;
        SliderJointComponentStore &_sliderJointStore;
        bool _enableWarmStart;
    };

} // namespace Vulkyrie
