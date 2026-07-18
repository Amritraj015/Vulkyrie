#include "physics/systems/ball_and_socket_joint_solver_system.h"
#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"
#include "core/asserts.h"
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
        const size_t activeJointCount = _basStore.GetActiveComponentCount();
        _jointIndices.resize(activeJointCount);

        for (size_t i = 0; i < activeJointCount; ++i) {
            const Entity jointEntity = _basStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            VASSERT(!_rigidBodyStore.EntityDisabled(bodyOneEntity) || !_rigidBodyStore.EntityDisabled(bodyTwoEntity), "Both bodies must be active.");

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            // Resolve the entity-to-index hash lookups once per step; WarmStart and the solver phases run
            // every iteration and read the cached indices instead of repeating them.
            _jointIndices[i] = { jointIndex, bodyOneIndex, bodyTwoIndex };

            const bool baumgartePositionCorrection =
                JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex);

            const glm::mat3 &invI1 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex);
            const glm::mat3 &invI2 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex);
            _basStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _basStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            const TransformComponent &bodyOneTransform = _transformStore.GetTransform(bodyOneEntity);
            const TransformComponent &bodyTwoTransform = _transformStore.GetTransform(bodyTwoEntity);

            const glm::vec3 &localAnchorPointOnBodyOne = _basStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 r1 = bodyOneTransform.Rotation * (localAnchorPointOnBodyOne - localCenterOfMassBodyOne);

            const glm::vec3 &localAnchorPointOnBodyTwo = _basStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);
            const glm::vec3 r2 = bodyTwoTransform.Rotation * (localAnchorPointOnBodyTwo - localCenterOfMassBodyTwo);

            _basStore.SetR1WorldAtIndex(i, r1);
            _basStore.SetR2WorldAtIndex(i, r2);

            const glm::mat3 skewR1 = SkewSymmetric(r1);
            const glm::mat3 skewR2 = SkewSymmetric(r2);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            const glm::mat3 kTranslation = glm::mat3(sumInvMass) + skewR1 * invI1 * glm::transpose(skewR1) + skewR2 * invI2 * glm::transpose(skewR2);

            _basStore.SetInverseMassMatrixAtIndex(i, glm::mat3(0));
            const f32 kTranslationDet = glm::determinant(kTranslation);

            if (VE_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _basStore.SetInverseMassMatrixAtIndex(i, InverseMat3(kTranslation, kTranslationDet));
                }
            }

            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            if (baumgartePositionCorrection) {
                _basStore.SetBiasVectorAtIndex(i, biasFactor * (x2 + r2 - x1 - r1));
            } else {
                _basStore.SetBiasVectorAtIndex(i, glm::vec3(0.0f));
            }

            // All the cone-limit state (rotation axis, violation flag, mass matrix and bias) is only needed
            // when the limit is enabled, so skip the normalizations and the acos entirely when it is not.
            if (_basStore.ConeLimitEnabledAtIndex(i)) {
                const glm::vec3 r1Unit = glm::normalize(r1);
                const glm::vec3 r2Unit = glm::normalize(r2);
                const glm::vec3 coneLimitAxis = glm::cross(r1Unit, -r2Unit);
                _basStore.SetConeLimitAxesCrossProductAtIndex(i, coneLimitAxis);

                const f32 coneAngle = ComputeCurrentConeHalfAngle(r1Unit, -r2Unit);
                const f32 coneLimitError = _basStore.GetConeLimitHalfAngleAtIndex(i) - coneAngle;

                const bool oldConeLimitViolated = _basStore.ConeLimitViolatedAtIndex(i);
                const bool coneLimitViolated = coneLimitError < 0;
                _basStore.SetConeLimitViolatedFlagAtIndex(i, coneLimitViolated);

                if (!coneLimitViolated || coneLimitViolated != oldConeLimitViolated) {
                    _basStore.SetConeLimitImpulseAtIndex(i, f32(0.0f));
                }

                // Compute the mass matrix K=JM^-1J^t for the cone limit (1x1), then its inverse.
                const f32 kConeLimit = glm::dot(coneLimitAxis, invI1 * coneLimitAxis) + glm::dot(coneLimitAxis, invI2 * coneLimitAxis);
                const f32 invKConeLimit = (kConeLimit > f32(0.0)) ? f32(1.0) / kConeLimit : f32(0.0);

                _basStore.SetInverseMassMatrixConeLimitAtIndex(i, invKConeLimit);

                // Compute the bias "b" of the lower limit constraint.
                if (baumgartePositionCorrection) {
                    _basStore.SetConeLimitBiasAtIndex(i, biasFactor * coneLimitError);
                } else {
                    _basStore.SetConeLimitBiasAtIndex(i, f32(0.0f));
                }
            }

            if (!_enableWarmStartup) {
                _basStore.SetImpulseAtIndex(i, glm::vec3(0.0f));
                _basStore.SetConeLimitImpulseAtIndex(i, f32(0.0));
            }
        }
    }

    void BallAndSocketJointSolverSystem::WarmStart() {
        VASSERT(_jointIndices.size() == _basStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before WarmStart().");

        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); i++) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::vec3 &r1 = _basStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2 = _basStore.GetR2WorldAtIndex(i);

            const glm::mat3 &invI1 = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::vec3 &impulse = _basStore.GetImpulseAtIndex(i);

            // Compute the translational impulse P=J^T * lambda for body 1 (negated, opposing the constraint direction)
            const glm::vec3 linearImpulseBody1 = -impulse;
            glm::vec3 angularImpulseBody1 = glm::cross(impulse, r1);

            // Compute the cone limit impulse vector and accumulate into body 1's angular impulse
            const glm::vec3 coneLimitImpulse = _basStore.GetConeLimitImpulseAtIndex(i) * _basStore.GetConeLimitAxesCrossProductAtIndex(i);
            angularImpulseBody1 += coneLimitImpulse;

            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Apply the warm-start impulse to body 1
            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            const glm::vec3 newW1 = w1 + _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (invI1 * angularImpulseBody1);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

            // Compute the translational and cone limit impulse for body 2 (opposite direction)
            glm::vec3 angularImpulseBody2 = -glm::cross(impulse, r2);
            angularImpulseBody2 += -coneLimitImpulse;

            // Apply the warm-start impulse to body 2
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * impulse;
            const glm::vec3 newW2 = w2 + _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (invI2 * angularImpulseBody2);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
        }
    }

    void BallAndSocketJointSolverSystem::SolveVelocityConstraint() {
        VASSERT(_jointIndices.size() == _basStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolveVelocityConstraint().");

        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const glm::mat3 &invI1 = _basStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _basStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // --------------- Cone Limit Constraint --------------- //

            if (_basStore.ConeLimitEnabledAtIndex(i) && _basStore.ConeLimitViolatedAtIndex(i)) {
                const glm::vec3 &coneLimitAxis = _basStore.GetConeLimitAxesCrossProductAtIndex(i);

                // Compute Jv for the cone limit: J = [0, coneLimitAxis, 0, -coneLimitAxis]
                const f32 JvConeLimit = glm::dot(coneLimitAxis, w1 - w2);

                // Compute the Lagrange multiplier delta and clamp to [0, inf) (unilateral constraint)
                f32 deltaLambdaConeLimit = _basStore.GetInverseMassMatrixConeLimitAtIndex(i) * (-JvConeLimit - _basStore.GetConeLimitBiasAtIndex(i));
                const f32 oldConeLimitImpulse = _basStore.GetConeLimitImpulseAtIndex(i);
                const f32 newConeLimitImpulse = std::max(oldConeLimitImpulse + deltaLambdaConeLimit, f32(0.0));
                _basStore.SetConeLimitImpulseAtIndex(i, newConeLimitImpulse);
                deltaLambdaConeLimit = newConeLimitImpulse - oldConeLimitImpulse;

                // Apply the cone limit impulse P=J^T * lambda to both bodies (angular only)
                const glm::vec3 angularImpulseBody1 = deltaLambdaConeLimit * coneLimitAxis;
                const glm::vec3 newW1 = w1 + (angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

                const glm::vec3 angularImpulseBody2 = -deltaLambdaConeLimit * coneLimitAxis;
                const glm::vec3 newW2 = w2 + (angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
            }

            // --------------- Translational Joint Constraint --------------- //

            const glm::vec3 &r1 = _basStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2 = _basStore.GetR2WorldAtIndex(i);

            // Compute Jv = v2 + w2 x r2 - v1 - w1 x r1 (relative velocity at the anchor point)
            const glm::vec3 JvTranslation = v2 + glm::cross(w2, r2) - v1 - glm::cross(w1, r1);

            // Compute the Lagrange multiplier delta and accumulate the total impulse
            const glm::mat3 &invKTranslation = _basStore.GetInverseMassMatrixAtIndex(i);
            const glm::vec3 deltaLambdaTranslation = invKTranslation * (-JvTranslation - _basStore.GetBiasVectorAtIndex(i));
            _basStore.SetImpulseAtIndex(i, _basStore.GetImpulseAtIndex(i) + deltaLambdaTranslation);

            // Apply the translational impulse P=J^T * lambda to body 1
            const glm::vec3 linearImpulseBody1 = -deltaLambdaTranslation;
            const glm::vec3 angularImpulseBody1 = glm::cross(deltaLambdaTranslation, r1);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

            // Apply the translational impulse P=J^T * lambda to body 2
            const glm::vec3 angularImpulseBody2 = glm::cross(-deltaLambdaTranslation, r2);

            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * deltaLambdaTranslation;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
        }
    }

    void BallAndSocketJointSolverSystem::SolvePositionConstraint() {
        VASSERT(_jointIndices.size() == _basStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolvePositionConstraint().");

        for (size_t i = 0; i < _basStore.GetActiveComponentCount(); ++i) {
            const size_t jointIndex = _jointIndices[i].JointIndex;

            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::quat &q1 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &q2 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            const glm::vec3 &invI1Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &invI2Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            // Recompute the world-space inverse inertia tensors from the current constrained orientations,
            // since the orientations may have changed during previous position-solving iterations.
            glm::mat3 invI1;
            glm::mat3 invI2;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q1), invI1Local, invI1);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q2), invI2Local, invI2);

            _basStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _basStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            // Recompute the anchor-point offset vectors r1, r2 in world space from the current constrained orientations.
            const glm::vec3 r1 = q1 * (_basStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex));
            const glm::vec3 r2 = q2 * (_basStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex));

            _basStore.SetR1WorldAtIndex(i, r1);
            _basStore.SetR2WorldAtIndex(i, r2);

            // --------------- Cone Limit Constraint --------------- //

            if (_basStore.ConeLimitEnabledAtIndex(i)) {
                const glm::vec3 r1Unit = glm::normalize(r1);
                const glm::vec3 r2Unit = glm::normalize(r2);

                // Recompute the cone limit rotation axis (r1 x -r2) for the constraint Jacobian.
                const glm::vec3 coneLimitAxis = glm::cross(r1Unit, -r2Unit);
                _basStore.SetConeLimitAxesCrossProductAtIndex(i, coneLimitAxis);

                // Check whether the cone limit is violated: positive error means within the limit, negative means exceeded.
                const f32 coneAngle = ComputeCurrentConeHalfAngle(r1Unit, -r2Unit);
                const f32 coneLimitError = _basStore.GetConeLimitHalfAngleAtIndex(i) - coneAngle;
                const bool coneLimitViolated = coneLimitError < 0;
                _basStore.SetConeLimitViolatedFlagAtIndex(i, coneLimitViolated);

                if (coneLimitViolated) {
                    // Compute the mass matrix K=JM^-1J^t for the cone limit (1x1), then its inverse.
                    const f32 kConeLimit = glm::dot(coneLimitAxis, invI1 * coneLimitAxis) + glm::dot(coneLimitAxis, invI2 * coneLimitAxis);
                    const f32 invKConeLimit = kConeLimit > f32(0.0) ? f32(1.0) / kConeLimit : f32(0.0);

                    _basStore.SetInverseMassMatrixConeLimitAtIndex(i, invKConeLimit);

                    // Compute the Lagrange multiplier lambda for the cone limit constraint.
                    const f32 lambdaConeLimit = invKConeLimit * (-coneLimitError);

                    // Compute the impulse P=J^T * lambda of body 1 and apply the pseudo-velocity to correct its orientation.
                    const glm::vec3 angularImpulseBody1 = lambdaConeLimit * coneLimitAxis;
                    const glm::vec3 w1 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (invI1 * angularImpulseBody1);
                    const glm::quat newQ1 = glm::normalize(q1 + (glm::quat(0, w1) * q1 * f32(0.5)));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, newQ1);

                    // Compute the impulse P=J^T * lambda of body 2 and apply the pseudo-velocity to correct its orientation.
                    const glm::vec3 angularImpulseBody2 = -lambdaConeLimit * coneLimitAxis;
                    const glm::vec3 w2 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (invI2 * angularImpulseBody2);
                    const glm::quat newQ2 = glm::normalize(q2 + (glm::quat(0, w2) * q2 * f32(0.5)));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, newQ2);
                }
            }

            // --------------- Translational Joint Constraint --------------- //

            // Compute the skew-symmetric matrices of r1 and r2 for use in the mass matrix K=JM^-1J^t.
            const glm::mat3 skewR1 = SkewSymmetric(r1);
            const glm::mat3 skewR2 = SkewSymmetric(r2);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Recompute the mass matrix K=JM^-1J^t for the 3 translational constraints.
            const f32 sumInvMass = invM1 + invM2;
            const glm::mat3 kTranslation = glm::mat3(sumInvMass) + skewR1 * invI1 * glm::transpose(skewR1) + skewR2 * invI2 * glm::transpose(skewR2);

            // Skip correction if the mass matrix is singular (degenerate configuration — no unique solution exists).
            const f32 kTranslationDet = glm::determinant(kTranslation);

            if (VE_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {

                    const glm::vec3 &x1 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
                    const glm::vec3 &x2 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);

                    // Compute the constraint error (value of the C(x) function)
                    const glm::vec3 errorTranslation = (x2 + r2 - x1 - r1);

                    // Solve K * lambda = -errorTranslation directly via Cramer's rule, without forming K^-1.
                    // invDet is computed once and reused as multiplications to avoid 3 expensive divisions.
                    const glm::vec3 rhs = -errorTranslation;
                    const f32 invDet = f32(1.0) / kTranslationDet;
                    const glm::vec3 lambdaTranslation(glm::determinant(glm::mat3(rhs, kTranslation[1], kTranslation[2])) * invDet,
                                                      glm::determinant(glm::mat3(kTranslation[0], rhs, kTranslation[2])) * invDet,
                                                      glm::determinant(glm::mat3(kTranslation[0], kTranslation[1], rhs)) * invDet);

                    // Compute the impulse P=J^T * lambda of body 1
                    const glm::vec3 linearImpulseBody1 = -lambdaTranslation;
                    const glm::vec3 angularImpulseBody1 = glm::cross(lambdaTranslation, r1);

                    // Compute the pseudo velocity of body 1 and update its position and orientation.
                    const glm::vec3 v1 = invM1 * _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex) * linearImpulseBody1;
                    const glm::vec3 w1 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex) * (invI1 * angularImpulseBody1);
                    const glm::quat newQ1 = glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5));

                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, newQ1);

                    // Compute the impulse P=J^T * lambda of body 2
                    const glm::vec3 angularImpulseBody2 = glm::cross(-lambdaTranslation, r2);

                    // Compute the pseudo velocity of body 2 and update its position and orientation.
                    const glm::vec3 v2 = invM2 * _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex) * lambdaTranslation;
                    const glm::vec3 w2 = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex) * (invI2 * angularImpulseBody2);
                    const glm::quat newQ2 = glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5));

                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, newQ2);
                }
            }
        }
    }

} // namespace Vulkyrie
