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

            VASSERT(!_rigidBodyStore.IsDisabled(bodyOneEntity) || !_rigidBodyStore.IsDisabled(bodyTwoEntity), "Both bodies must be active.");

            // Resolve the entity-to-index hash lookups once per step; WarmStart and the solver phases run
            // every iteration and read the cached indices instead of repeating them.
            _jointIndices[i] = { jointIndex, bodyOneIndex, bodyTwoIndex };

            const bool baumgartePositionCorrection =
                JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex);

            // Cache the world-space inverse inertia tensors of both bodies for use in the mass matrices below.
            const glm::mat3 &inertiaTensorBodyOne = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex);
            const glm::mat3 &inertiaTensorBodyTwo = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex);
            _fixedJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, inertiaTensorBodyOne);
            _fixedJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, inertiaTensorBodyTwo);

            const glm::quat &orientationBodyOne = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &orientationBodyTwo = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _fixedJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _fixedJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the vector from each body's centre of mass to the anchor point, in world space (the lever arms r1, r2).
            const glm::vec3 rOneWorld = orientationBodyOne * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 rTwoWorld = orientationBodyTwo * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _fixedJointStore.SetR1WorldAtIndex(i, rOneWorld);
            _fixedJointStore.SetR2WorldAtIndex(i, rTwoWorld);

            // Compute the corresponding skew-symmetric matrices so that a cross product r x v becomes the matrix product [r]x * v.
            const glm::mat3 skewSymmetricMatrixU1 = SkewSymmetric(rOneWorld);
            const glm::mat3 skewSymmetricMatrixU2 = SkewSymmetric(rTwoWorld);

            const f32 bodyOneInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 bodyTwoInverseMass = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 totalInverseMass = bodyOneInverseMass + bodyTwoInverseMass;

            // Compute the mass matrix K=JM^-1J^t (3x3) for the 3 translation constraints.
            const glm::mat3 massMatrix = glm::mat3(glm::vec3(totalInverseMass, 0, 0), //
                                                   glm::vec3(0, totalInverseMass, 0), //
                                                   glm::vec3(0, 0, totalInverseMass)  //
                                                   ) +
                                         skewSymmetricMatrixU1 * inertiaTensorBodyOne * glm::transpose(skewSymmetricMatrixU1) +
                                         skewSymmetricMatrixU2 * inertiaTensorBodyTwo * glm::transpose(skewSymmetricMatrixU2);

            // Compute the inverse translation mass matrix K^-1, leaving it zeroed for a singular or fully non-dynamic body pair.
            _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat3(0));
            const f32 massMatrixDeterminant = glm::determinant(massMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat3(massMatrix, massMatrixDeterminant));
                }
            }

            // Get the world-space centres of mass, used to measure the current positional drift between the anchor points.
            const glm::vec3 &centerOfMassBodyOne = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &centerOfMassBodyTwo = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the Baumgarte bias "b" for the 3 translation constraints from the current anchor-point separation.
            if (baumgartePositionCorrection) {
                _fixedJointStore.SetTranslationBiasAtIndex(i, biasFactor * (centerOfMassBodyTwo + rTwoWorld - centerOfMassBodyOne - rOneWorld));
            } else {
                _fixedJointStore.SetTranslationBiasAtIndex(i, glm::vec3(0));
            }

            // Compute the mass matrix K=JM^-1J^t (3x3) for the 3 rotation constraints, then its inverse K^-1,
            // leaving it zeroed for a singular or fully non-dynamic body pair (same guards as above).
            const glm::mat3 massMatrixRotation = inertiaTensorBodyOne + inertiaTensorBodyTwo;
            _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, glm::mat3(0));

            const f32 massMatrixRotationDeterminant = glm::determinant(massMatrixRotation);
            if (VE_MACHINE_EPSILON < std::abs(massMatrixRotationDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat3(massMatrixRotation, massMatrixRotationDeterminant));
                }
            }

            // Compute the Baumgarte bias "b" for the 3 rotation constraints from the current orientation error.
            if (baumgartePositionCorrection) {
                // qError is the drift away from the rest orientation difference q0: qError = q2 * q0^-1 * q1^-1.
                // For a small error, 2 * qError.xyz approximates the rotation error vector (axis scaled by angle).
                const glm::quat &initialOrientationDifferenceInverse = _fixedJointStore.GetInitialOrientationDifferenceInverseAtIndex(i);
                const glm::quat qError = orientationBodyTwo * initialOrientationDifferenceInverse * glm::inverse(orientationBodyOne);

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
            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Get the accumulated translation and rotation impulses to re-apply.
            const glm::vec3 &impulseTranslation = _fixedJointStore.GetImpulseTranslationAtIndex(i);
            const glm::vec3 &impulseRotation = _fixedJointStore.GetImpulseRotationAtIndex(i);

            const glm::vec3 &rOneWorld = _fixedJointStore.GetR1WorldAtIndex(i);
            const glm::vec3 &rTwoWorld = _fixedJointStore.GetR2WorldAtIndex(i);

            const glm::mat3 &inertiaTensorBodyOne = _fixedJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _fixedJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &linearVelocityBodyOne = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &linearVelocityBodyTwo = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularVelocityBodyOne = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &angularVelocityBodyTwo = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 1
            glm::vec3 linearImpulseBodyOne = -impulseTranslation;
            glm::vec3 angularImpulseBodyOne = glm::cross(impulseTranslation, rOneWorld);

            // Compute the impulse P=J^T * lambda for the 3 rotation constraints for body 1
            angularImpulseBodyOne += -impulseRotation;

            // Apply the impulse to the body 1
            const glm::vec3 &linearlockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newLinearVelocityBodyOne = linearVelocityBodyOne + inverseMassBodyOne * linearlockAxisFactorBodyOne * linearImpulseBodyOne;
            const glm::vec3 newAngularVelocityBodyOne = angularVelocityBodyOne + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newLinearVelocityBodyOne);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newAngularVelocityBodyOne);

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 2
            glm::vec3 angularImpulseBodyTwo = glm::cross(-impulseTranslation, rTwoWorld);

            // Compute the impulse P=J^T * lambda for the 3 rotation constraints for body 2
            angularImpulseBodyTwo += impulseRotation;

            // Apply the impulse to the body 2
            const glm::vec3 &linearlockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newLinearVelocityBodyTwo = linearVelocityBodyTwo + inverseMassBodyTwo * linearlockAxisFactorBodyTwo * impulseTranslation;
            const glm::vec3 newAngularVelocityBodyTwo = angularVelocityBodyTwo + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBodyTwo);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newLinearVelocityBodyTwo);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newAngularVelocityBodyTwo);
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

            const glm::vec3 &rOneWorld = _fixedJointStore.GetR1WorldAtIndex(i);
            const glm::vec3 &rTwoWorld = _fixedJointStore.GetR2WorldAtIndex(i);

            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            const glm::mat3 &inertiaTensorBodyOne = _fixedJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _fixedJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &linearVelocityBodyOne = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &linearVelocityBodyTwo = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularVelocityBodyOne = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &angularVelocityBodyTwo = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // --------------- Translation Constraints --------------- //

            // Compute J*v for the 3 translation constraints (the relative velocity of the two anchor points).
            const glm::vec3 JvTranslation =
                linearVelocityBodyTwo + glm::cross(angularVelocityBodyTwo, rTwoWorld) - linearVelocityBodyOne - glm::cross(angularVelocityBodyOne, rOneWorld);

            const glm::mat3 &inverseMassMatrixTranslation = _fixedJointStore.GetInverseMassTranslationMatrixAtIndex(i);

            // Compute the Lagrange multiplier lambda and accumulate it into the total translation impulse.
            const glm::vec3 deltaLambda = inverseMassMatrixTranslation * (-JvTranslation - _fixedJointStore.GetTranslationBiasAtIndex(i));
            _fixedJointStore.SetImpulseTranslationAtIndex(i, _fixedJointStore.GetImpulseTranslationAtIndex(i) + deltaLambda);

            // Compute the impulse P=J^T * lambda for body 1.
            const glm::vec3 linearImpulseBodyOne = -deltaLambda;
            glm::vec3 angularImpulseBodyOne = glm::cross(deltaLambda, rOneWorld);

            // Apply the impulse to body 1. Only the linear velocity is committed here; the angular velocity is kept
            // in a local and committed once after the rotation stage adds its contribution.
            const glm::vec3 &linearlockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newLinearVelocityBodyOne = linearVelocityBodyOne + inverseMassBodyOne * linearlockAxisFactorBodyOne * linearImpulseBodyOne;
            const glm::vec3 newAngularVelocityBodyOne = angularVelocityBodyOne + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newLinearVelocityBodyOne);

            // Compute the impulse P=J^T * lambda for body 2 (its linear impulse is +deltaLambda, applied below).
            const glm::vec3 angularImpulseBodyTwo = -glm::cross(deltaLambda, rTwoWorld);

            // Apply the impulse to body 2 (linear velocity now; angular velocity committed after the rotation stage).
            const glm::vec3 &linearlockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newLinearVelocityBodyTwo = linearVelocityBodyTwo + inverseMassBodyTwo * linearlockAxisFactorBodyTwo * deltaLambda;
            const glm::vec3 newAngularVelocityBodyTwo = angularVelocityBodyTwo + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBodyTwo);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newLinearVelocityBodyTwo);

            // --------------- Rotation Constraints --------------- //

            // Compute J*v for the 3 rotation constraints (the relative angular velocity), using the post-translation velocities.
            const glm::vec3 JvRotation = newAngularVelocityBodyTwo - newAngularVelocityBodyOne;

            const glm::vec3 &biasRotation = _fixedJointStore.GetRotationBiasAtIndex(i);
            const glm::mat3 &inverseMassMatrixRotation = _fixedJointStore.GetInverseMassRotationMatrixAtIndex(i);

            // Compute the Lagrange multiplier lambda for the 3 rotation constraints and accumulate the total rotation impulse.
            glm::vec3 deltaLambdaTwo = inverseMassMatrixRotation * (-JvRotation - biasRotation);
            _fixedJointStore.SetImpulseRotationAtIndex(i, _fixedJointStore.GetImpulseRotationAtIndex(i) + deltaLambdaTwo);

            // Compute the impulse P=J^T * lambda for the 3 rotation constraints for body 1 (body 2 uses +deltaLambdaTwo).
            angularImpulseBodyOne = -deltaLambdaTwo;

            // Add the rotation-stage impulse on top of the translation-stage angular velocities...
            const glm::vec3 updatedAngularVelocityBodyOne =
                newAngularVelocityBodyOne + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne);

            const glm::vec3 updatedAngularVelocityBodyTwo = newAngularVelocityBodyTwo + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * deltaLambdaTwo);

            // ...and commit each body's final angular velocity once.
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, updatedAngularVelocityBodyOne);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, updatedAngularVelocityBodyTwo);
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
            const glm::quat &orientationBodyOne = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &orientationBodyTwo = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            // Get the (inverse) local-space inertia tensors, used to rebuild the world-space inertia tensors below.
            const glm::vec3 &bodyOneLocalInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &bodyTwoLocalInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            // Per-axis factors that zero out the correction on locked/frozen rotational degrees of freedom.
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            // Recompute the world-space inverse inertia tensors from the current orientations, which have changed
            // since InitializeBeforeSolving as earlier position-correction iterations rotated the bodies.
            glm::mat3 bodyOneWorldInertiaTensor;
            glm::mat3 bodyTwoWorldInertiaTensor;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(orientationBodyOne), bodyOneLocalInertiaTensor, bodyOneWorldInertiaTensor);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(orientationBodyTwo), bodyTwoLocalInertiaTensor, bodyTwoWorldInertiaTensor);

            _fixedJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, bodyOneWorldInertiaTensor);
            _fixedJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, bodyTwoWorldInertiaTensor);

            // Recompute the world-space lever arms (centre of mass -> anchor point) from the current orientations.
            const glm::vec3 rOneWorld =
                orientationBodyOne * (_fixedJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex));
            const glm::vec3 rTwoWorld =
                orientationBodyTwo * (_fixedJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex));

            _fixedJointStore.SetR1WorldAtIndex(i, rOneWorld);
            _fixedJointStore.SetR2WorldAtIndex(i, rTwoWorld);

            // Skew-symmetric matrices so that a cross product r x v becomes the matrix product [r]x * v.
            const glm::mat3 skewSymmetricMatrixU1 = SkewSymmetric(rOneWorld);
            const glm::mat3 skewSymmetricMatrixU2 = SkewSymmetric(rTwoWorld);

            // --------------- Translation Constraints --------------- //

            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 totalInverseMass = inverseMassBodyOne + inverseMassBodyTwo;

            // Compute the mass matrix K=JM^-1J^t (3x3) for the 3 translation constraints.
            const glm::mat3 massMatrix = glm::mat3(glm::vec3(totalInverseMass, 0, 0), //
                                                   glm::vec3(0, totalInverseMass, 0), //
                                                   glm::vec3(0, 0, totalInverseMass)  //
                                                   ) +
                                         skewSymmetricMatrixU1 * bodyOneWorldInertiaTensor * glm::transpose(skewSymmetricMatrixU1) +
                                         skewSymmetricMatrixU2 * bodyTwoWorldInertiaTensor * glm::transpose(skewSymmetricMatrixU2);

            // Compute the inverse translation mass matrix K^-1, leaving it zeroed for a singular or fully non-dynamic body pair.
            glm::mat3 inverseMassMatrixTranslation(0);
            _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, inverseMassMatrixTranslation);

            const f32 massMatrixDeterminant = glm::determinant(massMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    inverseMassMatrixTranslation = InverseMat3(massMatrix, massMatrixDeterminant);
                    _fixedJointStore.SetInverseMassTranslationMatrixAtIndex(i, inverseMassMatrixTranslation);
                }

                // Measure the current positional drift between the two anchor points (the constraint error C).
                const glm::vec3 &positionBodyOne = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
                const glm::vec3 &positionBodyTwo = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);

                const glm::vec3 errorTranslation = positionBodyTwo + rTwoWorld - positionBodyOne - rOneWorld;

                // Solve K * lambda = -C for the position Lagrange multiplier that removes the drift.
                const glm::vec3 lambdaTranslation = inverseMassMatrixTranslation * (-errorTranslation);

                // Compute the impulse P=J^T * lambda for body 1.
                glm::vec3 linearImpulseBodyOne = -lambdaTranslation;
                glm::vec3 angularImpulseBodyOne = glm::cross(lambdaTranslation, rOneWorld);

                // Turn the impulse into a pseudo velocity (masked by the locked axes) and integrate body 1's
                // position and orientation directly. The orientation uses the quaternion derivative q += 0.5 * (0,w) * q.
                const glm::vec3 &linearlockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
                const glm::vec3 pseudoLinearVelocityBodyOne = inverseMassBodyOne * linearlockAxisFactorBodyOne * linearImpulseBodyOne;
                glm::vec3 pseudoAngularVelocityBodyOne = angularLockAxisFactorBodyOne * (bodyOneWorldInertiaTensor * angularImpulseBodyOne);

                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, positionBodyOne + pseudoLinearVelocityBodyOne);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(
                    bodyOneIndex, glm::normalize(orientationBodyOne + glm::quat(0, pseudoAngularVelocityBodyOne) * orientationBodyOne * f32(0.5)));

                // Compute the impulse P=J^T * lambda for body 2 (its linear impulse is +lambdaTranslation).
                glm::vec3 angularImpulseBodyTwo = -glm::cross(lambdaTranslation, rTwoWorld);

                // Turn the impulse into a pseudo velocity (masked by the locked axes) and integrate body 2's position/orientation.
                const glm::vec3 &linearlockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
                const glm::vec3 pseudoLinearVelocityBodyTwo = inverseMassBodyTwo * linearlockAxisFactorBodyTwo * lambdaTranslation;
                glm::vec3 pseudoAngularVelocityBodyTwo = angularLockAxisFactorBodyTwo * (bodyTwoWorldInertiaTensor * angularImpulseBodyTwo);

                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, positionBodyTwo + pseudoLinearVelocityBodyTwo);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(
                    bodyTwoIndex, glm::normalize(orientationBodyTwo + glm::quat(0, pseudoAngularVelocityBodyTwo) * orientationBodyTwo * f32(0.5)));
            }

            // --------------- Rotation Constraints --------------- //

            // Build the mass matrix K=JM^-1J^t (3x3) for the 3 rotation constraints. The relative-rotation Jacobian
            // is [-1, 1] on the two bodies' angular parts, so K reduces to the sum of their world-space inverse
            // inertia tensors. Invert it in place, leaving it zeroed for a singular or fully non-dynamic body pair.
            const glm::mat3 massMatrixRotation = bodyOneWorldInertiaTensor + bodyTwoWorldInertiaTensor;
            glm::mat3 inverseMassMatrixRotation(0);
            _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, inverseMassMatrixRotation);
            const f32 massMatrixRotationDeterminant = glm::determinant(massMatrixRotation);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixRotationDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    inverseMassMatrixRotation = InverseMat3(massMatrixRotation, massMatrixRotationDeterminant);
                    _fixedJointStore.SetInverseMassRotationMatrixAtIndex(i, inverseMassMatrixRotation);
                }

                // Measure the orientation drift away from the joint's rest orientation difference q0. A drift-free
                // joint holds the bodies at that rest difference, i.e. orientationBodyTwo = orientationBodyOne * q0;
                // any actual drift shows up as the error quaternion qError in orientationBodyTwo = qError * orientationBodyOne * q0.
                // Solving for it gives qError = orientationBodyTwo * q0^-1 * orientationBodyOne^-1, and qError is
                // identity when the joint is satisfied. GetInitialOrientationDifferenceInverse() supplies q0^-1.
                glm::quat qError = orientationBodyTwo * _fixedJointStore.GetInitialOrientationDifferenceInverseAtIndex(i) * glm::inverse(orientationBodyOne);

                // Convert the error quaternion into a rotation-error vector (rotation axis scaled by angle). A quaternion is
                // q = [sin(theta/2) * axis, cos(theta/2)], and for a small error sin(theta/2) ~= theta/2, so twice the
                // vector part approximates theta * axis, i.e. the error we need to drive to zero.
                const glm::vec3 errorRotation = f32(2.0) * glm::vec3(qError.x, qError.y, qError.z);

                // Solve K * lambda = -errorRotation for the rotation Lagrange multiplier that removes the drift.
                const glm::vec3 lambdaRotation = inverseMassMatrixRotation * (-errorRotation);

                // Compute the impulse P=J^T * lambda for body 1 (its rotation Jacobian block is -1, hence the negation).
                glm::vec3 angularImpulseBodyOne = -lambdaRotation;

                // Turn the impulse into a pseudo angular velocity (masked by the locked axes) and integrate body 1's
                // orientation directly via the quaternion derivative q += 0.5 * (0,w) * q.
                glm::vec3 pseudoAngularVelocityBodyOne = angularLockAxisFactorBodyOne * (bodyOneWorldInertiaTensor * angularImpulseBodyOne);

                _rigidBodyStore.SetConstrainedOrientationAtIndex(
                    bodyOneIndex, glm::normalize(orientationBodyOne + glm::quat(0, pseudoAngularVelocityBodyOne) * orientationBodyOne * f32(0.5)));

                // Body 2's rotation Jacobian block is +1, so its impulse is +lambdaRotation; integrate its orientation the same way.
                glm::vec3 pseudoAngularVelocityBodyTwo = angularLockAxisFactorBodyTwo * (bodyTwoWorldInertiaTensor * lambdaRotation);

                _rigidBodyStore.SetConstrainedOrientationAtIndex(
                    bodyTwoIndex, glm::normalize(orientationBodyTwo + glm::quat(0, pseudoAngularVelocityBodyTwo) * orientationBodyTwo * f32(0.5)));
            }
        }
    }

} // namespace Vulkyrie
