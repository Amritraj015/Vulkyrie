#include "physics/systems/ball_and_socket_joint_solver_system.h"

namespace Vulkyrie {

    BallAndSocketJointSolverSystem::BallAndSocketJointSolverSystem(PhysicsWorld &world,
                                                                   RigidBodyComponentStore &rigidBodyStore,
                                                                   TransformComponentStore &transformStore,
                                                                   JointComponentStore &jointStore,
                                                                   BallAndSocketJointComponentStore &basStore)
        : _physicsWorld(world)
        , _rigidBodyStore(rigidBodyStore)
        , _transformStore(transformStore)
        , _jointStore(jointStore)
        , _basStore(basStore)
        , _enableWarmStart(true) {
    }

    void BallAndSocketJointSolverSystem::InitializeBeforeSolve(f32 biasFactor) {
        // for (size_t i = 0; _basStore.GetActiveComponentCount(); ++i) {
        //     const Entity jointEntity = _basStore.GetEntityAtIndex(i);
        //     const size_t entityIndex = _jointStore.GetEntityIndex(jointEntity);
        //
        //     const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(entityIndex);
        //     const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(entityIndex);
        //
        //     VASSERT(!_rigidBodyStore.IsDisabled(bodyOneEntity) || !_rigidBodyStore.IsDisabled(bodyTwoEntity), "Both bodies must be active.");
        //
        //     const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
        //     const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);
        //
        //     _basStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex));
        //     _basStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex));
        //
        //     const TransformComponent &bodyOneTransform = _transformStore.GetTransform(bodyOneEntity);
        //     const TransformComponent &bodyTwoTransform = _transformStore.GetTransform(bodyTwoEntity);
        //     const glm::quat &bodyOneRotation = bodyOneTransform.Rotation;
        //     const glm::quat &bodyTwoRotation = bodyTwoTransform.Rotation;
        // }
    }

    void BallAndSocketJointSolverSystem::WarmStart() {
        // For each joint component
        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); i++) {
            const Entity jointEntity = _basStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            const glm::vec3 &r1World = _basStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2World = _basStore.GetR2WorldAtIndex(i);

            const glm::mat3 &i1 = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &i2 = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // Compute the impulse P=J^T * lambda for the body 1
            glm::vec3 linearImpulseBodyOne = -_basStore.GetImpulseAtIndex(i);
            glm::vec3 angularImpulseBodyOne = glm::cross(_basStore.GetImpulseAtIndex(i), r1World);

            // Compute the impulse P=J^T * lambda for the lower and upper limits constraints
            const glm::vec3 coneLimitImpulse = _basStore.GetConeLimitImpulseAtIndex(i) * _basStore.GetConeLimitAxesCrossProductAtIndex(i);

            // Compute the impulse P=J^T * lambda for the cone limit constraint of body 1
            angularImpulseBodyOne += coneLimitImpulse;

            // Get the velocities
            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Apply the impulse to the body 1
            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newV1 = v1 + inverseMassBodyOne * linearLockAxisFactorBodyOne * linearImpulseBodyOne;
            const glm::vec3 newW1 = w1 + _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (i1 * angularImpulseBodyOne);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

            // Compute the impulse P=J^T * lambda for the body 2
            glm::vec3 angularImpulseBodyTwo = -glm::cross(_basStore.GetImpulseAtIndex(i), r2World);

            // Compute the impulse P=J^T * lambda for the cone limit constraint of body 2
            angularImpulseBodyTwo += -coneLimitImpulse;

            // Apply the impulse to the body to body 2
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newV2 = v2 + inverseMassBodyTwo * linearLockAxisFactorBodyTwo * _basStore.GetImpulseAtIndex(i);
            const glm::vec3 newW2 = w2 + _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (i2 * angularImpulseBodyTwo);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
        }
    }

    void BallAndSocketJointSolverSystem::SolveVelocityConstraint() {
    }

    void BallAndSocketJointSolverSystem::SolvePositionConstraint() {
    }

} // namespace Vulkyrie
