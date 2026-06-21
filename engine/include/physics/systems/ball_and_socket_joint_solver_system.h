#pragma once

#include "vlkypch.h"
#include "physics/components/ball_and_socket_joint_component_store.h"
#include "physics/components/joint_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class BallAndSocketJointSolverSystem final {
    public:
        explicit BallAndSocketJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup);

        VE_DELETE_MOVE_AND_COPY(BallAndSocketJointSolverSystem);

        ~BallAndSocketJointSolverSystem() = default;

        [[nodiscard]] VE_INLINE static f32 ComputeCurrentConeHalfAngle([[maybe_unused]] glm::vec3 coneLimitWorldAxisBodyOne,
                                                                       [[maybe_unused]] glm::vec3 coneLimitWorldAxisBodyTwo) {
            return std::acos(glm::dot(coneLimitWorldAxisBodyOne, coneLimitWorldAxisBodyTwo));
        }

        void InitializeBeforeSolving(f32 biasFactor);
        void WarmStart();
        void SolveVelocityConstraint();
        void SolvePositionConstraint();

    private:
        RigidBodyComponentStore &_rigidBodyStore;
        TransformComponentStore &_transformStore;
        JointComponentStore &_jointStore;
        BallAndSocketJointComponentStore &_basStore;
        bool &_enableWarmStartup;
    };

} // namespace Vulkyrie
