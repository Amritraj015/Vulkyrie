#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/body/rigid_body.h"
#include "core/utilities.h"

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

    void BallAndSocketJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
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
        //
        //     const glm::vec3 &localAnchorPointOnBodyOne = _basStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
        //     const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
        //     const glm::vec3 newVecOne = bodyOneTransform.Rotation * (localAnchorPointOnBodyOne - localCenterOfMassBodyOne);
        //
        //     const glm::vec3 &localAnchorPointOnBodyTwo = _basStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);
        //     const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);
        //     const glm::vec3 newVecTwo = bodyTwoTransform.Rotation * (localAnchorPointOnBodyTwo - localCenterOfMassBodyTwo);
        //
        //     _basStore.SetR1WorldAtIndex(i, newVecOne);
        //     _basStore.SetR2WorldAtIndex(i, newVecTwo);
        //
        //     const glm::vec3 &r1World = _basStore.GetR1WorldAtIndex(i);
        //     const glm::vec3 &r2World = _basStore.GetR2WorldAtIndex(i);
        //     const glm::mat3 skewSymmetricMatrixU1 = SkewSymmetric(r1World);
        //     const glm::mat3 skewSymmetricMatrixU2 = SkewSymmetric(r2World);
        //
        //     const f32 bodyOneInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
        //     const f32 bodyTwoInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
        //     const f32 totalInverseMass = bodyOneInverseMass + bodyTwoInverseMass;
        //
        //     const glm::mat3 &inertiaTensorBodyOne = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
        //     const glm::mat3 &inertiaTensorBodyTwo = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);
        // }
    }

    void BallAndSocketJointSolverSystem::WarmStart() {
        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); i++) {
            const Entity jointEntity = _basStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            const glm::vec3 &r1World = _basStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2World = _basStore.GetR2WorldAtIndex(i);

            const glm::mat3 &inertiaTensorBodyOne = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // Compute the translational impulse P=J^T * lambda for body 1 (negated, opposing the constraint direction)
            const glm::vec3 linearImpulseBodyOne = -_basStore.GetImpulseAtIndex(i);
            glm::vec3 angularImpulseBodyOne = glm::cross(_basStore.GetImpulseAtIndex(i), r1World);

            // Compute the cone limit impulse vector and accumulate into body 1's angular impulse
            const glm::vec3 coneLimitImpulse = _basStore.GetConeLimitImpulseAtIndex(i) * _basStore.GetConeLimitAxesCrossProductAtIndex(i);
            angularImpulseBodyOne += coneLimitImpulse;

            const glm::vec3 &linearVelocityBodyOne = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &linearVelocityBodyTwo = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularVelocityBodyOne = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &angularVelocityBodyTwo = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Apply the warm-start impulse to body 1
            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newLinearVelocityBodyOne = linearVelocityBodyOne + inverseMassBodyOne * linearLockAxisFactorBodyOne * linearImpulseBodyOne;
            const glm::vec3 newAngularVelocityBodyOne =
                angularVelocityBodyOne + _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (inertiaTensorBodyOne * angularImpulseBodyOne);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newLinearVelocityBodyOne);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newAngularVelocityBodyOne);

            // Compute the translational and cone limit impulse for body 2 (opposite direction)
            glm::vec3 angularImpulseBodyTwo = -glm::cross(_basStore.GetImpulseAtIndex(i), r2World);
            angularImpulseBodyTwo += -coneLimitImpulse;

            // Apply the warm-start impulse to body 2
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newLinearVelocityBodyTwo =
                linearVelocityBodyTwo + inverseMassBodyTwo * linearLockAxisFactorBodyTwo * _basStore.GetImpulseAtIndex(i);
            const glm::vec3 newAngularVelocityBodyTwo =
                angularVelocityBodyTwo + _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (inertiaTensorBodyTwo * angularImpulseBodyTwo);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newLinearVelocityBodyTwo);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newAngularVelocityBodyTwo);
        }
    }

    void BallAndSocketJointSolverSystem::SolveVelocityConstraint() {
        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _basStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            const glm::vec3 &bodyOneLinearVelocity = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &bodyTwoLinearVelocity = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &bodyOneAngularVelocity = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &bodyTwoAngularVelocity = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const glm::mat3 &inertiaTensorBodyOne = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // --------------- Cone Limit Constraint --------------- //

            if (_basStore.ConeLimitEnabledAtIndex(i) && _basStore.ConeLimitViolatedAtIndex(i)) {
                const glm::vec3 &axesCrossProduct = _basStore.GetConeLimitAxesCrossProductAtIndex(i);

                // Compute Jv for the cone limit: J = [0, axesCrossProduct, 0, -axesCrossProduct]
                const f32 jvConeLimit = glm::dot(axesCrossProduct, bodyOneAngularVelocity - bodyTwoAngularVelocity);

                // Compute the Lagrange multiplier delta and clamp to [0, inf) (unilateral constraint)
                f32 deltaLambdaConeLimit = _basStore.GetInverseMassMatrixConeLimitAtIndex(i) * (-jvConeLimit - _basStore.GetConeLimitBiasAtIndex(i));
                const f32 oldConeLimitImpulse = _basStore.GetConeLimitImpulseAtIndex(i);
                const f32 newConeLimitImpulse = std::max(oldConeLimitImpulse + deltaLambdaConeLimit, f32(0.0));
                _basStore.SetConeLimitImpulseAtIndex(i, newConeLimitImpulse);
                deltaLambdaConeLimit = newConeLimitImpulse - oldConeLimitImpulse;

                // Apply the cone limit impulse P=J^T * lambda to both bodies (angular only)
                const glm::vec3 angularImpulseBodyOne = deltaLambdaConeLimit * axesCrossProduct;
                const glm::vec3 newAngularVelocityBodyOne =
                    bodyOneAngularVelocity + (angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne));
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newAngularVelocityBodyOne);

                const glm::vec3 angularImpulseBodyTwo = -deltaLambdaConeLimit * axesCrossProduct;
                const glm::vec3 newAngularVelocityBodyTwo =
                    bodyTwoAngularVelocity + (angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBodyTwo));
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newAngularVelocityBodyTwo);
            }

            // --------------- Translational Joint Constraint --------------- //

            const glm::vec3 &r1World = _basStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2World = _basStore.GetR2WorldAtIndex(i);

            // Compute Jv = v2 + w2 x r2 - v1 - w1 x r1 (relative velocity at the anchor point)
            const glm::vec3 jvTranslation =
                bodyTwoLinearVelocity + glm::cross(bodyTwoAngularVelocity, r2World) - bodyOneLinearVelocity - glm::cross(bodyOneAngularVelocity, r1World);

            // Compute the Lagrange multiplier delta and accumulate the total impulse
            const glm::mat3 inverseMassMatrix = _basStore.GetInverseMassMatrixAtIndex(i);
            const glm::vec3 deltaLambda = inverseMassMatrix * (-jvTranslation - _basStore.GetBiasVectorAtIndex(i));
            _basStore.SetImpulseAtIndex(i, _basStore.GetImpulseAtIndex(i) + deltaLambda);

            // Apply the translational impulse P=J^T * lambda to body 1
            const glm::vec3 linearImpulseBodyOne = -deltaLambda;
            const glm::vec3 angularImpulseBodyOne = glm::cross(deltaLambda, r1World);

            const f32 bodyOneInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const glm::vec3 linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newLinearVelocityBodyOne = bodyOneLinearVelocity + bodyOneInverseMass * linearLockAxisFactorBodyOne * linearImpulseBodyOne;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newLinearVelocityBodyOne);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(
                bodyOneIndex, bodyOneAngularVelocity + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne));

            // Apply the translational impulse P=J^T * lambda to body 2
            const glm::vec3 angularImpulseBodyTwo = glm::cross(-deltaLambda, r2World);

            const f32 bodyTwoInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const glm::vec3 linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newLinearVelocityBodyTwo = bodyTwoLinearVelocity + bodyTwoInverseMass * linearLockAxisFactorBodyTwo * deltaLambda;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newLinearVelocityBodyTwo);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(
                bodyTwoIndex, bodyTwoAngularVelocity + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBodyTwo));
        }
    }

    void BallAndSocketJointSolverSystem::SolvePositionConstraint() {
        // for (size_t i = 0; i < _basStore.GetActiveComponentCount(); ++i) {
        //     const Entity jointEntity = _basStore.GetEntityAtIndex(i);
        //     const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);
        //
        //     if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
        //         continue;
        //     }
        //
        //     const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
        //     const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);
        //
        //     const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
        //     const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);
        //
        //     const glm::quat &bodyOneOrientation = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
        //     const glm::quat &bodyTwoOrientation = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);
        //
        //     const glm::vec3 &bodyOneLocalInertiaTensor = _rigidBodyStore.GetLocalInertiaTensorAtIndex(bodyOneIndex);
        //     const glm::vec3 &bodyTwoLocalInertiaTensor = _rigidBodyStore.GetLocalInertiaTensorAtIndex(bodyTwoIndex);
        //
        //     const glm::mat3 &bodyOneWorldInertiaTensor = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
        //     const glm::mat3 &bodyTwoWorldInertiaTensor = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);
        //
        //     RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(bodyOneOrientation), bodyOneLocalInertiaTensor, bodyOneWorldInertiaTensor);
        //     RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(bodyTwoOrientation), bodyTwoLocalInertiaTensor, bodyTwoWorldInertiaTensor);
        // }
    }

} // namespace Vulkyrie
