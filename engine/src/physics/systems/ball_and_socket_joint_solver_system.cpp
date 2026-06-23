#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"
#include "core/utilities.h"

namespace Vulkyrie {

    BallAndSocketJointSolverSystem::BallAndSocketJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _basStore(world.GetBallAndSocketJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void BallAndSocketJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _basStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            VASSERT(!_rigidBodyStore.IsDisabled(bodyOneEntity) || !_rigidBodyStore.IsDisabled(bodyTwoEntity), "Both bodies must be active.");

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            _basStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex));
            _basStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex));

            const TransformComponent &bodyOneTransform = _transformStore.GetTransform(bodyOneEntity);
            const TransformComponent &bodyTwoTransform = _transformStore.GetTransform(bodyTwoEntity);

            const glm::vec3 &localAnchorPointOnBodyOne = _basStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 newVecOne = bodyOneTransform.Rotation * (localAnchorPointOnBodyOne - localCenterOfMassBodyOne);

            const glm::vec3 &localAnchorPointOnBodyTwo = _basStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);
            const glm::vec3 newVecTwo = bodyTwoTransform.Rotation * (localAnchorPointOnBodyTwo - localCenterOfMassBodyTwo);

            _basStore.SetR1WorldAtIndex(i, newVecOne);
            _basStore.SetR2WorldAtIndex(i, newVecTwo);

            const glm::vec3 &r1World = _basStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2World = _basStore.GetR2WorldAtIndex(i);
            const glm::mat3 skewSymmetricMatrixU1 = SkewSymmetric(r1World);
            const glm::mat3 skewSymmetricMatrixU2 = SkewSymmetric(r2World);

            const f32 bodyOneInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 bodyTwoInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 totalInverseMass = bodyOneInverseMass + bodyTwoInverseMass;

            const glm::mat3 &inertiaTensorBodyOne = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::mat3 massMatrix = glm::mat3(glm::vec3(totalInverseMass, 0, 0), //
                                                   glm::vec3(0, totalInverseMass, 0), //
                                                   glm::vec3(0, 0, totalInverseMass)  //
                                                   ) +
                                         skewSymmetricMatrixU1 * inertiaTensorBodyOne * glm::transpose(skewSymmetricMatrixU1) +
                                         skewSymmetricMatrixU2 * inertiaTensorBodyTwo * glm::transpose(skewSymmetricMatrixU2);

            _basStore.SetInverseMassMatrixAtIndex(i, glm::mat3(0));
            const f32 massMatrixDeterminant = glm::determinant(massMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _basStore.SetInverseMassMatrixAtIndex(i, InverseMat3(massMatrix, massMatrixDeterminant));
                }
            }

            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            _basStore.SetBiasVectorAtIndex(i, glm::vec3(0.0f));

            if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                _basStore.SetBiasVectorAtIndex(i, biasFactor * (x2 + r2World - x1 - r1World));
            }

            const glm::vec3 r1WorldUnit = glm::normalize(r1World);
            const glm::vec3 r2WorldUnit = glm::normalize(r2World);
            _basStore.SetConeLimitAxesCrossProductAtIndex(i, glm::cross(r1WorldUnit, -r2WorldUnit));

            const f32 coneAngle = ComputeCurrentConeHalfAngle(r1WorldUnit, -r2WorldUnit);
            const f32 coneLimitError = _basStore.GetConeLimitHalfAngleAtIndex(i) - coneAngle;

            const bool oldConeLimitViolated = _basStore.ConeLimitViolatedAtIndex(i);
            const bool coneLimitViolated = coneLimitError < 0;
            _basStore.SetConeLimitViolatedFlagAtIndex(i, coneLimitViolated);

            if (!coneLimitViolated || coneLimitViolated != oldConeLimitViolated) {
                _basStore.SetConeLimitImpulseAtIndex(i, f32(0.0f));
            }

            if (_basStore.ConeLimitEnabledAtIndex(i)) {
                // Compute the inverse of the mass matrix K=JM^-1J^t for the cone limit
                const glm::vec3 coneLimitAxisCrossProduct = _basStore.GetConeLimitAxesCrossProductAtIndex(i);

                f32 inverseMassMatrixConeLimit = glm::dot(coneLimitAxisCrossProduct, inertiaTensorBodyOne * coneLimitAxisCrossProduct) +
                                                 glm::dot(coneLimitAxisCrossProduct, inertiaTensorBodyTwo * coneLimitAxisCrossProduct);
                inverseMassMatrixConeLimit = (inverseMassMatrixConeLimit > f32(0.0)) ? f32(1.0) / inverseMassMatrixConeLimit : f32(0.0);

                _basStore.SetInverseMassMatrixConeLimitAtIndex(i, inverseMassMatrixConeLimit);

                // Compute the bias "b" of the lower limit constraint.
                if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                    _basStore.SetConeLimitBiasAtIndex(i, biasFactor * coneLimitError);
                } else {
                    _basStore.SetConeLimitBiasAtIndex(i, f32(0.0f));
                }
            }

            if (!_enableWarmStartup) {
                _basStore.SetImpulseAtIndex(i, glm::vec3(0.0f));
            }
        }
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
        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _basStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            const glm::quat &bodyOneOrientation = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &bodyTwoOrientation = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            // Recompute the world-space inverse inertia tensors from the current constrained orientations,
            // since the orientations may have changed during previous position-solving iterations.
            const glm::vec3 &bodyOneLocalInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &bodyTwoLocalInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            const glm::mat3 &inertiaTensorBodyOne = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            glm::mat3 bodyOneWorldInertiaTensor;
            glm::mat3 bodyTwoWorldInertiaTensor;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(bodyOneOrientation), bodyOneLocalInertiaTensor, bodyOneWorldInertiaTensor);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(bodyTwoOrientation), bodyTwoLocalInertiaTensor, bodyTwoWorldInertiaTensor);

            _basStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, bodyOneWorldInertiaTensor);
            _basStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, bodyTwoWorldInertiaTensor);

            // Recompute the anchor-point offset vectors r1, r2 in world space from the current constrained orientations.
            const glm::vec3 rOneWorld =
                bodyOneOrientation * (_basStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex));
            const glm::vec3 rTwoWorld =
                bodyTwoOrientation * (_basStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex));

            _basStore.SetR1WorldAtIndex(i, rOneWorld);
            _basStore.SetR2WorldAtIndex(i, rTwoWorld);

            // --------------- Cone Limit Constraint --------------- //

            if (_basStore.ConeLimitEnabledAtIndex(i)) {
                const glm::vec3 rOneWorldUnit = glm::normalize(rOneWorld);
                const glm::vec3 rTwoWorldUnit = glm::normalize(rTwoWorld);

                // Recompute the cone limit rotation axis (r1 x -r2) and the current cone half-angle error.
                const glm::vec3 coneLimitAxesCrossProduct = glm::cross(rOneWorldUnit, -rTwoWorldUnit);
                _basStore.SetConeLimitAxesCrossProductAtIndex(i, coneLimitAxesCrossProduct);

                const f32 coneAngle = ComputeCurrentConeHalfAngle(rOneWorldUnit, -rTwoWorldUnit);
                const f32 coneLimitError = _basStore.GetConeLimitHalfAngleAtIndex(i) - coneAngle;
                const bool coneLimitViolated = coneLimitError < 0;
                _basStore.SetConeLimitViolatedFlagAtIndex(i, coneLimitViolated);

                if (coneLimitViolated) {
                    // Compute the inverse of the mass matrix K=JM^-1J^t for the cone limit (1x1 matrix)
                    f32 inverseMassMatrixConeLimit = glm::dot(coneLimitAxesCrossProduct, inertiaTensorBodyOne * coneLimitAxesCrossProduct) +
                                                     glm::dot(coneLimitAxesCrossProduct, inertiaTensorBodyTwo * coneLimitAxesCrossProduct);
                    inverseMassMatrixConeLimit = inverseMassMatrixConeLimit > f32(0.0) ? f32(1.0) / inverseMassMatrixConeLimit : f32(0.0);

                    _basStore.SetInverseMassMatrixConeLimitAtIndex(i, inverseMassMatrixConeLimit);

                    // Compute the Lagrange multiplier lambda for the cone limit constraint.
                    const f32 lambdaConeLimit = inverseMassMatrixConeLimit * (-coneLimitError);

                    // Compute the impulse P=J^T * lambda of body 1 and apply the pseudo-velocity to correct its orientation.
                    const glm::vec3 angularImpulseBodyOne = lambdaConeLimit * coneLimitAxesCrossProduct;
                    const glm::vec3 w1 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (inertiaTensorBodyOne * angularImpulseBodyOne);
                    const glm::quat q1 = glm::normalize(bodyOneOrientation + (glm::quat(0, w1) * bodyOneOrientation * f32(0.5)));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, q1);

                    // Compute the impulse P=J^T * lambda of body 2 and apply the pseudo-velocity to correct its orientation.
                    const glm::vec3 angularImpulseBodyTwo = -lambdaConeLimit * coneLimitAxesCrossProduct;
                    const glm::vec3 w2 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (inertiaTensorBodyTwo * angularImpulseBodyTwo);
                    const glm::quat q2 = glm::normalize(bodyTwoOrientation + (glm::quat(0, w2) * bodyTwoOrientation * f32(0.5)));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, q2);
                }
            }

            // --------------- Translational Joint Constraint --------------- //

            // Compute the skew-symmetric matrices of r1 and r2 for use in the mass matrix K=JM^-1J^t.
            const glm::mat3 skewSymmetricMatrixU1 = SkewSymmetric(rOneWorld);
            const glm::mat3 skewSymmetricMatrixU2 = SkewSymmetric(rTwoWorld);

            // Get the inverse masses of the bodies.
            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Recompute the mass matrix K=JM^-1J^t for the 3 translational constraints.
            const f32 inverseMassBodies = inverseMassBodyOne + inverseMassBodyTwo;
            const glm::mat3 massMatrix = glm::mat3(glm::vec3(inverseMassBodies, 0, 0), //
                                                   glm::vec3(0, inverseMassBodies, 0), //
                                                   glm::vec3(0, 0, inverseMassBodies)  //
                                                   ) +
                                         skewSymmetricMatrixU1 * inertiaTensorBodyOne * glm::transpose(skewSymmetricMatrixU1) +
                                         skewSymmetricMatrixU2 * inertiaTensorBodyTwo * glm::transpose(skewSymmetricMatrixU2);

            const f32 massMatrixDeterminant = glm::determinant(massMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {

                    const glm::vec3 &x1 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
                    const glm::vec3 &x2 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);

                    // Compute the constraint error (value of the C(x) function)
                    const glm::vec3 constraintError = (x2 + rTwoWorld - x1 - rOneWorld);

                    // Solve K * lambda = -constraintError directly via Cramer's rule, without forming K^-1.
                    // invDet is computed once and reused as multiplications to avoid 3 expensive divisions.
                    const glm::vec3 b = -constraintError;
                    const f32 invDet = f32(1.0) / massMatrixDeterminant;
                    const glm::vec3 lambda(glm::determinant(glm::mat3(b, massMatrix[1], massMatrix[2])) * invDet,
                                           glm::determinant(glm::mat3(massMatrix[0], b, massMatrix[2])) * invDet,
                                           glm::determinant(glm::mat3(massMatrix[0], massMatrix[1], b)) * invDet);

                    // Compute the impulse P=J^T * lambda of body 1
                    const glm::vec3 linearImpulseBody1 = -lambda;
                    const glm::vec3 angularImpulseBody1 = glm::cross(lambda, rOneWorld);

                    // Compute the pseudo velocity of body 1 and update its position and orientation.
                    const glm::vec3 v1 = inverseMassBodyOne * _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex) * linearImpulseBody1;
                    const glm::vec3 w1 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (inertiaTensorBodyOne * angularImpulseBody1);
                    const glm::quat q1 = glm::normalize(bodyOneOrientation + glm::quat(0, w1) * bodyOneOrientation * f32(0.5));

                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, q1);

                    // Compute the impulse P=J^T * lambda of body 2
                    const glm::vec3 angularImpulseBody2 = glm::cross(-lambda, rTwoWorld);

                    // Compute the pseudo velocity of body 2 and update its position and orientation.
                    const glm::vec3 v2 = inverseMassBodyTwo * _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex) * lambda;
                    const glm::vec3 w2 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (inertiaTensorBodyTwo * angularImpulseBody2);
                    const glm::quat q2 = glm::normalize(bodyTwoOrientation + glm::quat(0, w2) * bodyTwoOrientation * f32(0.5));

                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, q2);
                }
            }
        }
    }

} // namespace Vulkyrie
