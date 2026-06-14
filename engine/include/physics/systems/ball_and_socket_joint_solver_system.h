#pragma once

#include "vlkypch.h"
#include "physics/components/ball_and_socket_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class BallAndSocketJointSolverSystem final {
    public:
        BallAndSocketJointSolverSystem(PhysicsWorld &world,
                                       RigidBodyComponentStore &rigidBodyStore,
                                       TransformComponentStore &transformStore,
                                       JointComponentStore &jointStore,
                                       BallAndSocketJointComponentStore &basStore);

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJointSolverSystem);

        ~BallAndSocketJointSolverSystem() = default;

        [[nodiscard]] VE_INLINE static f32 ComputeCurrentConeHalfAngle([[maybe_unused]] glm::vec3 coneLimitWorldAxisBodyOne,
                                                                       [[maybe_unused]] glm::vec3 coneLimitWorldAxisBodyTwo) {
            return std::acos(glm::dot(coneLimitWorldAxisBodyOne, coneLimitWorldAxisBodyTwo));
        }

        VE_INLINE void SetWarmStartFlag(bool enableWarmStart) {
            _enableWarmStart = enableWarmStart;
        }

        void InitializeBeforeSolve(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint();
        void SolvePositionConstraint();

    private:
        PhysicsWorld &_physicsWorld;
        RigidBodyComponentStore &_rigidBodyStore;
        TransformComponentStore &_transformStore;
        JointComponentStore &_jointStore;
        BallAndSocketJointComponentStore &_basStore;
        bool _enableWarmStart;
    };

} // namespace Vulkyrie
