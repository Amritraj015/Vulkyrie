#include "physics/systems/slider_joint_solver_system.h"
#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"
#include "core/utilities.h"

namespace Vulkyrie {

    SliderJointSolverSystem::SliderJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _sliderJointStore(world.GetSliderJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void SliderJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
        // For each active slider joint, precompute the solver state that stays constant across every
        // velocity-solver iteration of this step: lever arms, mass matrices, and bias terms.
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _sliderJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            VASSERT(!_rigidBodyStore.IsDisabled(bodyOneEntity) || !_rigidBodyStore.IsDisabled(bodyTwoEntity), "Both bodies must be active.");

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            // Cache the world-space inverse inertia tensors of both bodies for use in the mass matrices below.
            _sliderJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex));
            _sliderJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex));

            const glm::quat &orientationBodyOne = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &orientationBodyTwo = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _sliderJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _sliderJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the lever arms (anchor point relative to center of mass) in world space.
            const glm::vec3 rOneWorld = orientationBodyOne * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 rTwoWorld = orientationBodyTwo * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _sliderJointStore.SetR1WorldAtIndex(i, rOneWorld);
            _sliderJointStore.SetR2WorldAtIndex(i, rTwoWorld);

            // Compute the slider axis in world space; n1 and n2 span the plane orthogonal to it and form the
            // basis the 2-DOF translation constraint's Jacobian (below) is built from.
            const glm::vec3 sliderAxisInWorldSpace = glm::normalize(orientationBodyOne * _sliderJointStore.GetSliderAxisInBodyOneLocalSpaceAtIndex(i));
            _sliderJointStore.SetSliderAxisInWorldSpaceAtIndex(i, sliderAxisInWorldSpace);

            const glm::vec3 n1 = GetOrthogonalUnitVector(sliderAxisInWorldSpace);
            const glm::vec3 n2 = glm::cross(sliderAxisInWorldSpace, n1);
            _sliderJointStore.SetN1AtIndex(i, n1);
            _sliderJointStore.SetN2AtIndex(i, n2);

            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            // u is the world-space separation between the anchor points (zero once the translation constraint
            // is satisfied). Body 1's angular Jacobian block for the n1/n2 constraints uses r1PlusU = r1 + u
            // rather than r1 alone, since sliding along the axis shifts body 1's effective lever arm relative
            // to body 2's anchor - this is the standard slider-joint derivation.
            const glm::vec3 u = x2 + rTwoWorld - x1 - rOneWorld;
            const glm::vec3 r1PlusU = rOneWorld + u;
            const glm::vec3 &r1PlusUCrossN1 = glm::cross(r1PlusU, n1);
            const glm::vec3 &r1PlusUCrossN2 = glm::cross(r1PlusU, n2);

            // Precomputed Jacobian lever-arm terms, reused below and in WarmStart/SolveVelocityConstraint.
            _sliderJointStore.SetR1PlusUCrossN1AtIndex(i, r1PlusUCrossN1);
            _sliderJointStore.SetR1PlusUCrossN2AtIndex(i, r1PlusUCrossN2);
            _sliderJointStore.SetR1PlusUCrossSliderAxisAtIndex(i, glm::cross(r1PlusU, sliderAxisInWorldSpace));

            // Determine whether the lower/upper limit constraints (translation along the slider axis) are
            // currently violated. A limit's accumulated impulse is reset whenever it stops being violated or
            // flips violation state, since a stale impulse from a different regime would bias the new solve.
            const f32 uDotSliderAxis = glm::dot(u, sliderAxisInWorldSpace);
            const f32 lowerLimitError = uDotSliderAxis - _sliderJointStore.GetLowerLimitAtIndex(i);
            const f32 upperLimitError = _sliderJointStore.GetUpperLimitAtIndex(i) - uDotSliderAxis;
            const bool oldIsLowerLimitViolated = _sliderJointStore.GetIsLowerLimitViolatedAtIndex(i);
            const bool lowerLimitViolated = lowerLimitError <= 0;
            _sliderJointStore.SetIsLowerLimitViolatedAtIndex(i, lowerLimitViolated);

            if (!lowerLimitViolated || lowerLimitViolated != oldIsLowerLimitViolated) {
                _sliderJointStore.SetImpulseLowerLimitAtIndex(i, f32(0.0));
            }

            const bool oldIsUpperLimitViolated = _sliderJointStore.GetIsUpperLimitViolatedAtIndex(i);
            const bool upperLimitViolated = upperLimitError <= 0;
            _sliderJointStore.SetIsUpperLimitViolatedAtIndex(i, upperLimitViolated);

            if (!upperLimitViolated || upperLimitViolated != oldIsUpperLimitViolated) {
                _sliderJointStore.SetImpulseUpperLimitAtIndex(i, f32(0.0));
            }

            // Compute the Baumgarte bias "b" for the 2 translation constraints from the current anchor-point
            // separation projected onto n1/n2 (zero once the anchor points are aligned along the slider axis).
            if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                const glm::vec2 newTranslationBias = glm::vec2(glm::dot(u, n1) * biasFactor, glm::dot(u, n2) * biasFactor);
                _sliderJointStore.SetTranslationBiasAtIndex(i, newTranslationBias);
            } else {
                _sliderJointStore.SetTranslationBiasAtIndex(i, glm::vec2(0));
            }

            const glm::mat3 &i1 = _sliderJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &i2 = _sliderJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInverseMass = inverseMassBodyOne + inverseMassBodyTwo;

            // The limit constraint shares its 1-DOF Jacobian (the slider axis) with the motor, but unlike the
            // motor its inverse mass matrix also has an angular component, so it's computed separately here.
            // Note: r2CrossSliderAxis is read below before it is (re)computed further down this same iteration,
            // so this uses the value from the previous call rather than the fresh one - that is intentional,
            // so treat it as expected behavior rather than a bug to silently fix.
            if (_sliderJointStore.IsLimitEnabledAtIndex(i) && (lowerLimitViolated || upperLimitViolated)) {
                const glm::vec3 &r2CrossSliderAxis = _sliderJointStore.GetR2CrossSliderAxisAtIndex(i);
                const glm::vec3 &r1PlusUCrossSliderAxis = _sliderJointStore.GetR1PlusUCrossSliderAxisAtIndex(i);

                const f32 temp =
                    sumInverseMass + glm::dot(r1PlusUCrossSliderAxis, i1 * r1PlusUCrossSliderAxis) + glm::dot(r2CrossSliderAxis, i2 * r2CrossSliderAxis);
                _sliderJointStore.SetInverseMassMatrixLimitAtIndex(i, temp > f32(0.0) ? f32(1.0) / temp : f32(0.0));

                // Compute the bias "b" of the lower limit constraint.
                if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                    _sliderJointStore.SetBiasLowerLimitAtIndex(i, biasFactor * lowerLimitError);
                } else {
                    _sliderJointStore.SetBiasLowerLimitAtIndex(i, f32(0.0));
                }

                // Compute the bias "b" of the upper limit constraint.
                if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                    _sliderJointStore.SetBiasUpperLimitAtIndex(i, biasFactor * upperLimitError);
                } else {
                    _sliderJointStore.SetBiasUpperLimitAtIndex(i, f32(0.0));
                }
            }

            // Compute the remaining Jacobian lever-arm terms for body 2, now that r2CrossSliderAxis (read
            // stale above) can safely be refreshed for the next call.
            const glm::vec3 &r2 = _sliderJointStore.GetR2WorldAtIndex(i);
            const glm::vec3 r2CrossN1 = glm::cross(r2, n1);
            const glm::vec3 r2CrossN2 = glm::cross(r2, n2);
            _sliderJointStore.SetR2CrossN1AtIndex(i, r2CrossN1);
            _sliderJointStore.SetR2CrossN2AtIndex(i, r2CrossN2);

            _sliderJointStore.SetR2CrossSliderAxisAtIndex(i, glm::cross(rTwoWorld, sliderAxisInWorldSpace));

            // Compute the mass matrix K = J*M^-1*J^t (2x2) for the 2 translation constraints, then its inverse.
            const glm::vec3 I1R1PlusUCrossN1 = i1 * r1PlusUCrossN1;
            const glm::vec3 I1R1PlusUCrossN2 = i1 * r1PlusUCrossN2;
            const glm::vec3 I2R2CrossN1 = i2 * r2CrossN1;
            const glm::vec3 I2R2CrossN2 = i2 * r2CrossN2;
            const f32 el11 = sumInverseMass + glm::dot(r1PlusUCrossN1, I1R1PlusUCrossN1) + glm::dot(r2CrossN1, I2R2CrossN1);
            const f32 el12 = glm::dot(r1PlusUCrossN1, I1R1PlusUCrossN2) + glm::dot(r2CrossN1, I2R2CrossN2);
            const f32 el21 = glm::dot(r1PlusUCrossN2, I1R1PlusUCrossN1) + glm::dot(r2CrossN2, I2R2CrossN1);
            const f32 el22 = sumInverseMass + glm::dot(r1PlusUCrossN2, I1R1PlusUCrossN2) + glm::dot(r2CrossN2, I2R2CrossN2);

            // glm::mat2's 4-scalar constructor is column-major (col0, col1), so the middle two
            // arguments are swapped from row-major reading order to build [[el11,el12],[el21,el22]].
            const glm::mat2 matrixKTranslation(el11, el21, el12, el22);

            _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat2(0));
            const f32 matrixKTranslationDeterminant = glm::determinant(matrixKTranslation);

            // Skip the inverse (leave it zeroed) if the mass matrix is singular or both bodies are non-dynamic;
            // a singular K means the constraint is degenerate and has no unique impulse solution this step.
            if (VE_MACHINE_EPSILON < std::abs(matrixKTranslationDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat2(matrixKTranslation, matrixKTranslationDeterminant));
                }
            }

            // Compute the mass matrix K = I1 + I2 (3x3) for the 3 rotation constraints, then its inverse.
            // Note: unlike the translation matrix above, the store is not zeroed before this - if K is
            // singular or both bodies are non-dynamic, the slot is left holding the raw (uninverted) K matrix
            // rather than zero. SolveVelocityConstraint should account for this explicitly rather than assume
            // a zero fallback.
            const glm::mat3 inverseMassRotationMatrix = i1 + i2;
            _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, inverseMassRotationMatrix);
            f32 massMatrixRotationDeterminant = glm::determinant(inverseMassRotationMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixRotationDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat3(inverseMassRotationMatrix, massMatrixRotationDeterminant));
                }
            }

            // Compute the Baumgarte bias "b" for the 3 rotation constraints from the quaternion error between
            // the current and initial relative orientations (2x the error quaternion's vector part is the
            // small-angle approximation of the orientation drift).
            if (JointsPositionCorrectionTechnique::BaumgarteJoints == _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                const glm::quat qError =
                    orientationBodyTwo * _sliderJointStore.GetInitialOrientationDifferenceInverseAtIndex(i) * glm::inverse(orientationBodyOne);
                _sliderJointStore.SetRotationBiasAtIndex(i, biasFactor * f32(2.0) * glm::vec3(qError.x, qError.y, qError.z));
            } else {
                _sliderJointStore.SetRotationBiasAtIndex(i, glm::vec3(0));
            }

            // Compute the inverse of the 1x1 mass matrix K = M1^-1 + M2^-1 for the motor constraint. Motion
            // along the slider axis is a pure translation, so unlike the hinge/limit motors this has no
            // angular (inertia tensor) component.
            if (_sliderJointStore.IsMotorEnabledAtIndex(i)) {
                _sliderJointStore.SetInverseMassMatrixMotorAtIndex(i, sumInverseMass > f32(0.0) ? f32(1.0) / sumInverseMass : f32(0.0));
            }

            // If warm-starting is disabled, discard every impulse accumulated last step so this step's
            // velocity solver starts from zero instead of re-applying stale impulses.
            if (!_enableWarmStartup) {
                _sliderJointStore.SetImpulseTranslationAtIndex(i, glm::vec2(0.0));
                _sliderJointStore.SetImpulseRotationAtIndex(i, glm::vec3(0.0));
                _sliderJointStore.SetImpulseLowerLimitAtIndex(i, f32(0.0));
                _sliderJointStore.SetImpulseUpperLimitAtIndex(i, f32(0.0));
                _sliderJointStore.SetImpulseMotorAtIndex(i, f32(0.0));
            }
        }
    }

    void SliderJointSolverSystem::WarmStart() {
        // Re-apply the impulses accumulated in the previous step as an initial guess, so the velocity solver
        // starts close to the solution and converges in fewer iterations. Body 1 receives the negated impulse
        // terms and body 2 the positive ones, mirroring the impulse pairs SolveVelocityConstraint applies.
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _sliderJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            // Get the inverse mass of the bodies.
            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &linearVelocityBodyOne = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &linearVelocityBodyTwo = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularVelocityBodyOne = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &angularVelocityBodyTwo = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Per-axis factors that zero out the impulse on locked/frozen degrees of freedom.
            const glm::vec3 &linearlockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearlockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const glm::mat3 &inertiaTensorBodyOne = _sliderJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _sliderJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // n1 and n2 are the two axes orthogonal to the slider axis; together with the slider axis they
            // form the basis the 2-DOF translation constraint's Jacobian is built from (see InitializeBeforeSolving).
            const glm::vec3 &n1 = _sliderJointStore.GetN1AtIndex(i);
            const glm::vec3 &n2 = _sliderJointStore.GetN2AtIndex(i);

            const glm::vec3 &sliderAxisInWorldSpace = _sliderJointStore.GetSliderAxisInWorldSpaceAtIndex(i);

            // Compute the impulse P=J^T * lambda for the lower and upper limits constraints of body 1
            const f32 impulseLimits = _sliderJointStore.GetImpulseUpperLimitAtIndex(i) - _sliderJointStore.GetImpulseLowerLimitAtIndex(i);
            const glm::vec3 linearImpulseLimits = impulseLimits * sliderAxisInWorldSpace;

            // Compute the impulse P=J^T * lambda for the motor constraint.
            const glm::vec3 impulseMotor = _sliderJointStore.GetImpulseMotorAtIndex(i) * sliderAxisInWorldSpace;

            // Get the accumulated translation and rotation impulses to re-apply.
            const glm::vec2 &impulseTranslation = _sliderJointStore.GetImpulseTranslationAtIndex(i);
            const glm::vec3 &impulseRotation = _sliderJointStore.GetImpulseRotationAtIndex(i);

            // Compute the impulse P=J^T * lambda for the 2 translation constraints for body 1.
            glm::vec3 linearImpulseBodyOne = -n1 * impulseTranslation.x - n2 * impulseTranslation.y;
            glm::vec3 angularImpulseBodyOne =
                -_sliderJointStore.GetR1PlusUCrossN1AtIndex(i) * impulseTranslation.x - _sliderJointStore.GetR1PlusUCrossN2AtIndex(i) * impulseTranslation.y;

            // Add the rotation, limit and motor impulse contributions for body 1.
            angularImpulseBodyOne += -impulseRotation;
            linearImpulseBodyOne += linearImpulseLimits;
            angularImpulseBodyOne += impulseLimits * _sliderJointStore.GetR1PlusUCrossSliderAxisAtIndex(i);
            linearImpulseBodyOne += impulseMotor;

            // Apply the impulse to body 1.
            const glm::vec3 newLinearVelocityBodyOne = linearVelocityBodyOne + inverseMassBodyOne * linearlockAxisFactorBodyOne * linearImpulseBodyOne;
            const glm::vec3 newAngularVelocityBodyOne = angularVelocityBodyOne + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newLinearVelocityBodyOne);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newAngularVelocityBodyOne);

            // Compute the impulse P=J^T * lambda for the 2 translation constraints for body 2.
            glm::vec3 linearImpulseBodyTwo = n1 * impulseTranslation.x + n2 * impulseTranslation.y;
            glm::vec3 angularImpulseBodyTwo =
                _sliderJointStore.GetR2CrossN1AtIndex(i) * impulseTranslation.x + _sliderJointStore.GetR2CrossN2AtIndex(i) * impulseTranslation.y;

            // Add the rotation, limit and motor impulse contributions for body 2 (equal and opposite to body 1's).
            angularImpulseBodyTwo += impulseRotation;
            linearImpulseBodyTwo += -linearImpulseLimits;
            angularImpulseBodyTwo += -impulseLimits * _sliderJointStore.GetR2CrossSliderAxisAtIndex(i);
            linearImpulseBodyTwo += -impulseMotor;

            // Apply the impulse to body 2.
            const glm::vec3 newLinearVelocityBodyTwo = linearVelocityBodyTwo + inverseMassBodyTwo * linearlockAxisFactorBodyTwo * linearImpulseBodyTwo;
            const glm::vec3 newAngularVelocityBodyTwo = angularVelocityBodyTwo + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBodyTwo);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newLinearVelocityBodyTwo);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newAngularVelocityBodyTwo);
        }
    }

    void SliderJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
        // One sequential (projected Gauss-Seidel) sweep per active slider joint over its 4 sub-constraints:
        // limits, motor, rotation, translation. Velocities are read from and written back to the rigid body
        // store on every impulse application, so later sub-constraints in this same sweep see the effect of
        // earlier ones - this ordering intentionally mirrors ReactPhysics3D's solveVelocityConstraint().
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _sliderJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const glm::mat3 &i1 = _sliderJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &i2 = _sliderJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::vec3 &n1 = _sliderJointStore.GetN1AtIndex(i);
            const glm::vec3 &n2 = _sliderJointStore.GetN2AtIndex(i);

            const glm::vec3 &r2CrossN1 = _sliderJointStore.GetR2CrossN1AtIndex(i);
            const glm::vec3 &r2CrossN2 = _sliderJointStore.GetR2CrossN2AtIndex(i);
            const glm::vec3 &r1PlusUCrossN1 = _sliderJointStore.GetR1PlusUCrossN1AtIndex(i);
            const glm::vec3 &r1PlusUCrossN2 = _sliderJointStore.GetR1PlusUCrossN2AtIndex(i);

            const f32 inverseMassBody1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBody2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            const glm::vec3 &r2CrossSliderAxis = _sliderJointStore.GetR2CrossSliderAxisAtIndex(i);
            const glm::vec3 &r1PlusUCrossSliderAxis = _sliderJointStore.GetR1PlusUCrossSliderAxisAtIndex(i);

            const glm::vec3 &sliderAxisWorld = _sliderJointStore.GetSliderAxisInWorldSpaceAtIndex(i);

            // --------------- Limits Constraints --------------- //

            if (_sliderJointStore.IsLimitEnabledAtIndex(i)) {
                const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
                const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

                const f32 inverseMassMatrixLimit = _sliderJointStore.GetInverseMassMatrixLimitAtIndex(i);

                if (_sliderJointStore.GetIsLowerLimitViolatedAtIndex(i)) {
                    // J*v: relative velocity of the two anchor points along the slider axis (body 2 relative
                    // to body 1). This constraint only pushes the anchors apart, so it needs this to stay >= 0.
                    const f32 JvLowerLimit =
                        glm::dot(sliderAxisWorld, v2) + glm::dot(r2CrossSliderAxis, w2) - glm::dot(sliderAxisWorld, v1) - glm::dot(r1PlusUCrossSliderAxis, w1);

                    // Solve for the incremental Lagrange multiplier, then clamp the accumulated impulse to
                    // remain non-negative - a lower limit can only push the bodies apart, never pull them
                    // together. The impulse actually applied below is the delta between the clamped and
                    // pre-clamp accumulated value, not the raw solve, so the clamp isn't undone next iteration.
                    f32 deltaLambdaLower = inverseMassMatrixLimit * (-JvLowerLimit - _sliderJointStore.GetBiasLowerLimitAtIndex(i));
                    const f32 lambdaTemp = _sliderJointStore.GetImpulseLowerLimitAtIndex(i);
                    const f32 newImpulseLowerLimit = std::max(lambdaTemp + deltaLambdaLower, f32(0.0));
                    _sliderJointStore.SetImpulseLowerLimitAtIndex(i, newImpulseLowerLimit);
                    deltaLambdaLower = newImpulseLowerLimit - lambdaTemp;

                    // Apply the impulse P = J^T * lambda to body 1, and the equal-and-opposite impulse to body 2.
                    const glm::vec3 linearImpulseBody1 = -deltaLambdaLower * sliderAxisWorld;
                    const glm::vec3 angularImpulseBody1 = -deltaLambdaLower * r1PlusUCrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1));

                    const glm::vec3 linearImpulseBody2 = deltaLambdaLower * sliderAxisWorld;
                    const glm::vec3 angularImpulseBody2 = deltaLambdaLower * r2CrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2));
                }

                if (_sliderJointStore.GetIsUpperLimitViolatedAtIndex(i)) {
                    // Mirrors the lower limit above with the Jacobian sign flipped (body 1 relative to body 2):
                    // the upper limit is violated when the anchor separation exceeds the upper bound, so it
                    // pushes the anchors back together instead of apart.
                    const f32 JvUpperLimit =
                        glm::dot(sliderAxisWorld, v1) + glm::dot(r1PlusUCrossSliderAxis, w1) - glm::dot(sliderAxisWorld, v2) - glm::dot(r2CrossSliderAxis, w2);

                    f32 deltaLambdaUpper = inverseMassMatrixLimit * (-JvUpperLimit - _sliderJointStore.GetBiasUpperLimitAtIndex(i));
                    const f32 lambdaTemp = _sliderJointStore.GetImpulseUpperLimitAtIndex(i);
                    const f32 newImpulseUpperLimit = std::max(lambdaTemp + deltaLambdaUpper, f32(0.0));
                    _sliderJointStore.SetImpulseUpperLimitAtIndex(i, newImpulseUpperLimit);
                    deltaLambdaUpper = newImpulseUpperLimit - lambdaTemp;

                    const glm::vec3 linearImpulseBody1 = deltaLambdaUpper * sliderAxisWorld;
                    const glm::vec3 angularImpulseBody1 = deltaLambdaUpper * r1PlusUCrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1));

                    const glm::vec3 linearImpulseBody2 = -deltaLambdaUpper * sliderAxisWorld;
                    const glm::vec3 angularImpulseBody2 = -deltaLambdaUpper * r2CrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2));
                }
            }

            // --------------- Motor --------------- //

            if (_sliderJointStore.IsMotorEnabledAtIndex(i)) {
                // J*v: relative velocity along the slider axis; the motor drives this towards the target speed.
                const f32 JvMotor = glm::dot(sliderAxisWorld, v1) - glm::dot(sliderAxisWorld, v2);

                // Solve for the incremental impulse and clamp the accumulated motor impulse to the maximum
                // impulse the motor can deliver this timestep (max force x dt), in either direction - unlike
                // the limits above, the motor is bilateral (it can push or pull).
                const f32 maxMotorImpulse = _sliderJointStore.GetMaxMotorForceAtIndex(i) * timestep.GetSeconds();
                f32 deltaLambdaMotor = _sliderJointStore.GetInverseMassMatrixMotorAtIndex(i) * (-JvMotor - _sliderJointStore.GetMotorSpeedAtIndex(i));
                const f32 lambdaTemp = _sliderJointStore.GetImpulseMotorAtIndex(i);
                const f32 newImpulseMotor = std::clamp(lambdaTemp + deltaLambdaMotor, -maxMotorImpulse, maxMotorImpulse);
                _sliderJointStore.SetImpulseMotorAtIndex(i, newImpulseMotor);
                deltaLambdaMotor = newImpulseMotor - lambdaTemp;

                // Apply the impulse along the slider axis to both bodies; a pure slide has no angular component.
                const glm::vec3 linearImpulseBody1 = deltaLambdaMotor * sliderAxisWorld;
                _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1);

                const glm::vec3 linearImpulseBody2 = -deltaLambdaMotor * sliderAxisWorld;
                _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
            }

            // --------------- Rotation Constraints --------------- //

            // J*v for the 3 rotation-locking constraints: the two bodies' angular velocities should match, so
            // that the relative orientation stays fixed at its initial value (see InitializeBeforeSolving's
            // qError bias computation). This is a bilateral (equality) constraint, so no clamping is needed.
            const glm::vec3 JvRotation = w2 - w1;
            const glm::vec3 deltaLambda2 =
                _sliderJointStore.GetInverseMassRotationMatrixAtIndex(i) * (-JvRotation - _sliderJointStore.GetRotationBias(jointEntity));
            _sliderJointStore.SetImpulseRotationAtIndex(i, _sliderJointStore.GetImpulseRotationAtIndex(i) + deltaLambda2);

            // Apply the equal-and-opposite angular impulse to both bodies. angularImpulseBody1/2 are reused
            // (reassigned) below for the translation constraints' impulses rather than redeclared.
            glm::vec3 angularImpulseBody1 = -deltaLambda2;
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1));

            glm::vec3 angularImpulseBody2 = deltaLambda2;
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2));

            // --------------- Translation Constraints --------------- //

            // J*v for the 2-DOF translation constraint: the anchor points' relative velocity projected onto
            // n1/n2, the plane orthogonal to the slider axis (zero once the anchors stay aligned along the
            // axis). Also a bilateral constraint - the joint resists drift off-axis in either direction.
            const f32 el1 = -glm::dot(n1, v1) - glm::dot(w1, r1PlusUCrossN1) + glm::dot(n1, v2) + glm::dot(w2, r2CrossN1);
            const f32 el2 = -glm::dot(n2, v1) - glm::dot(w1, r1PlusUCrossN2) + glm::dot(n2, v2) + glm::dot(w2, r2CrossN2);
            const glm::vec2 JvTranslation(el1, el2);

            // Solve the 2x2 system for the incremental impulse and accumulate it.
            const glm::vec2 deltaLambda =
                _sliderJointStore.GetInverseMassTranslationMatrixAtIndex(i) * (-JvTranslation - _sliderJointStore.GetTranslationBiasAtIndex(i));
            _sliderJointStore.SetImpulseTranslationAtIndex(i, _sliderJointStore.GetImpulseTranslationAtIndex(i) + deltaLambda);

            // Apply the impulse P = J^T * lambda to body 1, and the equal-and-opposite impulse to body 2.
            const glm::vec3 linearImpulseBody1 = -n1 * deltaLambda.x - n2 * deltaLambda.y;
            angularImpulseBody1 = -r1PlusUCrossN1 * deltaLambda.x - r1PlusUCrossN2 * deltaLambda.y;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1));

            const glm::vec3 linearImpulseBody2 = -linearImpulseBody1;
            angularImpulseBody2 = r2CrossN1 * deltaLambda.x + r2CrossN2 * deltaLambda.y;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2));
        }
    }

    void SliderJointSolverSystem::SolvePositionConstraint() {
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _sliderJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            const glm::vec3 &x1 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);
            const glm::quat &q1 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &q2 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            const glm::vec3 &bodyOneLocalInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &bodyTwoLocalInertiaTensor = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            glm::mat3 i1;
            glm::mat3 i2;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q1), bodyOneLocalInertiaTensor, i1);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q2), bodyTwoLocalInertiaTensor, i2);

            _sliderJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, i1);
            _sliderJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, i2);

            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const f32 inverseMassBody1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBody2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInverseMass = inverseMassBody1 + inverseMassBody2;

            const glm::vec3 &r1 = _sliderJointStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2 = _sliderJointStore.GetR2WorldAtIndex(i);

            const glm::vec3 u = x2 + r2 - x1 - r1;
            const glm::vec3 sliderAxisInWorldSpace = glm::normalize(q1 * _sliderJointStore.GetSliderAxisInBodyOneLocalSpaceAtIndex(i));
            _sliderJointStore.SetSliderAxisInWorldSpaceAtIndex(i, sliderAxisInWorldSpace);

            const glm::vec3 n1 = GetOrthogonalUnitVector(sliderAxisInWorldSpace);
            const glm::vec3 n2 = glm::cross(sliderAxisInWorldSpace, n1);

            _sliderJointStore.SetN1AtIndex(i, n1);
            _sliderJointStore.SetN2AtIndex(i, n2);

            const f32 uDotSliderAxis = glm::dot(u, sliderAxisInWorldSpace);
            const f32 lowerLimitError = uDotSliderAxis - _sliderJointStore.GetLowerLimit(jointEntity);
            const f32 upperLimitError = _sliderJointStore.GetUpperLimit(jointEntity) - uDotSliderAxis;
            const bool lowerLimitViolated = lowerLimitError <= 0;
            const bool upperLimitViolated = upperLimitError <= 0;

            _sliderJointStore.SetIsLowerLimitViolatedAtIndex(i, lowerLimitViolated);
            _sliderJointStore.SetIsUpperLimitViolatedAtIndex(i, upperLimitViolated);

            const glm::vec3 r2CrossN1 = glm::cross(r2, n1);
            const glm::vec3 r2CrossN2 = glm::cross(r2, n2);
            const glm::vec3 r2CrossSliderAxis = glm::cross(r2, sliderAxisInWorldSpace);

            _sliderJointStore.SetR2CrossN1AtIndex(i, r2CrossN1);
            _sliderJointStore.SetR2CrossN2AtIndex(i, r2CrossN2);
            _sliderJointStore.SetR2CrossSliderAxisAtIndex(i, r2CrossSliderAxis);

            const glm::vec3 r1PlusU = r1 + u;
            const glm::vec3 r1PlusUCrossN1 = glm::cross(r1PlusU, n1);
            const glm::vec3 r1PlusUCrossN2 = glm::cross(r1PlusU, n2);
            const glm::vec3 r1PlusUCrossSliderAxis = glm::cross(r1PlusU, sliderAxisInWorldSpace);

            _sliderJointStore.SetR1PlusUCrossN1AtIndex(i, r1PlusUCrossN1);
            _sliderJointStore.SetR1PlusUCrossN2AtIndex(i, r1PlusUCrossN2);
            _sliderJointStore.SetR1PlusUCrossSliderAxisAtIndex(i, r1PlusUCrossSliderAxis);

            // --------------- Limits Constraints --------------- //

            if (_sliderJointStore.IsLimitEnabledAtIndex(i)) {
                if (lowerLimitViolated || upperLimitViolated) {
                    const f32 temp =
                        sumInverseMass + glm::dot(r1PlusUCrossSliderAxis, i1 * r1PlusUCrossSliderAxis) + glm::dot(r2CrossSliderAxis, i2 * r2CrossSliderAxis);
                    _sliderJointStore.SetInverseMassMatrixLimitAtIndex(i, temp > f32(0.0) ? f32(1.0) / temp : f32(0.0));
                }

                if (lowerLimitViolated) {
                    const f32 lambdaLowerLimit = _sliderJointStore.GetInverseMassMatrixLimitAtIndex(i) * (-lowerLimitError);

                    const glm::vec3 linearImpulseBody1 = -lambdaLowerLimit * sliderAxisInWorldSpace;
                    const glm::vec3 angularImpulseBody1 = -lambdaLowerLimit * r1PlusUCrossSliderAxis;
                    const glm::vec3 v1 = inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                    const glm::vec3 w1 = angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                    const glm::vec3 linearImpulseBody2 = lambdaLowerLimit * sliderAxisInWorldSpace;
                    const glm::vec3 angularImpulseBody2 = lambdaLowerLimit * r2CrossSliderAxis;
                    const glm::vec3 v2 = inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
                    const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
                }

                if (upperLimitViolated) {
                    f32 lambdaUpperLimit = _sliderJointStore.GetInverseMassMatrixLimitAtIndex(i) * (-upperLimitError);

                    const glm::vec3 linearImpulseBody1 = lambdaUpperLimit * sliderAxisInWorldSpace;
                    const glm::vec3 angularImpulseBody1 = lambdaUpperLimit * r1PlusUCrossSliderAxis;
                    const glm::vec3 v1 = inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                    const glm::vec3 w1 = angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                    const glm::vec3 linearImpulseBody2 = -lambdaUpperLimit * sliderAxisInWorldSpace;
                    const glm::vec3 angularImpulseBody2 = -lambdaUpperLimit * r2CrossSliderAxis;
                    const glm::vec3 v2 = inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
                    const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
                }
            }

            // --------------- Rotation Constraints --------------- //

            const glm::mat3 inverseMassRotationMatrix = i1 + i2;
            _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, inverseMassRotationMatrix);
            f32 massMatrixRotationDeterminant = glm::determinant(inverseMassRotationMatrix);

            if (VE_MACHINE_EPSILON < std::abs(massMatrixRotationDeterminant)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat3(inverseMassRotationMatrix, massMatrixRotationDeterminant));
                }

                const glm::quat qError = q2 * _sliderJointStore.GetInitialOrientationDifferenceInverseAtIndex(i) * glm::inverse(q1);
                const glm::vec3 errorRotation = f32(2.0) * glm::vec3(qError.x, qError.y, qError.z);
                const glm::vec3 lambdaRotation = _sliderJointStore.GetInverseMassRotationMatrixAtIndex(i) * (-errorRotation);

                const glm::vec3 angularImpulseBody1 = -lambdaRotation;
                const glm::vec3 w1 = angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                const glm::vec3 angularImpulseBody2 = lambdaRotation;
                const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
            }

            // --------------- Translation Constraints --------------- //

            const glm::vec3 I1R1PlusUCrossN1 = i1 * r1PlusUCrossN1;
            const glm::vec3 I1R1PlusUCrossN2 = i1 * r1PlusUCrossN2;
            const glm::vec3 I2R2CrossN1 = i2 * r2CrossN1;
            const glm::vec3 I2R2CrossN2 = i2 * r2CrossN2;
            const f32 el11 = sumInverseMass + glm::dot(r1PlusUCrossN1, I1R1PlusUCrossN1) + glm::dot(r2CrossN1, I2R2CrossN1);
            const f32 el12 = glm::dot(r1PlusUCrossN1, I1R1PlusUCrossN2) + glm::dot(r2CrossN1, I2R2CrossN2);
            const f32 el21 = glm::dot(r1PlusUCrossN2, I1R1PlusUCrossN1) + glm::dot(r2CrossN2, I2R2CrossN1);
            const f32 el22 = sumInverseMass + glm::dot(r1PlusUCrossN2, I1R1PlusUCrossN2) + glm::dot(r2CrossN2, I2R2CrossN2);
            const glm::mat2 matrixKTranslation(el11, el12, el21, el22);

            _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat2(0));
            f32 matrixKTranslationDeterminant = glm::determinant(matrixKTranslation);

            if (std::abs(matrixKTranslationDeterminant) > VE_MACHINE_EPSILON) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat2(matrixKTranslation, matrixKTranslationDeterminant));
                }

                const glm::vec2 translationError(glm::dot(u, n1), glm::dot(u, n2));
                const glm::vec2 lambdaTranslation = _sliderJointStore.GetInverseMassTranslationMatrixAtIndex(i) * (-translationError);

                const glm::vec3 linearImpulseBody1 = -n1 * lambdaTranslation.x - n2 * lambdaTranslation.y;
                const glm::vec3 angularImpulseBody1 = -r1PlusUCrossN1 * lambdaTranslation.x - r1PlusUCrossN2 * lambdaTranslation.y;
                const glm::vec3 v1 = inverseMassBody1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                const glm::vec3 w1 = angularLockAxisFactorBodyOne * (i1 * angularImpulseBody1);
                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                const glm::vec3 linearImpulseBody2 = n1 * lambdaTranslation.x + n2 * lambdaTranslation.y;
                const glm::vec3 angularImpulseBody2 = r2CrossN1 * lambdaTranslation.x + r2CrossN2 * lambdaTranslation.y;
                const glm::vec3 v2 = inverseMassBody2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
                const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (i2 * angularImpulseBody2);
                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
            }
        }
    }

} // namespace Vulkyrie
