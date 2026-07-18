#include "physics/systems/fixed_joint_solver_system.h"
#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"
#include "core/asserts.h"
#include "core/utilities.h"

namespace Vulkyrie {

    FixedJointSolverSystem::FixedJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _fixedJointStore(world.GetFixedJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void FixedJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
        // For each fixed joint, precompute the solver state that stays constant across every velocity-solver
        // iteration of this step: the world-space lever arms, the inverse mass matrices K^-1 and the bias terms.
        const size_t activeJointCount = _fixedJointStore.GetActiveComponentCount();
        _jointIndices.resize(activeJointCount);

        for (size_t i = 0; i < activeJointCount; ++i) {
            const Entity jointEntity = _fixedJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            // Get the two bodies constrained by this joint and their rigid-body component indices.
            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            VASSERT(!_rigidBodyStore.EntityDisabled(bodyOneEntity) || !_rigidBodyStore.EntityDisabled(bodyTwoEntity), "Both bodies must be active.");

            // Resolve the entity-to-index hash lookups once per step; WarmStart and the solver phases run
            // every iteration and read the cached indices instead of repeating them.
            _jointIndices[i] = { jointIndex, bodyOneIndex, bodyTwoIndex };

            const bool baumgartePositionCorrection =
                JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex);

            // Cache the world-space inverse inertia tensors of both bodies for use in the mass matrices below.
            const glm::mat3 &invI1 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex);
            const glm::mat3 &invI2 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex);
            _fixedJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _fixedJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            const glm::quat &q1 = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &q2 = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _fixedJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _fixedJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the vector from each body's centre of mass to the anchor point, in world space (the lever arms r1, r2).
            const glm::vec3 r1 = q1 * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 r2 = q2 * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _fixedJointStore.SetR1WorldAtIndex(i, r1);
            _fixedJointStore.SetR2WorldAtIndex(i, r2);

            // Compute the corresponding skew-symmetric matrices so that a cross product r x v becomes the matrix product [r]x * v.
            const glm::mat3 skewR1 = SkewSymmetric(r1);
            const glm::mat3 skewR2 = SkewSymmetric(r2);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            // Compute the mass matrix K=JM^-1J^t (3x3) for the 3 translation constraints.
            const glm::mat3 kTranslation = glm::mat3(sumInvMass) + skewR1 * invI1 * glm::transpose(skewR1) + skewR2 * invI2 * glm::transpose(skewR2);

            // Compute the inverse translation mass matrix K^-1, leaving it zeroed for a singular or fully non-dynamic body pair.
            _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat3(0));
            const f32 kTranslationDet = glm::determinant(kTranslation);

            if (VE_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat3(kTranslation, kTranslationDet));
                }
            }

            // Get the world-space centres of mass, used to measure the current positional drift between the anchor points.
            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the Baumgarte bias "b" for the 3 translation constraints from the current anchor-point separation.
            if (baumgartePositionCorrection) {
                _fixedJointStore.SetTranslationBiasAtIndex(i, biasFactor * (x2 + r2 - x1 - r1));
            } else {
                _fixedJointStore.SetTranslationBiasAtIndex(i, glm::vec3(0));
            }

            // Compute the mass matrix K=JM^-1J^t (3x3) for the 3 rotation constraints, then its inverse K^-1,
            // leaving it zeroed for a singular or fully non-dynamic body pair (same guards as above).
            const glm::mat3 kRotation = invI1 + invI2;
            _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, glm::mat3(0));

            const f32 kRotationDet = glm::determinant(kRotation);
            if (VE_MACHINE_EPSILON < std::abs(kRotationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat3(kRotation, kRotationDet));
                }
            }

            // Compute the Baumgarte bias "b" for the 3 rotation constraints from the current orientation error.
            if (baumgartePositionCorrection) {
                // qError is the drift away from the rest orientation difference q0: qError = q2 * q0^-1 * q1^-1.
                // For a small error, 2 * qError.xyz approximates the rotation error vector (axis scaled by angle).
                const glm::quat &initialOrientationDifferenceInverse = _fixedJointStore.GetInitialOrientationDifferenceInverseAtIndex(i);
                const glm::quat qError = q2 * initialOrientationDifferenceInverse * glm::inverse(q1);

                _fixedJointStore.SetRotationBiasAtIndex(i, biasFactor * f32(2.0) * glm::vec3(qError.x, qError.y, qError.z));
            } else {
                _fixedJointStore.SetRotationBiasAtIndex(i, glm::vec3(0));
            }

            // When warm-starting is disabled, discard the impulses accumulated during the previous step.
            if (!_enableWarmStartup) {
                _fixedJointStore.SetImpulseTranslationAtIndex(i, glm::vec3(0));
                _fixedJointStore.SetImpulseRotationAtIndex(i, glm::vec3(0));
            }
        }
    }

    void FixedJointSolverSystem::WarmStart() {
        // Re-apply the impulses accumulated in the previous step as an initial guess, so the velocity solver
        // starts close to the solution and converges in fewer iterations. Body 1 receives -P and body 2 receives +P.
        VASSERT(_jointIndices.size() == _fixedJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before WarmStart().");

        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            // Get the inverse mass of the bodies.
            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Get the accumulated translation and rotation impulses to re-apply.
            const glm::vec3 &impulseTranslation = _fixedJointStore.GetImpulseTranslationAtIndex(i);
            const glm::vec3 &impulseRotation = _fixedJointStore.GetImpulseRotationAtIndex(i);

            const glm::vec3 &r1 = _fixedJointStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2 = _fixedJointStore.GetR2WorldAtIndex(i);

            const glm::mat3 &invI1 = _fixedJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _fixedJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 1
            glm::vec3 linearImpulseBody1 = -impulseTranslation;
            glm::vec3 angularImpulseBody1 = glm::cross(impulseTranslation, r1);

            // Compute the impulse P=J^T * lambda for the 3 rotation constraints for body 1
            angularImpulseBody1 += -impulseRotation;

            // Apply the impulse to the body 1
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            const glm::vec3 newW1 = w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 2
            glm::vec3 angularImpulseBody2 = glm::cross(-impulseTranslation, r2);

            // Compute the impulse P=J^T * lambda for the 3 rotation constraints for body 2
            angularImpulseBody2 += impulseRotation;

            // Apply the impulse to the body 2
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * impulseTranslation;
            const glm::vec3 newW2 = w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
        }
    }

    void FixedJointSolverSystem::SolveVelocityConstraint() {
        // For each fixed joint, apply sequential impulses that drive the relative velocity at the anchor to zero.
        // The 3 translation constraints are solved first, then the 3 rotation constraints reuse the resulting
        // velocities (Gauss-Seidel), so the joint converges over the solver's iterations.
        VASSERT(_jointIndices.size() == _fixedJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolveVelocityConstraint().");

        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::vec3 &r1 = _fixedJointStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2 = _fixedJointStore.GetR2WorldAtIndex(i);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            const glm::mat3 &invI1 = _fixedJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _fixedJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // --------------- Translation Constraints --------------- //

            // Compute J*v for the 3 translation constraints (the relative velocity of the two anchor points).
            const glm::vec3 JvTranslation = v2 + glm::cross(w2, r2) - v1 - glm::cross(w1, r1);

            const glm::mat3 &invKTranslation = _fixedJointStore.GetInverseMassTranslationMatrixAtIndex(i);

            // Compute the Lagrange multiplier lambda and accumulate it into the total translation impulse.
            const glm::vec3 deltaLambdaTranslation = invKTranslation * (-JvTranslation - _fixedJointStore.GetTranslationBiasAtIndex(i));
            _fixedJointStore.SetImpulseTranslationAtIndex(i, _fixedJointStore.GetImpulseTranslationAtIndex(i) + deltaLambdaTranslation);

            // Compute the impulse P=J^T * lambda for body 1.
            const glm::vec3 linearImpulseBody1 = -deltaLambdaTranslation;
            glm::vec3 angularImpulseBody1 = glm::cross(deltaLambdaTranslation, r1);

            // Apply the impulse to body 1. Only the linear velocity is committed here; the angular velocity is kept
            // in a local and committed once after the rotation stage adds its contribution.
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            const glm::vec3 newW1 = w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);

            // Compute the impulse P=J^T * lambda for body 2 (its linear impulse is +deltaLambdaTranslation, applied below).
            const glm::vec3 angularImpulseBody2 = -glm::cross(deltaLambdaTranslation, r2);

            // Apply the impulse to body 2 (linear velocity now; angular velocity committed after the rotation stage).
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * deltaLambdaTranslation;
            const glm::vec3 newW2 = w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);

            // --------------- Rotation Constraints --------------- //

            // Compute J*v for the 3 rotation constraints (the relative angular velocity), using the post-translation velocities.
            const glm::vec3 JvRotation = newW2 - newW1;

            const glm::vec3 &bRotation = _fixedJointStore.GetRotationBiasAtIndex(i);
            const glm::mat3 &invKRotation = _fixedJointStore.GetInverseMassRotationMatrixAtIndex(i);

            // Compute the Lagrange multiplier lambda for the 3 rotation constraints and accumulate the total rotation impulse.
            glm::vec3 deltaLambdaRotation = invKRotation * (-JvRotation - bRotation);
            _fixedJointStore.SetImpulseRotationAtIndex(i, _fixedJointStore.GetImpulseRotationAtIndex(i) + deltaLambdaRotation);

            // Compute the impulse P=J^T * lambda for the 3 rotation constraints for body 1 (body 2 uses +deltaLambdaRotation).
            angularImpulseBody1 = -deltaLambdaRotation;

            // Add the rotation-stage impulse on top of the translation-stage angular velocities...
            const glm::vec3 finalW1 = newW1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);

            const glm::vec3 finalW2 = newW2 + angularLockAxisFactorBodyTwo * (invI2 * deltaLambdaRotation);

            // ...and commit each body's final angular velocity once.
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, finalW1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, finalW2);
        }
    }

    void FixedJointSolverSystem::SolvePositionConstraint() {
        // Non-linear Gauss-Seidel position correction: for each fixed joint, directly move the two bodies'
        // positions and orientations to remove the anchor-point separation and orientation drift that remains
        // after the velocity solver. Because the geometry changes as the bodies move, the lever arms and mass
        // matrices are recomputed here from the current state rather than reusing the cached solver values.
        VASSERT(_jointIndices.size() == _fixedJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolvePositionConstraint().");

        for (size_t i = 0; i < _fixedJointStore.GetActiveComponentCount(); ++i) {
            const size_t jointIndex = _jointIndices[i].JointIndex;

            // This solver only runs for joints configured to use non-linear Gauss-Seidel position correction.
            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            // Get the constrained (in-progress) orientations; this solver reads and updates them in place.
            const glm::quat &q1 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &q2 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            // Get the (inverse) local-space inertia tensors, used to rebuild the world-space inertia tensors below.
            const glm::vec3 &invI1Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &invI2Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            // Per-axis factors that zero out the correction on locked/frozen rotational degrees of freedom.
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            // Recompute the world-space inverse inertia tensors from the current orientations, which have changed
            // since InitializeBeforeSolving as earlier position-correction iterations rotated the bodies.
            glm::mat3 invI1;
            glm::mat3 invI2;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q1), invI1Local, invI1);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q2), invI2Local, invI2);

            _fixedJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _fixedJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            // Recompute the world-space lever arms (centre of mass -> anchor point) from the current orientations.
            const glm::vec3 r1 =
                q1 * (_fixedJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex));
            const glm::vec3 r2 =
                q2 * (_fixedJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex));

            _fixedJointStore.SetR1WorldAtIndex(i, r1);
            _fixedJointStore.SetR2WorldAtIndex(i, r2);

            // Skew-symmetric matrices so that a cross product r x v becomes the matrix product [r]x * v.
            const glm::mat3 skewR1 = SkewSymmetric(r1);
            const glm::mat3 skewR2 = SkewSymmetric(r2);

            // --------------- Translation Constraints --------------- //

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            // Compute the mass matrix K=JM^-1J^t (3x3) for the 3 translation constraints.
            const glm::mat3 kTranslation = glm::mat3(sumInvMass) + skewR1 * invI1 * glm::transpose(skewR1) + skewR2 * invI2 * glm::transpose(skewR2);

            // Compute the inverse translation mass matrix K^-1, leaving it zeroed for a singular or fully non-dynamic body pair.
            glm::mat3 invKTranslation(0);
            _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, invKTranslation);

            const f32 kTranslationDet = glm::determinant(kTranslation);

            if (VE_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    invKTranslation = InverseMat3(kTranslation, kTranslationDet);
                    _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, invKTranslation);
                }

                // Measure the current positional drift between the two anchor points (the constraint error C).
                const glm::vec3 &x1 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
                const glm::vec3 &x2 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);

                const glm::vec3 errorTranslation = x2 + r2 - x1 - r1;

                // Solve K * lambda = -C for the position Lagrange multiplier that removes the drift.
                const glm::vec3 lambdaTranslation = invKTranslation * (-errorTranslation);

                // Compute the impulse P=J^T * lambda for body 1.
                glm::vec3 linearImpulseBody1 = -lambdaTranslation;
                glm::vec3 angularImpulseBody1 = glm::cross(lambdaTranslation, r1);

                // Turn the impulse into a pseudo velocity (masked by the locked axes) and integrate body 1's
                // position and orientation directly. The orientation uses the quaternion derivative q += 0.5 * (0,w) * q.
                const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
                const glm::vec3 v1 = invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);

                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                // Compute the impulse P=J^T * lambda for body 2 (its linear impulse is +lambdaTranslation).
                glm::vec3 angularImpulseBody2 = -glm::cross(lambdaTranslation, r2);

                // Turn the impulse into a pseudo velocity (masked by the locked axes) and integrate body 2's position/orientation.
                const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
                const glm::vec3 v2 = invM2 * linearLockAxisFactorBodyTwo * lambdaTranslation;
                glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);

                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
            }

            // --------------- Rotation Constraints --------------- //

            // Build the mass matrix K=JM^-1J^t (3x3) for the 3 rotation constraints. The relative-rotation Jacobian
            // is [-1, 1] on the two bodies' angular parts, so K reduces to the sum of their world-space inverse
            // inertia tensors. Invert it in place, leaving it zeroed for a singular or fully non-dynamic body pair.
            const glm::mat3 kRotation = invI1 + invI2;
            glm::mat3 invKRotation(0);
            _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, invKRotation);
            const f32 kRotationDet = glm::determinant(kRotation);

            if (VE_MACHINE_EPSILON < std::abs(kRotationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    invKRotation = InverseMat3(kRotation, kRotationDet);
                    _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, invKRotation);
                }

                // Measure the orientation drift away from the joint's rest orientation difference q0. A drift-free
                // joint holds the bodies at that rest difference, i.e. q2 = q1 * q0;
                // any actual drift shows up as the error quaternion qError in q2 = qError * q1 * q0.
                // Solving for it gives qError = q2 * q0^-1 * q1^-1, and qError is
                // identity when the joint is satisfied. GetInitialOrientationDifferenceInverse() supplies q0^-1.
                glm::quat qError = q2 * _fixedJointStore.GetInitialOrientationDifferenceInverseAtIndex(i) * glm::inverse(q1);

                // Convert the error quaternion into a rotation-error vector (rotation axis scaled by angle). A quaternion is
                // q = [sin(theta/2) * axis, cos(theta/2)], and for a small error sin(theta/2) ~= theta/2, so twice the
                // vector part approximates theta * axis, i.e. the error we need to drive to zero.
                const glm::vec3 errorRotation = f32(2.0) * glm::vec3(qError.x, qError.y, qError.z);

                // Solve K * lambda = -errorRotation for the rotation Lagrange multiplier that removes the drift.
                const glm::vec3 lambdaRotation = invKRotation * (-errorRotation);

                // Compute the impulse P=J^T * lambda for body 1 (its rotation Jacobian block is -1, hence the negation).
                glm::vec3 angularImpulseBody1 = -lambdaRotation;

                // Turn the impulse into a pseudo angular velocity (masked by the locked axes) and integrate body 1's
                // orientation directly via the quaternion derivative q += 0.5 * (0,w) * q.
                glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);

                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                // Body 2's rotation Jacobian block is +1, so its impulse is +lambdaRotation; integrate its orientation the same way.
                glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * lambdaRotation);

                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
            }
        }
    }

} // namespace Vulkyrie
