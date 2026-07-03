#pragma once

#include "vlkypch.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/slider_joint_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class SliderJointSolverSystem {
    public:
        explicit SliderJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(SliderJointSolverSystem);

        ~SliderJointSolverSystem() = default;

        void InitializeBeforeSolving(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint(Timestep timestep);
        void SolvePositionConstraint();

    private:
        [[maybe_unused]] RigidBodyComponentStore &_rigidBodyStore;
        [[maybe_unused]] TransformComponentStore &_transformStore;
        [[maybe_unused]] JointComponentStore &_jointStore;
        [[maybe_unused]] SliderJointComponentStore &_sliderJointStore;
        [[maybe_unused]] bool &_enableWarmStartup;
    };

} // namespace Vulkyrie
