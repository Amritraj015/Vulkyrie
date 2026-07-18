#include "physics/systems/slider_joint_solver_system.h"
#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"
#include "core/asserts.h"
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
        const size_t activeJointCount = _sliderJointStore.GetActiveComponentCount();
        _jointIndices.resize(activeJointCount);

        for (size_t i = 0; i < activeJointCount; ++i) {
            const Entity jointEntity = _sliderJointStore.GetEntityAtIndex(i);
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

            // Cache the world-space inverse inertia tensors of both bodies for use in the mass matrices below.
            const glm::mat3 &invI1 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex);
            const glm::mat3 &invI2 = _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex);
            _sliderJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _sliderJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            const glm::quat &q1 = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &q2 = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _sliderJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _sliderJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the lever arms (anchor point relative to center of mass) in world space.
            const glm::vec3 r1 = q1 * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 r2 = q2 * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _sliderJointStore.SetR1WorldAtIndex(i, r1);
            _sliderJointStore.SetR2WorldAtIndex(i, r2);

            // Compute the slider axis in world space; n1 and n2 span the plane orthogonal to it and form the
            // basis the 2-DOF translation constraint's Jacobian (below) is built from.
            const glm::vec3 sliderAxis = glm::normalize(q1 * _sliderJointStore.GetSliderAxisInBodyOneLocalSpaceAtIndex(i));
            _sliderJointStore.SetSliderAxisInWorldSpaceAtIndex(i, sliderAxis);

            const glm::vec3 n1 = GetOrthogonalUnitVector(sliderAxis);
            const glm::vec3 n2 = glm::cross(sliderAxis, n1);
            _sliderJointStore.SetN1AtIndex(i, n1);
            _sliderJointStore.SetN2AtIndex(i, n2);

            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            // u is the world-space separation between the anchor points (zero once the translation constraint
            // is satisfied). Body 1's angular Jacobian block for the n1/n2 constraints uses r1PlusU = r1 + u
            // rather than r1 alone, since sliding along the axis shifts body 1's effective lever arm relative
            // to body 2's anchor - this is the standard slider-joint derivation.
            const glm::vec3 u = x2 + r2 - x1 - r1;
            const glm::vec3 r1PlusU = r1 + u;
            const glm::vec3 r1PlusUCrossN1 = glm::cross(r1PlusU, n1);
            const glm::vec3 r1PlusUCrossN2 = glm::cross(r1PlusU, n2);
            const glm::vec3 r1PlusUCrossSliderAxis = glm::cross(r1PlusU, sliderAxis);

            // Precomputed Jacobian lever-arm terms, reused below and in WarmStart/SolveVelocityConstraint.
            _sliderJointStore.SetR1PlusUCrossN1AtIndex(i, r1PlusUCrossN1);
            _sliderJointStore.SetR1PlusUCrossN2AtIndex(i, r1PlusUCrossN2);
            _sliderJointStore.SetR1PlusUCrossSliderAxisAtIndex(i, r1PlusUCrossSliderAxis);

            const glm::vec3 r2CrossN1 = glm::cross(r2, n1);
            const glm::vec3 r2CrossN2 = glm::cross(r2, n2);
            const glm::vec3 r2CrossSliderAxis = glm::cross(r2, sliderAxis);
            _sliderJointStore.SetR2CrossN1AtIndex(i, r2CrossN1);
            _sliderJointStore.SetR2CrossN2AtIndex(i, r2CrossN2);
            _sliderJointStore.SetR2CrossSliderAxisAtIndex(i, r2CrossSliderAxis);

            // Determine whether the lower/upper limit constraints (translation along the slider axis) are
            // currently violated. A limit's accumulated impulse is reset whenever it stops being violated or
            // flips violation state, since a stale impulse from a different regime would bias the new solve.
            const f32 uDotSliderAxis = glm::dot(u, sliderAxis);
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
            if (baumgartePositionCorrection) {
                const glm::vec2 bTranslation = glm::vec2(glm::dot(u, n1) * biasFactor, glm::dot(u, n2) * biasFactor);
                _sliderJointStore.SetTranslationBiasAtIndex(i, bTranslation);
            } else {
                _sliderJointStore.SetTranslationBiasAtIndex(i, glm::vec2(0));
            }

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            // The limit constraint shares its 1-DOF Jacobian (the slider axis) with the motor, but unlike the
            // motor its inverse mass matrix also has an angular component, so it's computed separately here.
            if (_sliderJointStore.IsLimitEnabledAtIndex(i) && (lowerLimitViolated || upperLimitViolated)) {
                const f32 kLimit =
                    sumInvMass + glm::dot(r1PlusUCrossSliderAxis, invI1 * r1PlusUCrossSliderAxis) + glm::dot(r2CrossSliderAxis, invI2 * r2CrossSliderAxis);
                _sliderJointStore.SetInverseMassMatrixLimitAtIndex(i, kLimit > f32(0.0) ? f32(1.0) / kLimit : f32(0.0));

                // Compute the bias "b" of the lower and upper limit constraints.
                if (baumgartePositionCorrection) {
                    _sliderJointStore.SetBiasLowerLimitAtIndex(i, biasFactor * lowerLimitError);
                    _sliderJointStore.SetBiasUpperLimitAtIndex(i, biasFactor * upperLimitError);
                } else {
                    _sliderJointStore.SetBiasLowerLimitAtIndex(i, f32(0.0));
                    _sliderJointStore.SetBiasUpperLimitAtIndex(i, f32(0.0));
                }
            }

            // Compute the mass matrix K = J*M^-1*J^t (2x2) for the 2 translation constraints, then its inverse.
            // K is symmetric (el21 == el12) because the inverse inertia tensors are symmetric, so only three
            // of the four elements need computing.
            const glm::vec3 invI1R1PlusUCrossN1 = invI1 * r1PlusUCrossN1;
            const glm::vec3 invI1R1PlusUCrossN2 = invI1 * r1PlusUCrossN2;
            const glm::vec3 invI2R2CrossN1 = invI2 * r2CrossN1;
            const glm::vec3 invI2R2CrossN2 = invI2 * r2CrossN2;
            const f32 el11 = sumInvMass + glm::dot(r1PlusUCrossN1, invI1R1PlusUCrossN1) + glm::dot(r2CrossN1, invI2R2CrossN1);
            const f32 el12 = glm::dot(r1PlusUCrossN1, invI1R1PlusUCrossN2) + glm::dot(r2CrossN1, invI2R2CrossN2);
            const f32 el22 = sumInvMass + glm::dot(r1PlusUCrossN2, invI1R1PlusUCrossN2) + glm::dot(r2CrossN2, invI2R2CrossN2);

            const glm::mat2 kTranslation(el11, el12, el12, el22);

            _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat2(0));
            const f32 kTranslationDet = glm::determinant(kTranslation);

            // Skip the inverse (leave it zeroed) if the mass matrix is singular or both bodies are non-dynamic;
            // a singular K means the constraint is degenerate and has no unique impulse solution this step.
            if (VE_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat2(kTranslation, kTranslationDet));
                }
            }

            // Compute the mass matrix K = I1 + I2 (3x3) for the 3 rotation constraints, then its inverse,
            // leaving it zeroed for a singular or fully non-dynamic body pair (same guards as the
            // translation matrix above).
            const glm::mat3 kRotation = invI1 + invI2;
            _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, glm::mat3(0));
            const f32 kRotationDet = glm::determinant(kRotation);

            if (VE_MACHINE_EPSILON < std::abs(kRotationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat3(kRotation, kRotationDet));
                }
            }

            // Compute the Baumgarte bias "b" for the 3 rotation constraints from the quaternion error between
            // the current and initial relative orientations (2x the error quaternion's vector part is the
            // small-angle approximation of the orientation drift).
            if (baumgartePositionCorrection) {
                const glm::quat qError = q2 * _sliderJointStore.GetInitialOrientationDifferenceInverseAtIndex(i) * glm::inverse(q1);
                _sliderJointStore.SetRotationBiasAtIndex(i, biasFactor * f32(2.0) * glm::vec3(qError.x, qError.y, qError.z));
            } else {
                _sliderJointStore.SetRotationBiasAtIndex(i, glm::vec3(0));
            }

            // Compute the inverse of the 1x1 mass matrix K = M1^-1 + M2^-1 for the motor constraint. Motion
            // along the slider axis is a pure translation, so unlike the hinge/limit motors this has no
            // angular (inertia tensor) component.
            if (_sliderJointStore.IsMotorEnabledAtIndex(i)) {
                _sliderJointStore.SetInverseMassMatrixMotorAtIndex(i, sumInvMass > f32(0.0) ? f32(1.0) / sumInvMass : f32(0.0));
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
        VASSERT(_jointIndices.size() == _sliderJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before WarmStart().");

        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            // Get the inverse mass of the bodies.
            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Per-axis factors that zero out the impulse on locked/frozen degrees of freedom.
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const glm::mat3 &inertiaTensorBodyOne = _sliderJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _sliderJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            // n1 and n2 are the two axes orthogonal to the slider axis; together with the slider axis they
            // form the basis the 2-DOF translation constraint's Jacobian is built from (see InitializeBeforeSolving).
            const glm::vec3 &n1 = _sliderJointStore.GetN1AtIndex(i);
            const glm::vec3 &n2 = _sliderJointStore.GetN2AtIndex(i);

            const glm::vec3 &sliderAxis = _sliderJointStore.GetSliderAxisInWorldSpaceAtIndex(i);

            // Compute the impulse P=J^T * lambda for the lower and upper limits constraints of body 1
            const f32 impulseLimits = _sliderJointStore.GetImpulseUpperLimitAtIndex(i) - _sliderJointStore.GetImpulseLowerLimitAtIndex(i);
            const glm::vec3 linearImpulseLimits = impulseLimits * sliderAxis;

            // Compute the impulse P=J^T * lambda for the motor constraint.
            const glm::vec3 impulseMotor = _sliderJointStore.GetImpulseMotorAtIndex(i) * sliderAxis;

            // Get the accumulated translation and rotation impulses to re-apply.
            const glm::vec2 &impulseTranslation = _sliderJointStore.GetImpulseTranslationAtIndex(i);
            const glm::vec3 &impulseRotation = _sliderJointStore.GetImpulseRotationAtIndex(i);

            // Compute the impulse P=J^T * lambda for the 2 translation constraints for body 1.
            glm::vec3 linearImpulseBody1 = -n1 * impulseTranslation.x - n2 * impulseTranslation.y;
            glm::vec3 angularImpulseBody1 =
                -_sliderJointStore.GetR1PlusUCrossN1AtIndex(i) * impulseTranslation.x - _sliderJointStore.GetR1PlusUCrossN2AtIndex(i) * impulseTranslation.y;

            // Add the rotation, limit and motor impulse contributions for body 1.
            angularImpulseBody1 += -impulseRotation;
            linearImpulseBody1 += linearImpulseLimits;
            angularImpulseBody1 += impulseLimits * _sliderJointStore.GetR1PlusUCrossSliderAxisAtIndex(i);
            linearImpulseBody1 += impulseMotor;

            // Apply the impulse to body 1.
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            const glm::vec3 newW1 = w1 + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBody1);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

            // Compute the impulse P=J^T * lambda for the 2 translation constraints for body 2.
            glm::vec3 linearImpulseBody2 = n1 * impulseTranslation.x + n2 * impulseTranslation.y;
            glm::vec3 angularImpulseBody2 =
                _sliderJointStore.GetR2CrossN1AtIndex(i) * impulseTranslation.x + _sliderJointStore.GetR2CrossN2AtIndex(i) * impulseTranslation.y;

            // Add the rotation, limit and motor impulse contributions for body 2 (equal and opposite to body 1's).
            angularImpulseBody2 += impulseRotation;
            linearImpulseBody2 += -linearImpulseLimits;
            angularImpulseBody2 += -impulseLimits * _sliderJointStore.GetR2CrossSliderAxisAtIndex(i);
            linearImpulseBody2 += -impulseMotor;

            // Apply the impulse to body 2.
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
            const glm::vec3 newW2 = w2 + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBody2);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
        }
    }

    void SliderJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
        // One sequential (projected Gauss-Seidel) sweep per active slider joint over its 4 sub-constraints:
        // limits, motor, rotation, translation. Velocities are read from and written back to the rigid body
        // store on every impulse application, so later sub-constraints in this same sweep see the effect of
        // earlier ones - this ordering intentionally mirrors ReactPhysics3D's solveVelocityConstraint().
        VASSERT(_jointIndices.size() == _sliderJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolveVelocityConstraint().");

        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const glm::mat3 &invI1 = _sliderJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _sliderJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::vec3 &n1 = _sliderJointStore.GetN1AtIndex(i);
            const glm::vec3 &n2 = _sliderJointStore.GetN2AtIndex(i);

            const glm::vec3 &r2CrossN1 = _sliderJointStore.GetR2CrossN1AtIndex(i);
            const glm::vec3 &r2CrossN2 = _sliderJointStore.GetR2CrossN2AtIndex(i);
            const glm::vec3 &r1PlusUCrossN1 = _sliderJointStore.GetR1PlusUCrossN1AtIndex(i);
            const glm::vec3 &r1PlusUCrossN2 = _sliderJointStore.GetR1PlusUCrossN2AtIndex(i);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            const glm::vec3 &r2CrossSliderAxis = _sliderJointStore.GetR2CrossSliderAxisAtIndex(i);
            const glm::vec3 &r1PlusUCrossSliderAxis = _sliderJointStore.GetR1PlusUCrossSliderAxisAtIndex(i);

            const glm::vec3 &sliderAxis = _sliderJointStore.GetSliderAxisInWorldSpaceAtIndex(i);

            // --------------- Limits Constraints --------------- //

            if (_sliderJointStore.IsLimitEnabledAtIndex(i)) {
                const f32 invKLimit = _sliderJointStore.GetInverseMassMatrixLimitAtIndex(i);

                if (_sliderJointStore.GetIsLowerLimitViolatedAtIndex(i)) {
                    // J*v: relative velocity of the two anchor points along the slider axis (body 2 relative
                    // to body 1). This constraint only pushes the anchors apart, so it needs this to stay >= 0.
                    const f32 JvLowerLimit =
                        glm::dot(sliderAxis, v2) + glm::dot(r2CrossSliderAxis, w2) - glm::dot(sliderAxis, v1) - glm::dot(r1PlusUCrossSliderAxis, w1);

                    // Solve for the incremental Lagrange multiplier, then clamp the accumulated impulse to
                    // remain non-negative - a lower limit can only push the bodies apart, never pull them
                    // together. The impulse actually applied below is the delta between the clamped and
                    // pre-clamp accumulated value, not the raw solve, so the clamp isn't undone next iteration.
                    f32 deltaLambdaLower = invKLimit * (-JvLowerLimit - _sliderJointStore.GetBiasLowerLimitAtIndex(i));
                    const f32 currentImpulseLowerLimit = _sliderJointStore.GetImpulseLowerLimitAtIndex(i);
                    const f32 newImpulseLowerLimit = std::max(currentImpulseLowerLimit + deltaLambdaLower, f32(0.0));
                    _sliderJointStore.SetImpulseLowerLimitAtIndex(i, newImpulseLowerLimit);
                    deltaLambdaLower = newImpulseLowerLimit - currentImpulseLowerLimit;

                    // Apply the impulse P = J^T * lambda to body 1, and the equal-and-opposite impulse to body 2.
                    const glm::vec3 linearImpulseBody1 = -deltaLambdaLower * sliderAxis;
                    const glm::vec3 angularImpulseBody1 = -deltaLambdaLower * r1PlusUCrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

                    const glm::vec3 linearImpulseBody2 = deltaLambdaLower * sliderAxis;
                    const glm::vec3 angularImpulseBody2 = deltaLambdaLower * r2CrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
                }

                if (_sliderJointStore.GetIsUpperLimitViolatedAtIndex(i)) {
                    // Mirrors the lower limit above with the Jacobian sign flipped (body 1 relative to body 2):
                    // the upper limit is violated when the anchor separation exceeds the upper bound, so it
                    // pushes the anchors back together instead of apart.
                    const f32 JvUpperLimit =
                        glm::dot(sliderAxis, v1) + glm::dot(r1PlusUCrossSliderAxis, w1) - glm::dot(sliderAxis, v2) - glm::dot(r2CrossSliderAxis, w2);

                    f32 deltaLambdaUpper = invKLimit * (-JvUpperLimit - _sliderJointStore.GetBiasUpperLimitAtIndex(i));
                    const f32 currentImpulseUpperLimit = _sliderJointStore.GetImpulseUpperLimitAtIndex(i);
                    const f32 newImpulseUpperLimit = std::max(currentImpulseUpperLimit + deltaLambdaUpper, f32(0.0));
                    _sliderJointStore.SetImpulseUpperLimitAtIndex(i, newImpulseUpperLimit);
                    deltaLambdaUpper = newImpulseUpperLimit - currentImpulseUpperLimit;

                    const glm::vec3 linearImpulseBody1 = deltaLambdaUpper * sliderAxis;
                    const glm::vec3 angularImpulseBody1 = deltaLambdaUpper * r1PlusUCrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

                    const glm::vec3 linearImpulseBody2 = -deltaLambdaUpper * sliderAxis;
                    const glm::vec3 angularImpulseBody2 = -deltaLambdaUpper * r2CrossSliderAxis;
                    _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
                }
            }

            // --------------- Motor --------------- //

            if (_sliderJointStore.IsMotorEnabledAtIndex(i)) {
                // J*v: relative velocity along the slider axis; the motor drives this towards the target speed.
                const f32 JvMotor = glm::dot(sliderAxis, v1) - glm::dot(sliderAxis, v2);

                // Solve for the incremental impulse and clamp the accumulated motor impulse to the maximum
                // impulse the motor can deliver this timestep (max force x dt), in either direction - unlike
                // the limits above, the motor is bilateral (it can push or pull).
                const f32 maxMotorImpulse = _sliderJointStore.GetMaxMotorForceAtIndex(i) * timestep.GetSeconds();
                f32 deltaLambdaMotor = _sliderJointStore.GetInverseMassMatrixMotorAtIndex(i) * (-JvMotor - _sliderJointStore.GetMotorSpeedAtIndex(i));
                const f32 currentImpulseMotor = _sliderJointStore.GetImpulseMotorAtIndex(i);
                const f32 newImpulseMotor = std::clamp(currentImpulseMotor + deltaLambdaMotor, -maxMotorImpulse, maxMotorImpulse);
                _sliderJointStore.SetImpulseMotorAtIndex(i, newImpulseMotor);
                deltaLambdaMotor = newImpulseMotor - currentImpulseMotor;

                // Apply the impulse along the slider axis to both bodies; a pure slide has no angular component.
                const glm::vec3 linearImpulseBody1 = deltaLambdaMotor * sliderAxis;
                _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1);

                const glm::vec3 linearImpulseBody2 = -deltaLambdaMotor * sliderAxis;
                _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
            }

            // --------------- Rotation Constraints --------------- //

            // J*v for the 3 rotation-locking constraints: the two bodies' angular velocities should match, so
            // that the relative orientation stays fixed at its initial value (see InitializeBeforeSolving's
            // qError bias computation). This is a bilateral (equality) constraint, so no clamping is needed.
            const glm::vec3 JvRotation = w2 - w1;
            const glm::vec3 deltaLambdaRotation =
                _sliderJointStore.GetInverseMassRotationMatrixAtIndex(i) * (-JvRotation - _sliderJointStore.GetRotationBiasAtIndex(i));
            _sliderJointStore.SetImpulseRotationAtIndex(i, _sliderJointStore.GetImpulseRotationAtIndex(i) + deltaLambdaRotation);

            // Apply the equal-and-opposite angular impulse to both bodies. angularImpulseBody1/2 are reused
            // (reassigned) below for the translation constraints' impulses rather than redeclared.
            glm::vec3 angularImpulseBody1 = -deltaLambdaRotation;
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

            glm::vec3 angularImpulseBody2 = deltaLambdaRotation;
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));

            // --------------- Translation Constraints --------------- //

            // J*v for the 2-DOF translation constraint: the anchor points' relative velocity projected onto
            // n1/n2, the plane orthogonal to the slider axis (zero once the anchors stay aligned along the
            // axis). Also a bilateral constraint - the joint resists drift off-axis in either direction.
            const f32 el1 = -glm::dot(n1, v1) - glm::dot(w1, r1PlusUCrossN1) + glm::dot(n1, v2) + glm::dot(w2, r2CrossN1);
            const f32 el2 = -glm::dot(n2, v1) - glm::dot(w1, r1PlusUCrossN2) + glm::dot(n2, v2) + glm::dot(w2, r2CrossN2);
            const glm::vec2 JvTranslation(el1, el2);

            // Solve the 2x2 system for the incremental impulse and accumulate it.
            const glm::vec2 deltaLambdaTranslation =
                _sliderJointStore.GetInverseMassTranslationMatrixAtIndex(i) * (-JvTranslation - _sliderJointStore.GetTranslationBiasAtIndex(i));
            _sliderJointStore.SetImpulseTranslationAtIndex(i, _sliderJointStore.GetImpulseTranslationAtIndex(i) + deltaLambdaTranslation);

            // Apply the impulse P = J^T * lambda to body 1, and the equal-and-opposite impulse to body 2.
            const glm::vec3 linearImpulseBody1 = -n1 * deltaLambdaTranslation.x - n2 * deltaLambdaTranslation.y;
            angularImpulseBody1 = -r1PlusUCrossN1 * deltaLambdaTranslation.x - r1PlusUCrossN2 * deltaLambdaTranslation.y;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

            const glm::vec3 linearImpulseBody2 = -linearImpulseBody1;
            angularImpulseBody2 = r2CrossN1 * deltaLambdaTranslation.x + r2CrossN2 * deltaLambdaTranslation.y;
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, v2 + invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
        }
    }

    void SliderJointSolverSystem::SolvePositionConstraint() {
        // One non-linear Gauss-Seidel (NGS) position-correction sweep per active slider joint. Unlike the
        // velocity solver, this corrects position/orientation directly rather than through velocities, and is
        // invoked multiple times per step (see PhysicsWorld::solvePositionCorrection) - so every quantity that
        // depends on orientation must be re-derived here from the *current* constrained orientation on every
        // call, rather than reusing values cached during InitializeBeforeSolving/SolveVelocityConstraint.
        VASSERT(_jointIndices.size() == _sliderJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolvePositionConstraint().");

        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
            const size_t jointIndex = _jointIndices[i].JointIndex;

            // Position correction technique is a per-joint setting, so joints that don't use NGS are skipped
            // individually here rather than aborting the whole sweep.
            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::vec3 &x1 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);
            const glm::quat &q1 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &q2 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            // Recompute the world-space inverse inertia tensors from the current orientation - by the time
            // this runs, the bodies have already been integrated forward (and, on later NGS iterations,
            // corrected further), so the tensors cached earlier in the step no longer reflect the true orientation.
            const glm::vec3 &invI1Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &invI2Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            glm::mat3 invI1;
            glm::mat3 invI2;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q1), invI1Local, invI1);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q2), invI2Local, invI2);

            _sliderJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _sliderJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            // Re-derive the lever arms (anchor point relative to center of mass) in world space from the
            // current orientation, exactly as InitializeBeforeSolving does at the start of the step. This is
            // essential here: by the time SolvePositionConstraint runs, the bodies have already integrated
            // (and, across NGS iterations, been corrected) past the orientation R1World/R2World were cached
            // for, so simply reusing those cached values would feed every Jacobian below a stale lever arm.
            const glm::vec3 &localAnchorPointBodyOne = _sliderJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _sliderJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);
            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            const glm::vec3 r1 = q1 * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 r2 = q2 * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);
            _sliderJointStore.SetR1WorldAtIndex(i, r1);
            _sliderJointStore.SetR2WorldAtIndex(i, r2);

            // u is the world-space separation between the anchor points (zero once the translation constraint
            // is satisfied).
            const glm::vec3 u = x2 + r2 - x1 - r1;

            // Refresh the slider axis and its orthogonal basis (n1/n2) from the current orientation too, for
            // the same reason r1/r2 needed refreshing above.
            const glm::vec3 sliderAxis = glm::normalize(q1 * _sliderJointStore.GetSliderAxisInBodyOneLocalSpaceAtIndex(i));
            _sliderJointStore.SetSliderAxisInWorldSpaceAtIndex(i, sliderAxis);

            const glm::vec3 n1 = GetOrthogonalUnitVector(sliderAxis);
            const glm::vec3 n2 = glm::cross(sliderAxis, n1);

            _sliderJointStore.SetN1AtIndex(i, n1);
            _sliderJointStore.SetN2AtIndex(i, n2);

            // Re-check whether the lower/upper limit constraints (translation along the slider axis) are
            // currently violated, using the freshly-updated anchor separation.
            const f32 uDotSliderAxis = glm::dot(u, sliderAxis);
            const f32 lowerLimitError = uDotSliderAxis - _sliderJointStore.GetLowerLimitAtIndex(i);
            const f32 upperLimitError = _sliderJointStore.GetUpperLimitAtIndex(i) - uDotSliderAxis;
            const bool lowerLimitViolated = lowerLimitError <= 0;
            const bool upperLimitViolated = upperLimitError <= 0;

            _sliderJointStore.SetIsLowerLimitViolatedAtIndex(i, lowerLimitViolated);
            _sliderJointStore.SetIsUpperLimitViolatedAtIndex(i, upperLimitViolated);

            // Precomputed Jacobian lever-arm terms, reused below by the Limits and Translation sections.
            const glm::vec3 r2CrossN1 = glm::cross(r2, n1);
            const glm::vec3 r2CrossN2 = glm::cross(r2, n2);
            const glm::vec3 r2CrossSliderAxis = glm::cross(r2, sliderAxis);

            _sliderJointStore.SetR2CrossN1AtIndex(i, r2CrossN1);
            _sliderJointStore.SetR2CrossN2AtIndex(i, r2CrossN2);
            _sliderJointStore.SetR2CrossSliderAxisAtIndex(i, r2CrossSliderAxis);

            const glm::vec3 r1PlusU = r1 + u;
            const glm::vec3 r1PlusUCrossN1 = glm::cross(r1PlusU, n1);
            const glm::vec3 r1PlusUCrossN2 = glm::cross(r1PlusU, n2);
            const glm::vec3 r1PlusUCrossSliderAxis = glm::cross(r1PlusU, sliderAxis);

            _sliderJointStore.SetR1PlusUCrossN1AtIndex(i, r1PlusUCrossN1);
            _sliderJointStore.SetR1PlusUCrossN2AtIndex(i, r1PlusUCrossN2);
            _sliderJointStore.SetR1PlusUCrossSliderAxisAtIndex(i, r1PlusUCrossSliderAxis);

            // --------------- Limits Constraints --------------- //

            if (_sliderJointStore.IsLimitEnabledAtIndex(i)) {
                // Compute the inverse of the 1x1 mass matrix K = M1^-1 + M2^-1 + (angular terms) for the
                // limit constraint, shared by whichever of the lower/upper checks below is violated.
                f32 invKLimit = f32(0.0);

                if (lowerLimitViolated || upperLimitViolated) {
                    const f32 kLimit =
                        sumInvMass + glm::dot(r1PlusUCrossSliderAxis, invI1 * r1PlusUCrossSliderAxis) + glm::dot(r2CrossSliderAxis, invI2 * r2CrossSliderAxis);
                    invKLimit = kLimit > f32(0.0) ? f32(1.0) / kLimit : f32(0.0);
                    _sliderJointStore.SetInverseMassMatrixLimitAtIndex(i, invKLimit);
                }

                // Directly solve for the impulse that fully cancels the (already-negative) lower limit error -
                // unlike the velocity solver, there's no running impulse to accumulate/clamp here, since
                // position correction resolves the error in a single shot each NGS iteration.
                if (lowerLimitViolated) {
                    const f32 lambdaLowerLimit = invKLimit * (-lowerLimitError);

                    // Apply the correction P = J^T * lambda to body 1, and the equal-and-opposite correction to body 2.
                    const glm::vec3 linearImpulseBody1 = -lambdaLowerLimit * sliderAxis;
                    const glm::vec3 angularImpulseBody1 = -lambdaLowerLimit * r1PlusUCrossSliderAxis;
                    const glm::vec3 v1 = invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                    const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                    const glm::vec3 linearImpulseBody2 = lambdaLowerLimit * sliderAxis;
                    const glm::vec3 angularImpulseBody2 = lambdaLowerLimit * r2CrossSliderAxis;
                    const glm::vec3 v2 = invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
                    const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
                }

                // Mirrors the lower limit above with the Jacobian sign flipped.
                if (upperLimitViolated) {
                    const f32 lambdaUpperLimit = invKLimit * (-upperLimitError);

                    const glm::vec3 linearImpulseBody1 = lambdaUpperLimit * sliderAxis;
                    const glm::vec3 angularImpulseBody1 = lambdaUpperLimit * r1PlusUCrossSliderAxis;
                    const glm::vec3 v1 = invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                    const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                    const glm::vec3 linearImpulseBody2 = -lambdaUpperLimit * sliderAxis;
                    const glm::vec3 angularImpulseBody2 = -lambdaUpperLimit * r2CrossSliderAxis;
                    const glm::vec3 v2 = invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
                    const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                    _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
                }
            }

            // --------------- Rotation Constraints --------------- //

            // Compute the mass matrix K = I1 + I2 (3x3) for the 3 rotation constraints, then its inverse,
            // leaving it zeroed for a singular or fully non-dynamic body pair.
            const glm::mat3 kRotation = invI1 + invI2;
            glm::mat3 invKRotation(0);
            _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, invKRotation);
            const f32 kRotationDet = glm::determinant(kRotation);

            if (VE_MACHINE_EPSILON < std::abs(kRotationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    invKRotation = InverseMat3(kRotation, kRotationDet);
                    _sliderJointStore.SetInverseMassRotationMatrixAtIndex(i, invKRotation);
                }

                // Same small-angle orientation-error approximation the velocity solver's Baumgarte bias uses,
                // but here it directly drives a one-shot position/orientation correction rather than a bias term.
                const glm::quat qError = q2 * _sliderJointStore.GetInitialOrientationDifferenceInverseAtIndex(i) * glm::inverse(q1);
                const glm::vec3 errorRotation = f32(2.0) * glm::vec3(qError.x, qError.y, qError.z);
                const glm::vec3 lambdaRotation = invKRotation * (-errorRotation);

                const glm::vec3 angularImpulseBody1 = -lambdaRotation;
                const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                const glm::vec3 angularImpulseBody2 = lambdaRotation;
                const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
            }

            // --------------- Translation Constraints --------------- //

            // Compute the mass matrix K = J*M^-1*J^t (2x2) for the 2 translation constraints, then its inverse.
            // K is symmetric (el21 == el12) because the inverse inertia tensors are symmetric, so only three
            // of the four elements need computing.
            const glm::vec3 invI1R1PlusUCrossN1 = invI1 * r1PlusUCrossN1;
            const glm::vec3 invI1R1PlusUCrossN2 = invI1 * r1PlusUCrossN2;
            const glm::vec3 invI2R2CrossN1 = invI2 * r2CrossN1;
            const glm::vec3 invI2R2CrossN2 = invI2 * r2CrossN2;
            const f32 el11 = sumInvMass + glm::dot(r1PlusUCrossN1, invI1R1PlusUCrossN1) + glm::dot(r2CrossN1, invI2R2CrossN1);
            const f32 el12 = glm::dot(r1PlusUCrossN1, invI1R1PlusUCrossN2) + glm::dot(r2CrossN1, invI2R2CrossN2);
            const f32 el22 = sumInvMass + glm::dot(r1PlusUCrossN2, invI1R1PlusUCrossN2) + glm::dot(r2CrossN2, invI2R2CrossN2);
            const glm::mat2 kTranslation(el11, el12, el12, el22);

            glm::mat2 invKTranslation(0);
            _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, invKTranslation);
            const f32 kTranslationDet = glm::determinant(kTranslation);

            // Skip the inverse (leave it zeroed) if the mass matrix is singular or both bodies are non-dynamic.
            if (VE_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    invKTranslation = InverseMat2(kTranslation, kTranslationDet);
                    _sliderJointStore.SetInverseMassTranslationMatrixAtIndex(i, invKTranslation);
                }

                // Position error for the 2 translation constraints: how far the anchor points have drifted
                // off the slider axis, projected onto the n1/n2 plane.
                const glm::vec2 errorTranslation(glm::dot(u, n1), glm::dot(u, n2));
                const glm::vec2 lambdaTranslation = invKTranslation * (-errorTranslation);

                // Apply the correction P = J^T * lambda to body 1, and the equal-and-opposite correction to body 2.
                const glm::vec3 linearImpulseBody1 = -n1 * lambdaTranslation.x - n2 * lambdaTranslation.y;
                const glm::vec3 angularImpulseBody1 = -r1PlusUCrossN1 * lambdaTranslation.x - r1PlusUCrossN2 * lambdaTranslation.y;
                const glm::vec3 v1 = invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5)));

                const glm::vec3 linearImpulseBody2 = n1 * lambdaTranslation.x + n2 * lambdaTranslation.y;
                const glm::vec3 angularImpulseBody2 = r2CrossN1 * lambdaTranslation.x + r2CrossN2 * lambdaTranslation.y;
                const glm::vec3 v2 = invM2 * linearLockAxisFactorBodyTwo * linearImpulseBody2;
                const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5)));
            }
        }
    }

} // namespace Vulkyrie
