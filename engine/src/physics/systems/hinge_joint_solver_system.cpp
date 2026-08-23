#include "physics/systems/hinge_joint_solver_system.h"
#include "physics/physics_world.h"
#include "physics/body/rigid_body.h"
#include "core/asserts.h"
#include "core/utilities.h"

namespace Vulkyrie {

    HingeJointSolverSystem::HingeJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _hingeJointStore(world.GetHingeJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void HingeJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
        // For each active hinge joint, precompute the solver state that stays constant across every
        // velocity-solver iteration of this step: lever arms, mass matrices, and bias terms.
        const size_t activeJointCount = _hingeJointStore.GetActiveComponentCount();
        _jointIndices.resize(activeJointCount);

        for (size_t i = 0; i < activeJointCount; ++i) {
            const Entity jointEntity = _hingeJointStore.GetEntityAtIndex(i);
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
            _hingeJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _hingeJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            const glm::quat &q1 = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &q2 = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _hingeJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _hingeJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the lever arms (anchor point relative to center of mass) in world space.
            const glm::vec3 r1 = q1 * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 r2 = q2 * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _hingeJointStore.SetR1WorldAtIndex(i, r1);
            _hingeJointStore.SetR2WorldAtIndex(i, r2);

            // Compute the hinge axis of each body in world space; a1 is the reference axis, a2 is only used
            // to build the b2/c2 basis that spans the plane perpendicular to it.
            const glm::vec3 a1 = glm::normalize(q1 * _hingeJointStore.GetHingeAxisInBodyOneLocalSpaceAtIndex(i));
            const glm::vec3 a2 = glm::normalize(q2 * _hingeJointStore.GetHingeAxisInBodyTwoLocalSpaceAtIndex(i));

            _hingeJointStore.SetHingeAxisWorldSpaceAtIndex(i, a1);

            // b2 and c2 span the plane orthogonal to a2; the two rotation constraints keep a1 aligned with a2
            // by driving dot(a1, b2) and dot(a1, c2) to zero, so their Jacobians are built from b2/c2 x a1.
            const glm::vec3 b2 = GetOrthogonalUnitVector(a2);
            const glm::vec3 c2 = glm::cross(a2, b2);
            const glm::vec3 b2CrossA1 = glm::cross(b2, a1);
            const glm::vec3 c2CrossA1 = glm::cross(c2, a1);

            _hingeJointStore.SetB2CrossA1AtIndex(i, b2CrossA1);
            _hingeJointStore.SetC2CrossA1AtIndex(i, c2CrossA1);

            // Compute the Baumgarte bias "b" for the 2 rotation constraints from the current axis misalignment.
            if (baumgartePositionCorrection) {
                _hingeJointStore.SetRotationBiasAtIndex(i, biasFactor * glm::vec2(glm::dot(a1, b2), glm::dot(a1, c2)));
            } else {
                _hingeJointStore.SetRotationBiasAtIndex(i, glm::vec2(0));
            }

            // Skew-symmetric matrices of the lever arms, used to express the angular block of the
            // translation constraint's Jacobian as a matrix multiplication (see SkewSymmetric).
            const glm::mat3 skewR1 = SkewSymmetric(r1);
            const glm::mat3 skewR2 = SkewSymmetric(r2);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            // Compute the mass matrix K = J*M^-1*J^t (3x3) for the 3 translation constraints, then its inverse.
            const glm::mat3 kTranslation = glm::mat3(sumInvMass) + skewR1 * invI1 * glm::transpose(skewR1) + skewR2 * invI2 * glm::transpose(skewR2);

            _hingeJointStore.SetInverseMassTranslationMatrixAtIndex(i, glm::mat3(0));
            const f32 kTranslationDet = glm::determinant(kTranslation);

            // Skip the inverse (leave it zeroed) if the mass matrix is singular or both bodies are non-dynamic;
            // a singular K means the constraint is degenerate and has no unique impulse solution this step.
            if (VE_K_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _hingeJointStore.SetInverseMassTranslationMatrixAtIndex(i, InverseMat3(kTranslation, kTranslationDet));
                }
            }

            const glm::vec3 &x1 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &x2 = _rigidBodyStore.GetWorldCenterOfMassAtIndex(bodyTwoIndex);

            // Compute the Baumgarte bias "b" for the 3 translation constraints from the current anchor-point
            // separation (x2 + r2 should coincide with x1 + r1 when the constraint is satisfied).
            if (baumgartePositionCorrection) {
                _hingeJointStore.SetTranslationBiasAtIndex(i, biasFactor * (x2 + r2 - x1 - r1));
            } else {
                _hingeJointStore.SetTranslationBiasAtIndex(i, glm::vec3(0));
            }

            // Compute the mass matrix K = J*M^-1*J^t (2x2) for the 2 rotation constraints, then its inverse.
            // K is symmetric (el21 == el12) because the inverse inertia tensors are symmetric, so only three
            // of the four elements need computing.
            const glm::vec3 invI1B2CrossA1 = invI1 * b2CrossA1;
            const glm::vec3 invI1C2CrossA1 = invI1 * c2CrossA1;
            const glm::vec3 invI2B2CrossA1 = invI2 * b2CrossA1;
            const glm::vec3 invI2C2CrossA1 = invI2 * c2CrossA1;
            const f32 el11 = glm::dot(b2CrossA1, invI1B2CrossA1) + glm::dot(b2CrossA1, invI2B2CrossA1);
            const f32 el12 = glm::dot(b2CrossA1, invI1C2CrossA1) + glm::dot(b2CrossA1, invI2C2CrossA1);
            const f32 el22 = glm::dot(c2CrossA1, invI1C2CrossA1) + glm::dot(c2CrossA1, invI2C2CrossA1);

            const glm::mat2 kRotation(el11, el12, el12, el22);
            const f32 kRotationDet = glm::determinant(kRotation);

            _hingeJointStore.SetInverseMassRotationMatrixAtIndex(i, glm::mat2(0));

            // Same singular/non-dynamic guard as the translation mass matrix above.
            if (VE_K_MACHINE_EPSILON < std::abs(kRotationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    _hingeJointStore.SetInverseMassRotationMatrixAtIndex(i, InverseMat2(kRotation, kRotationDet));
                }
            }

            // If warm-starting is disabled, discard every impulse accumulated last step so this step's
            // velocity solver starts from zero instead of re-applying stale impulses.
            if (!_enableWarmStartup) {
                _hingeJointStore.SetImpulseTranslationAtIndex(i, glm::vec3(0.0f));
                _hingeJointStore.SetImpulseRotationAtIndex(i, glm::vec2(0.0f));
                _hingeJointStore.SetImpulseLowerLimitAtIndex(i, 0.0f);
                _hingeJointStore.SetImpulseUpperLimitAtIndex(i, 0.0f);
                _hingeJointStore.SetImpulseMotorAtIndex(i, 0.0f);
            }

            // Determine whether the lower/upper limit constraints are currently violated. A limit's
            // accumulated impulse is reset whenever it stops being violated or flips violation state,
            // since a stale impulse from a different regime would bias the new solve.
            const f32 hingeAngle = ComputeCurrentHingeAngleAtIndex(i, q1, q2);
            const f32 lowerLimitError = hingeAngle - _hingeJointStore.GetLowerLimitAtIndex(i);
            const f32 upperLimitError = _hingeJointStore.GetUpperLimitAtIndex(i) - hingeAngle;
            const bool oldIsLowerLimitViolated = _hingeJointStore.GetIsLowerLimitViolatedAtIndex(i);
            const bool isLowerLimitViolated = lowerLimitError <= 0;

            _hingeJointStore.SetIsLowerLimitViolatedAtIndex(i, isLowerLimitViolated);
            if (!isLowerLimitViolated || isLowerLimitViolated != oldIsLowerLimitViolated) {
                _hingeJointStore.SetImpulseLowerLimitAtIndex(i, 0.0f);
            }

            const bool oldIsUpperLimitViolated = _hingeJointStore.GetIsUpperLimitViolatedAtIndex(i);
            const bool isUpperLimitViolated = upperLimitError <= 0;

            _hingeJointStore.SetIsUpperLimitViolatedAtIndex(i, isUpperLimitViolated);
            if (!isUpperLimitViolated || isUpperLimitViolated != oldIsUpperLimitViolated) {
                _hingeJointStore.SetImpulseUpperLimitAtIndex(i, 0.0f);
            }

            const bool limitEnabled = _hingeJointStore.IsLimitEnabledAtIndex(i);

            // The motor and limit constraints share the same 1-DOF Jacobian (the hinge axis a1), so they
            // share the same inverse mass matrix K^-1 = 1 / (a1 . I1*a1 + a1 . I2*a1). Only compute it when
            // at least one of them can actually apply an impulse this step.
            if (_hingeJointStore.IsMotorEnabledAtIndex(i) || (limitEnabled && (isLowerLimitViolated || isUpperLimitViolated))) {
                const f32 kLimitMotor = glm::dot(a1, invI1 * a1) + glm::dot(a1, invI2 * a1);
                const f32 invKLimitMotor = (kLimitMotor > f32(0.0)) ? f32(1.0) / kLimitMotor : f32(0.0);

                _hingeJointStore.SetInverseMassMatrixLimitMotorAtIndex(i, invKLimitMotor);

                if (limitEnabled) {
                    // Compute the Baumgarte bias "b" for whichever limit constraint is active.
                    if (baumgartePositionCorrection) {
                        _hingeJointStore.SetBiasLowerLimitAtIndex(i, biasFactor * lowerLimitError);
                        _hingeJointStore.SetBiasUpperLimitAtIndex(i, biasFactor * upperLimitError);
                    } else {
                        _hingeJointStore.SetBiasLowerLimitAtIndex(i, 0.0f);
                        _hingeJointStore.SetBiasUpperLimitAtIndex(i, 0.0f);
                    }
                }
            }
        }
    }

    void HingeJointSolverSystem::WarmStart() {
        // Re-apply the impulses accumulated in the previous step as an initial guess, so the velocity solver
        // starts close to the solution and converges in fewer iterations. Body 1 receives -P and body 2 receives +P.
        VASSERT(_jointIndices.size() == _hingeJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before WarmStart().");

        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Get the inverse mass of the bodies.
            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Get the accumulated translation, rotation, limit and motor impulses to re-apply.
            const glm::vec3 &impulseTranslation = _hingeJointStore.GetImpulseTranslationAtIndex(i);
            const glm::vec2 &impulseRotation = _hingeJointStore.GetImpulseRotationAtIndex(i);

            const f32 impulseLowerLimit = _hingeJointStore.GetImpulseLowerLimitAtIndex(i);
            const f32 impulseUpperLimit = _hingeJointStore.GetImpulseUpperLimitAtIndex(i);

            const glm::mat3 &invI1 = _hingeJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _hingeJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::vec3 &b2CrossA1 = _hingeJointStore.GetB2CrossA1AtIndex(i);
            const glm::vec3 &a1 = _hingeJointStore.GetHingeAxisWorldSpaceAtIndex(i);

            // Compute the impulse P=J^T * lambda for the 2 rotation constraints.
            glm::vec3 rotationImpulse = -b2CrossA1 * impulseRotation.x - _hingeJointStore.GetC2CrossA1AtIndex(i) * impulseRotation.y;

            // Compute the impulse P=J^T * lambda for the lower and upper limit constraints.
            const glm::vec3 limitsImpulse = (impulseUpperLimit - impulseLowerLimit) * a1;

            // Compute the impulse P=J^T * lambda for the motor constraint.
            const glm::vec3 motorImpulse = -_hingeJointStore.GetImpulseMotorAtIndex(i) * a1;

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 1.
            glm::vec3 linearImpulseBody1 = -impulseTranslation;
            glm::vec3 angularImpulseBody1 = glm::cross(impulseTranslation, _hingeJointStore.GetR1WorldAtIndex(i));

            // Add the rotation, limit and motor impulse contributions for body 1.
            angularImpulseBody1 += rotationImpulse;
            angularImpulseBody1 += limitsImpulse;
            angularImpulseBody1 += motorImpulse;

            // Apply the impulse to body 1.
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            const glm::vec3 newW1 = w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 2.
            glm::vec3 angularImpulseBody2 = glm::cross(-impulseTranslation, _hingeJointStore.GetR2WorldAtIndex(i));

            // Add the rotation, limit and motor impulse contributions for body 2 (equal and opposite to body 1's).
            angularImpulseBody2 += -rotationImpulse;
            angularImpulseBody2 += -limitsImpulse;
            angularImpulseBody2 += -motorImpulse;

            // Apply the impulse to body 2.
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * impulseTranslation;
            const glm::vec3 newW2 = w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
        }
    }

    void HingeJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
        // Solve the hinge joint's constraints via sequential impulses, in the order limit -> motor ->
        // rotation -> translation. Each block below both reads and (via Set...) writes the constrained
        // velocities, and the v1/v2/w1/w2 references stay bound to the same storage slots for the
        // whole iteration, so every later block observes the velocity changes made by the earlier ones -
        // that ordering dependency is what makes this "sequential impulses" rather than one-shot impulses.
        VASSERT(_jointIndices.size() == _hingeJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolveVelocityConstraint().");

        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            const glm::vec3 &v1 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &v2 = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &w1 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &w2 = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            const glm::mat3 &invI1 = _hingeJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &invI2 = _hingeJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::vec3 &r1 = _hingeJointStore.GetR1WorldAtIndex(i);
            const glm::vec3 &r2 = _hingeJointStore.GetR2WorldAtIndex(i);

            const glm::vec3 &a1 = _hingeJointStore.GetHingeAxisWorldSpaceAtIndex(i);

            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            // Shared 1x1 inverse mass matrix for the limit and motor constraints (both are 1-DOF,
            // driven by the same Jacobian a1), precomputed in InitializeBeforeSolving.
            const f32 invKLimitMotor = _hingeJointStore.GetInverseMassMatrixLimitMotorAtIndex(i);

            // --------------- Limits Constraints --------------- //

            if (_hingeJointStore.IsLimitEnabledAtIndex(i)) {
                // Lower limit is a one-sided constraint (hingeAngle >= lowerLimit), so its accumulated
                // impulse is clamped to stay non-negative; it can only push the angle up, never pull it down.
                if (_hingeJointStore.GetIsLowerLimitViolatedAtIndex(i)) {
                    const f32 JvLowerLimit = glm::dot(w2 - w1, a1);
                    f32 deltaLambdaLower = invKLimitMotor * (-JvLowerLimit - _hingeJointStore.GetBiasLowerLimitAtIndex(i));
                    const f32 currentImpulseLowerLimit = _hingeJointStore.GetImpulseLowerLimitAtIndex(i);
                    const f32 newImpulseLowerLimit = std::max(currentImpulseLowerLimit + deltaLambdaLower, f32(0.0));

                    _hingeJointStore.SetImpulseLowerLimitAtIndex(i, newImpulseLowerLimit);
                    // Re-derive deltaLambda from the clamped impulse so the velocity update below reflects
                    // only the portion of the impulse that was actually applied, not the unclamped solve.
                    deltaLambdaLower = newImpulseLowerLimit - currentImpulseLowerLimit;

                    const glm::vec3 angularImpulseBody1 = -deltaLambdaLower * a1;
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

                    const glm::vec3 angularImpulseBody2 = deltaLambdaLower * a1;
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
                }

                // Upper limit (hingeAngle <= upperLimit) is the mirror image of the lower limit: the
                // Jacobian and impulse signs are flipped, but it's likewise clamped to stay non-negative.
                if (_hingeJointStore.GetIsUpperLimitViolatedAtIndex(i)) {
                    const f32 JvUpperLimit = -glm::dot(w2 - w1, a1);
                    f32 deltaLambdaUpper = invKLimitMotor * (-JvUpperLimit - _hingeJointStore.GetBiasUpperLimitAtIndex(i));
                    const f32 currentImpulseUpperLimit = _hingeJointStore.GetImpulseUpperLimitAtIndex(i);
                    const f32 newImpulseUpperLimit = std::max(currentImpulseUpperLimit + deltaLambdaUpper, f32(0.0));

                    _hingeJointStore.SetImpulseUpperLimitAtIndex(i, newImpulseUpperLimit);
                    deltaLambdaUpper = newImpulseUpperLimit - currentImpulseUpperLimit;

                    const glm::vec3 angularImpulseBody1 = deltaLambdaUpper * a1;
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

                    const glm::vec3 angularImpulseBody2 = -deltaLambdaUpper * a1;
                    _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
                }
            }

            // --------------- Motor --------------- //

            if (_hingeJointStore.IsMotorEnabledAtIndex(i)) {
                // The motor drives the relative angular velocity around a1 toward -motorSpeed, clamped to
                // the torque the motor can exert in one timestep (impulse = torque * dt).
                const f32 JvMotor = glm::dot(a1, w1 - w2);
                const f32 maxMotorImpulse = _hingeJointStore.GetMaxMotorTorqueAtIndex(i) * timestep.GetSeconds();
                f32 deltaLambdaMotor = invKLimitMotor * (-JvMotor - _hingeJointStore.GetMotorSpeedAtIndex(i));
                const f32 currentMotorImpulse = _hingeJointStore.GetImpulseMotorAtIndex(i);
                const f32 newMotorImpulse = std::clamp(currentMotorImpulse + deltaLambdaMotor, -maxMotorImpulse, maxMotorImpulse);

                _hingeJointStore.SetImpulseMotorAtIndex(i, newMotorImpulse);
                deltaLambdaMotor = newMotorImpulse - currentMotorImpulse;

                const glm::vec3 angularImpulseBody1 = -deltaLambdaMotor * a1;
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

                const glm::vec3 angularImpulseBody2 = deltaLambdaMotor * a1;
                _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));
            }

            // --------------- Joint Rotation Constraints --------------- //

            // Keeps a1 aligned with a2 by driving dot(a1,b2) and dot(a1,c2) back to zero (2 equality
            // constraints); b2CrossA1/c2CrossA1 are the precomputed Jacobian lever-arm terms.
            const glm::vec3 &b2CrossA1 = _hingeJointStore.GetB2CrossA1AtIndex(i);
            const glm::vec3 &c2CrossA1 = _hingeJointStore.GetC2CrossA1AtIndex(i);
            const glm::vec3 relativeAngularVelocity = w2 - w1;
            const glm::vec2 JvRotation(glm::dot(b2CrossA1, relativeAngularVelocity), glm::dot(c2CrossA1, relativeAngularVelocity));
            glm::vec2 deltaLambdaRotation =
                _hingeJointStore.GetInverseMassRotationMatrixAtIndex(i) * (-JvRotation - _hingeJointStore.GetRotationBiasAtIndex(i));

            // Equality constraint: the impulse is applied and accumulated in full, with no clamping.
            _hingeJointStore.SetImpulseRotationAtIndex(i, _hingeJointStore.GetImpulseRotationAtIndex(i) + deltaLambdaRotation);

            glm::vec3 angularImpulseBody1 = -b2CrossA1 * deltaLambdaRotation.x - c2CrossA1 * deltaLambdaRotation.y;
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1));

            glm::vec3 angularImpulseBody2 = b2CrossA1 * deltaLambdaRotation.x + c2CrossA1 * deltaLambdaRotation.y;
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2));

            // --------------- Joint Translation Constraints --------------- //

            // Keeps the two anchor points coincident (3 equality constraints): drives the relative
            // velocity of the anchor points, v2 + w2 x r2 - v1 - w1 x r1, to zero.
            const glm::vec3 JvTranslation = v2 + glm::cross(w2, r2) - v1 - glm::cross(w1, r1);
            const glm::vec3 deltaLambdaTranslation =
                _hingeJointStore.GetInverseMassTranslationMatrixAtIndex(i) * (-JvTranslation - _hingeJointStore.GetTranslationBiasAtIndex(i));
            _hingeJointStore.SetImpulseTranslationAtIndex(i, _hingeJointStore.GetImpulseTranslationAtIndex(i) + deltaLambdaTranslation);

            // Apply the impulse to body 1 (linear -P, angular r1 x P) and update its constrained velocities.
            const glm::vec3 linearImpulseBody1 = -deltaLambdaTranslation;
            angularImpulseBody1 = glm::cross(deltaLambdaTranslation, r1);
            const glm::vec3 newV1 = v1 + invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
            const glm::vec3 newW1 = w1 + angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newV1);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newW1);

            // Apply the equal-and-opposite impulse to body 2 (linear +P, angular -r2 x P).
            angularImpulseBody2 = -glm::cross(deltaLambdaTranslation, r2);
            const glm::vec3 newV2 = v2 + invM2 * linearLockAxisFactorBodyTwo * deltaLambdaTranslation;
            const glm::vec3 newW2 = w2 + angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newV2);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newW2);
        }
    }

    void HingeJointSolverSystem::SolvePositionConstraint() {
        // Non-linear Gauss-Seidel position correction: for each hinge joint, directly move the two bodies'
        // positions and orientations to remove the drift that remains after the velocity solver - a violated
        // angle limit, hinge axes that fell out of alignment, and anchor-point separation. Because the geometry
        // changes as the bodies move, the lever arms, hinge-axis basis and mass matrices are recomputed here
        // from the current state rather than reusing the cached solver values.
        VASSERT(_jointIndices.size() == _hingeJointStore.GetActiveComponentCount(), "InitializeBeforeSolving() must run before SolvePositionConstraint().");

        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            const size_t jointIndex = _jointIndices[i].JointIndex;

            // This solver only runs for joints configured to use non-linear Gauss-Seidel position correction.
            if (JointsPositionCorrectionTechnique::NonLinearGaussSeidel != _jointStore.GetJointsPositionCorrectionTechniqueAtIndex(jointIndex)) {
                continue;
            }

            const size_t bodyOneIndex = _jointIndices[i].BodyOneIndex;
            const size_t bodyTwoIndex = _jointIndices[i].BodyTwoIndex;

            // Get the constrained (in-progress) orientations; the references stay bound to the store's slots,
            // so each constraint block below observes the orientation updates made by the earlier ones.
            const glm::quat &q1 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyOneIndex);
            const glm::quat &q2 = _rigidBodyStore.GetConstrainedOrientationAtIndex(bodyTwoIndex);

            // Get the (inverse) local-space inertia tensors, used to rebuild the world-space inertia tensors below.
            const glm::vec3 &invI1Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyOneIndex);
            const glm::vec3 &invI2Local = _rigidBodyStore.GetInverseLocalInertiaTensorAtIndex(bodyTwoIndex);

            // Per-axis factors that zero out the correction on locked/frozen degrees of freedom.
            const glm::vec3 &linearLockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &linearLockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);

            // Recompute the world-space inverse inertia tensors from the current orientations, which have changed
            // since InitializeBeforeSolving as earlier position-correction iterations rotated the bodies.
            glm::mat3 invI1;
            glm::mat3 invI2;

            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q1), invI1Local, invI1);
            RigidBody::ComputeWorldSpaceInertiaTensorInverse(glm::mat3_cast(q2), invI2Local, invI2);

            _hingeJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, invI1);
            _hingeJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, invI2);

            // Recompute the world-space lever arms (centre of mass -> anchor point) from the current orientations.
            const glm::vec3 r1 =
                q1 * (_hingeJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex));
            const glm::vec3 r2 =
                q2 * (_hingeJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i) - _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex));

            _hingeJointStore.SetR1WorldAtIndex(i, r1);
            _hingeJointStore.SetR2WorldAtIndex(i, r2);

            // Recompute the world-space hinge axes and the b2/c2 basis spanning the plane orthogonal to a2;
            // b2CrossA1 and c2CrossA1 are the Jacobian lever-arm terms of the two axis-alignment constraints
            // (see InitializeBeforeSolving).
            const glm::vec3 a1 = glm::normalize(q1 * _hingeJointStore.GetHingeAxisInBodyOneLocalSpaceAtIndex(i));
            const glm::vec3 a2 = glm::normalize(q2 * _hingeJointStore.GetHingeAxisInBodyTwoLocalSpaceAtIndex(i));
            const glm::vec3 b2 = GetOrthogonalUnitVector(a2);
            const glm::vec3 c2 = glm::cross(a2, b2);
            const glm::vec3 b2CrossA1 = glm::cross(b2, a1);
            const glm::vec3 c2CrossA1 = glm::cross(c2, a1);

            _hingeJointStore.SetHingeAxisWorldSpaceAtIndex(i, a1);
            _hingeJointStore.SetB2CrossA1AtIndex(i, b2CrossA1);
            _hingeJointStore.SetC2CrossA1AtIndex(i, c2CrossA1);

            // Re-evaluate the hinge angle and the limit-violation states from the current orientations.
            const f32 hingeAngle = ComputeCurrentHingeAngleAtIndex(i, q1, q2);
            const f32 lowerLimitError = hingeAngle - _hingeJointStore.GetLowerLimitAtIndex(i);
            const f32 upperLimitError = _hingeJointStore.GetUpperLimitAtIndex(i) - hingeAngle;
            const bool lowerLimitViolated = lowerLimitError <= 0;
            const bool upperLimitViolated = upperLimitError <= 0;
            _hingeJointStore.SetIsLowerLimitViolatedAtIndex(i, lowerLimitViolated);
            _hingeJointStore.SetIsUpperLimitViolatedAtIndex(i, upperLimitViolated);

            // --------------- Limits Constraints --------------- //

            if (_hingeJointStore.IsLimitEnabledAtIndex(i)) {
                // Compute the 1-DOF limit mass matrix K^-1 = 1 / (a1 . I1*a1 + a1 . I2*a1) from the
                // recomputed inertia tensors, shared by whichever of the lower/upper corrections below applies.
                f32 invKLimitMotor = f32(0.0);

                if (lowerLimitViolated || upperLimitViolated) {
                    const f32 kLimitMotor = glm::dot(a1, invI1 * a1) + glm::dot(a1, invI2 * a1);
                    invKLimitMotor = (kLimitMotor > f32(0.0)) ? f32(1.0) / kLimitMotor : f32(0.0);
                    _hingeJointStore.SetInverseMassMatrixLimitMotorAtIndex(i, invKLimitMotor);
                }

                // The lower limit is violated: rotate the bodies in opposite directions around a1 by an amount
                // proportional to the angle error to push the hinge angle back above the limit. Each orientation
                // update integrates the pseudo angular velocity via the quaternion derivative q += 0.5 * (0,w) * q.
                if (lowerLimitViolated) {
                    const f32 lambdaLowerLimit = invKLimitMotor * (-lowerLimitError);

                    const glm::vec3 angularImpulseBody1 = -lambdaLowerLimit * a1;
                    const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                    const glm::quat newQ1 = glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, newQ1);

                    const glm::vec3 angularImpulseBody2 = lambdaLowerLimit * a1;
                    const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                    const glm::quat newQ2 = glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, newQ2);
                }

                // The upper limit is the mirror image of the lower limit: the correction signs are flipped so
                // the bodies rotate the opposite way around a1 to bring the hinge angle back below the limit.
                if (upperLimitViolated) {
                    const f32 lambdaUpperLimit = invKLimitMotor * (-upperLimitError);

                    const glm::vec3 angularImpulseBody1 = lambdaUpperLimit * a1;
                    const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                    const glm::quat newQ1 = glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, newQ1);

                    const glm::vec3 angularImpulseBody2 = -lambdaUpperLimit * a1;
                    const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                    const glm::quat newQ2 = glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5));
                    _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, newQ2);
                }
            }

            // --------------- Rotation Constraints --------------- //

            // Compute the mass matrix K = J*M^-1*J^t (2x2) for the 2 rotation constraints, then its inverse.
            // K is symmetric (el21 == el12) because the inverse inertia tensors are symmetric, so only three
            // of the four elements need computing.
            const glm::vec3 invI1B2CrossA1 = invI1 * b2CrossA1;
            const glm::vec3 invI1C2CrossA1 = invI1 * c2CrossA1;
            const glm::vec3 invI2B2CrossA1 = invI2 * b2CrossA1;
            const glm::vec3 invI2C2CrossA1 = invI2 * c2CrossA1;
            const f32 el11 = glm::dot(b2CrossA1, invI1B2CrossA1) + glm::dot(b2CrossA1, invI2B2CrossA1);
            const f32 el12 = glm::dot(b2CrossA1, invI1C2CrossA1) + glm::dot(b2CrossA1, invI2C2CrossA1);
            const f32 el22 = glm::dot(c2CrossA1, invI1C2CrossA1) + glm::dot(c2CrossA1, invI2C2CrossA1);
            const glm::mat2 kRotation(el11, el12, el12, el22);

            // Skip the correction entirely if the mass matrix is singular; for a fully non-dynamic body pair
            // the inverse stays zeroed instead, which makes lambda - and thus the correction - zero below.
            glm::mat2 invKRotation(0.0f);
            _hingeJointStore.SetInverseMassRotationMatrixAtIndex(i, invKRotation);
            const f32 kRotationDet = glm::determinant(kRotation);

            if (VE_K_MACHINE_EPSILON < std::abs(kRotationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    invKRotation = InverseMat2(kRotation, kRotationDet);
                    _hingeJointStore.SetInverseMassRotationMatrixAtIndex(i, invKRotation);
                }

                // The rotation error is the current misalignment of the hinge axes: dot(a1,b2) and dot(a1,c2)
                // are both zero exactly when a1 is aligned with a2. Solve K * lambda = -C and rotate the two
                // bodies in opposite directions to remove the error.
                const glm::vec2 errorRotation = glm::vec2(glm::dot(a1, b2), glm::dot(a1, c2));
                const glm::vec2 lambdaRotation = invKRotation * (-errorRotation);
                const glm::vec3 angularImpulseBody1 = -b2CrossA1 * lambdaRotation.x - c2CrossA1 * lambdaRotation.y;
                const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                const glm::quat newQ1 = glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5));
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, newQ1);

                const glm::vec3 angularImpulseBody2 = b2CrossA1 * lambdaRotation.x + c2CrossA1 * lambdaRotation.y;
                const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                const glm::quat newQ2 = glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5));
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, newQ2);
            }

            // --------------- Translation Constraints --------------- //

            // Skew-symmetric matrices so that a cross product r x v becomes the matrix product [r]x * v.
            const glm::mat3 skewR1 = SkewSymmetric(r1);
            const glm::mat3 skewR2 = SkewSymmetric(r2);

            const f32 invM1 = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 invM2 = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);
            const f32 sumInvMass = invM1 + invM2;

            // Compute the mass matrix K = J*M^-1*J^t (3x3) for the 3 translation constraints, then its inverse.
            const glm::mat3 kTranslation = glm::mat3(sumInvMass) + skewR1 * invI1 * glm::transpose(skewR1) + skewR2 * invI2 * glm::transpose(skewR2);

            // Same singular/non-dynamic guard as the rotation mass matrix above.
            glm::mat3 invKTranslation(0.0f);
            _hingeJointStore.SetInverseMassTranslationMatrixAtIndex(i, invKTranslation);
            const f32 kTranslationDet = glm::determinant(kTranslation);

            if (VE_K_MACHINE_EPSILON < std::abs(kTranslationDet)) {
                if (BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyOneIndex) ||
                    BodyType::Dynamic == _rigidBodyStore.GetBodyTypeAtIndex(bodyTwoIndex)) {
                    invKTranslation = InverseMat3(kTranslation, kTranslationDet);
                    _hingeJointStore.SetInverseMassTranslationMatrixAtIndex(i, invKTranslation);
                }

                // Measure the current separation of the anchor points (the constraint error C) and solve
                // K * lambda = -C for the corrective position impulse.
                const glm::vec3 &x1 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyOneIndex);
                const glm::vec3 &x2 = _rigidBodyStore.GetConstrainedPositionAtIndex(bodyTwoIndex);

                const glm::vec3 errorTranslation = x2 + r2 - x1 - r1;
                const glm::vec3 lambdaTranslation = invKTranslation * (-errorTranslation);

                // Apply the impulse to body 1 (linear -P, angular r1 x P) as pseudo velocities, integrating its
                // position directly and its orientation via the quaternion derivative q += 0.5 * (0,w) * q.
                const glm::vec3 linearImpulseBody1 = -lambdaTranslation;
                const glm::vec3 angularImpulseBody1 = glm::cross(lambdaTranslation, r1);
                const glm::vec3 v1 = invM1 * linearLockAxisFactorBodyOne * linearImpulseBody1;
                const glm::vec3 w1 = angularLockAxisFactorBodyOne * (invI1 * angularImpulseBody1);
                const glm::quat newQ1 = glm::normalize(q1 + glm::quat(0, w1) * q1 * f32(0.5));
                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyOneIndex, x1 + v1);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyOneIndex, newQ1);

                // Apply the equal-and-opposite impulse to body 2 (linear +P, angular -r2 x P).
                const glm::vec3 angularImpulseBody2 = -glm::cross(lambdaTranslation, r2);
                const glm::vec3 v2 = invM2 * linearLockAxisFactorBodyTwo * lambdaTranslation;
                const glm::vec3 w2 = angularLockAxisFactorBodyTwo * (invI2 * angularImpulseBody2);
                const glm::quat newQ2 = glm::normalize(q2 + glm::quat(0, w2) * q2 * f32(0.5));
                _rigidBodyStore.SetConstrainedPositionAtIndex(bodyTwoIndex, x2 + v2);
                _rigidBodyStore.SetConstrainedOrientationAtIndex(bodyTwoIndex, newQ2);
            }
        }
    }

    f32 HingeJointSolverSystem::ComputeCurrentHingeAngleAtIndex(size_t jointComponentIndex,
                                                                const glm::quat &bodyOneOrientation,
                                                                const glm::quat &bodyTwoOrientation) {
        f32 hingeAngle;

        // Compute the current orientation difference between the two bodies
        const glm::quat currentOrientationDiff = glm::normalize(bodyTwoOrientation * glm::inverse(bodyOneOrientation));

        // Compute the relative rotation considering the initial orientation difference
        const glm::quat relativeRotation =
            glm::normalize(currentOrientationDiff * _hingeJointStore.GetInitialOrientationDifferenceInverseAtIndex(jointComponentIndex));

        // A quaternion q = [cos(theta/2); sin(theta/2) * rotAxis] where rotAxis is a unit
        // length vector. We can extract cos(theta/2) with q.w and we can extract |sin(theta/2)| with :
        // |sin(theta/2)| = q.getVectorV().length() since rotAxis is unit length. Note that any
        // rotation can be represented by a quaternion q and -q. Therefore, if the relative rotation
        // axis is not pointing in the same direction as the hinge axis, we use the rotation -q which
        // has the same |sin(theta/2)| value but the value cos(theta/2) is sign inverted. Some details
        // about this trick is explained in the source code of OpenTissue (http://www.opentissue.org).
        const f32 cosHalfAngle = relativeRotation.w;
        const glm::vec3 relativeRotationToVec3 = glm::vec3(relativeRotation.x, relativeRotation.y, relativeRotation.z);
        const f32 sinHalfAngleAbs = glm::length(relativeRotationToVec3);

        // Compute the dot product of the relative rotation axis and the hinge axis
        const f32 dotProduct = glm::dot(relativeRotationToVec3, _hingeJointStore.GetHingeAxisWorldSpaceAtIndex(jointComponentIndex));

        // If the relative rotation axis and the hinge axis are pointing the same direction
        if (dotProduct >= f32(0.0)) {
            hingeAngle = f32(2.0) * std::atan2(sinHalfAngleAbs, cosHalfAngle);
        } else {
            hingeAngle = f32(2.0) * std::atan2(sinHalfAngleAbs, -cosHalfAngle);
        }

        // Convert the angle from range [0; 2*pi] into the range [-pi; pi]
        hingeAngle = computeNormalizedAngle(hingeAngle);

        // Compute and return the corresponding angle near one of the two limits
        return computeCorrespondingAngleNearLimits(
            hingeAngle, _hingeJointStore.GetLowerLimitAtIndex(jointComponentIndex), _hingeJointStore.GetUpperLimitAtIndex(jointComponentIndex));
    }

    f32 HingeJointSolverSystem::computeNormalizedAngle(f32 angle) const {
        // Convert it into the range [-2*pi; 2*pi]
        angle = std::fmod(angle, TWICE_PI);

        // Convert it into the range [-pi; pi]
        if (angle < -std::numbers::pi_v<f32>) {
            return angle + TWICE_PI;
        } else if (angle > std::numbers::pi_v<f32>) {
            return angle - TWICE_PI;
        } else {
            return angle;
        }
    }

    f32 HingeJointSolverSystem::computeCorrespondingAngleNearLimits(f32 inputAngle, f32 lowerLimitAngle, f32 upperLimitAngle) const {
        // If the limits are degenerate (no effective limit range), the input angle already is the answer.
        if (upperLimitAngle <= lowerLimitAngle) {
            return inputAngle;
        } else if (inputAngle > upperLimitAngle) {
            // The angle lies above the upper limit; find whether it is closer (modulo 2*pi) to the upper or
            // lower limit, and shift it down by a full turn if that puts it nearer the lower limit instead.
            f32 diffToUpperLimit = std::abs(computeNormalizedAngle(inputAngle - upperLimitAngle));
            f32 diffToLowerLimit = std::abs(computeNormalizedAngle(inputAngle - lowerLimitAngle));

            return (diffToUpperLimit > diffToLowerLimit) ? (inputAngle - TWICE_PI) : inputAngle;
        } else if (inputAngle < lowerLimitAngle) {
            // The angle lies below the lower limit; symmetric case to the branch above, shifting up by a
            // full turn if that puts it nearer the upper limit instead.
            f32 diffToUpperLimit = std::abs(computeNormalizedAngle(upperLimitAngle - inputAngle));
            f32 diffToLowerLimit = std::abs(computeNormalizedAngle(lowerLimitAngle - inputAngle));

            return (diffToUpperLimit > diffToLowerLimit) ? inputAngle : (inputAngle + TWICE_PI);
        } else {
            // The angle already lies within the limits.
            return inputAngle;
        }
    }

} // namespace Vulkyrie
