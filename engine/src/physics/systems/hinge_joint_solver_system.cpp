#include "physics/systems/hinge_joint_solver_system.h"
#include "physics/physics_world.h"
#include "core/utilities.h"

namespace Vulkyrie {

    HingeJointSolverSystem::HingeJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _hingeJointStore(world.GetHingeJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void HingeJointSolverSystem::InitializeBeforeSolving([[maybe_unused]] f32 biasFactor) {
        // For each hinge joint, precompute the solver state that stays constant across every velocity-solver
        // iteration of this step: the world-space inertia tensors and lever arms.
        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _hingeJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            // Get the two bodies constrained by this joint and their rigid-body component indices.
            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            VASSERT(!_rigidBodyStore.IsDisabled(bodyOneEntity) || !_rigidBodyStore.IsDisabled(bodyTwoEntity), "Both bodies must be active.");

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            // Cache the world-space inverse inertia tensors of both bodies for use in the mass matrices.
            _hingeJointStore.SetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyOneIndex));
            _hingeJointStore.SetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i, _rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(bodyTwoIndex));

            const glm::quat &orientationBodyOne = _transformStore.GetTransform(bodyOneEntity).Rotation;
            const glm::quat &orientationBodyTwo = _transformStore.GetTransform(bodyTwoEntity).Rotation;

            const glm::vec3 &localAnchorPointBodyOne = _hingeJointStore.GetLocalSpaceAnchorPointOnBodyOneAtIndex(i);
            const glm::vec3 &localAnchorPointBodyTwo = _hingeJointStore.GetLocalSpaceAnchorPointOnBodyTwoAtIndex(i);

            const glm::vec3 &localCenterOfMassBodyOne = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyOneIndex);
            const glm::vec3 &localCenterOfMassBodyTwo = _rigidBodyStore.GetLocalCenterOfMassAtIndex(bodyTwoIndex);

            const glm::vec3 rOneWorld = orientationBodyOne * (localAnchorPointBodyOne - localCenterOfMassBodyOne);
            const glm::vec3 rTwoWorld = orientationBodyTwo * (localAnchorPointBodyTwo - localCenterOfMassBodyTwo);

            _hingeJointStore.SetR1WorldAtIndex(i, rOneWorld);
            _hingeJointStore.SetR2WorldAtIndex(i, rTwoWorld);

            const glm::vec3 a1 = glm::normalize(orientationBodyOne * _hingeJointStore.GetHingeAxisInBodyOneLocalSpaceAtIndex(i));
            const glm::vec3 a2 = glm::normalize(orientationBodyTwo * _hingeJointStore.GetHingeAxisInBodyTwoLocalSpaceAtIndex(i));

            _hingeJointStore.SetHingeAxisWorldSpaceAtIndex(i, a1);

            const glm::vec3 b2 = GetOrthogonalUnitVector(a2);
            const glm::vec3 c2 = glm::cross(a2, b2);

            _hingeJointStore.SetB2CrossA1AtIndex(i, glm::cross(b2, a1));
            _hingeJointStore.SetC2CrossA1AtIndex(i, glm::cross(c2, a1));

            // TODO: compute the hinge axis in world space (a1), the b2 x a1 / c2 x a1 vectors, the translation and
            // rotation bias terms, the inverse mass matrices for translation/rotation/limits/motor, the current hinge
            // angle and limit-violation state, and reset the accumulated impulses when warm-starting is disabled
            // (see ReactPhysics3D::initBeforeSolve).
        }
    }

    void HingeJointSolverSystem::WarmStart() {
        // Re-apply the impulses accumulated in the previous step as an initial guess, so the velocity solver
        // starts close to the solution and converges in fewer iterations. Body 1 receives -P and body 2 receives +P.
        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            const Entity jointEntity = _hingeJointStore.GetEntityAtIndex(i);
            const size_t jointIndex = _jointStore.GetEntityIndex(jointEntity);

            const Entity bodyOneEntity = _jointStore.GetBodyOneEntityAtIndex(jointIndex);
            const Entity bodyTwoEntity = _jointStore.GetBodyTwoEntityAtIndex(jointIndex);

            const size_t bodyOneIndex = _rigidBodyStore.GetEntityIndex(bodyOneEntity);
            const size_t bodyTwoIndex = _rigidBodyStore.GetEntityIndex(bodyTwoEntity);

            // Get the current constrained velocities of the bodies.
            const glm::vec3 &linearVelocityBodyOne = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &linearVelocityBodyTwo = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(bodyTwoIndex);
            const glm::vec3 &angularVelocityBodyOne = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyOneIndex);
            const glm::vec3 &angularVelocityBodyTwo = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(bodyTwoIndex);

            // Get the inverse mass of the bodies.
            const f32 inverseMassBodyOne = _rigidBodyStore.GetInverseMassAtIndex(bodyOneIndex);
            const f32 inverseMassBodyTwo = _rigidBodyStore.GetInverseMassAtIndex(bodyTwoIndex);

            // Get the accumulated translation, rotation, limit and motor impulses to re-apply.
            const glm::vec3 &impulseTranslation = _hingeJointStore.GetImpulseTranslationAtIndex(i);
            const glm::vec2 &impulseRotation = _hingeJointStore.GetImpulseRotationAtIndex(i);

            const f32 impulseLowerLimit = _hingeJointStore.GetImpulseLowerLimitAtIndex(i);
            const f32 impulseUpperLimit = _hingeJointStore.GetImpulseUpperLimitAtIndex(i);

            const glm::mat3 &inertiaTensorBodyOne = _hingeJointStore.GetInertiaTensorOfBodyOneInWorldSpaceAtIndex(i);
            const glm::mat3 &inertiaTensorBodyTwo = _hingeJointStore.GetInertiaTensorOfBodyTwoInWorldSpaceAtIndex(i);

            const glm::vec3 &b2CrossA1 = _hingeJointStore.GetB2CrossA1AtIndex(i);
            const glm::vec3 &hingeAxisInWorldSpace = _hingeJointStore.GetHingeAxisWorldSpaceAtIndex(i);

            // Compute the impulse P=J^T * lambda for the 2 rotation constraints.
            glm::vec3 rotationImpulse = -b2CrossA1 * impulseRotation.x - _hingeJointStore.GetC2CrossA1AtIndex(i) * impulseRotation.y;

            // Compute the impulse P=J^T * lambda for the lower and upper limit constraints.
            const glm::vec3 limitsImpulse = (impulseUpperLimit - impulseLowerLimit) * hingeAxisInWorldSpace;

            // Compute the impulse P=J^T * lambda for the motor constraint.
            const glm::vec3 motorImpulse = -_hingeJointStore.GetImpulseMotorAtIndex(i) * hingeAxisInWorldSpace;

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 1.
            glm::vec3 linearImpulseBodyOne = -impulseTranslation;
            glm::vec3 angularImpulseBodyOne = glm::cross(impulseTranslation, _hingeJointStore.GetR1WorldAtIndex(i));

            // Add the rotation, limit and motor impulse contributions for body 1.
            angularImpulseBodyOne += rotationImpulse;
            angularImpulseBodyOne += limitsImpulse;
            angularImpulseBodyOne += motorImpulse;

            // Apply the impulse to body 1.
            const glm::vec3 &linearlockAxisFactorBodyOne = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 &angularLockAxisFactorBodyOne = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyOneIndex);
            const glm::vec3 newLinearVelocityBodyOne = linearVelocityBodyOne + inverseMassBodyOne * linearlockAxisFactorBodyOne * linearImpulseBodyOne;
            const glm::vec3 newAngularVelocityBodyOne = angularVelocityBodyOne + angularLockAxisFactorBodyOne * (inertiaTensorBodyOne * angularImpulseBodyOne);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyOneIndex, newLinearVelocityBodyOne);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyOneIndex, newAngularVelocityBodyOne);

            // Compute the impulse P=J^T * lambda for the 3 translation constraints for body 2.
            glm::vec3 angularImpulseBodyTwo = glm::cross(-impulseTranslation, _hingeJointStore.GetR2WorldAtIndex(i));

            // Add the rotation, limit and motor impulse contributions for body 2 (equal and opposite to body 1's).
            angularImpulseBodyTwo += -rotationImpulse;
            angularImpulseBodyTwo += -limitsImpulse;
            angularImpulseBodyTwo += -motorImpulse;

            // Apply the impulse to body 2.
            const glm::vec3 &linearlockAxisFactorBodyTwo = _rigidBodyStore.GetLinearLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 &angularLockAxisFactorBodyTwo = _rigidBodyStore.GetAngularLockAxisFactorAtIndex(bodyTwoIndex);
            const glm::vec3 newLinearVelocityBodyTwo = linearVelocityBodyTwo + inverseMassBodyTwo * linearlockAxisFactorBodyTwo * impulseTranslation;
            const glm::vec3 newAngularVelocityBodyTwo = angularVelocityBodyTwo + angularLockAxisFactorBodyTwo * (inertiaTensorBodyTwo * angularImpulseBodyTwo);
            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(bodyTwoIndex, newLinearVelocityBodyTwo);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(bodyTwoIndex, newAngularVelocityBodyTwo);
        }
    }

    void HingeJointSolverSystem::SolveVelocityConstraint(Timestep timestep) {
        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            // TODO: solve the hinge joint's limit, motor, rotation and translation velocity constraints via
            // sequential impulses, in that order (see ReactPhysics3D::solveVelocityConstraint).

            (void)timestep;
        }
    }

    void HingeJointSolverSystem::SolvePositionConstraint() {
        for (size_t i = 0; i < _hingeJointStore.GetActiveComponentCount(); ++i) {
            // TODO: apply non-linear Gauss-Seidel position correction for the hinge joint's limit, rotation and
            // translation constraints, recomputing the lever arms and mass matrices from the current constrained
            // orientations/positions (see ReactPhysics3D::solvePositionConstraint).
        }
    }

    f32 HingeJointSolverSystem::ComputeCurrentHingeAngle(Entity jointEntity, const glm::quat &bodyOneOrientation, const glm::quat &bodyTwoOrientation) {
        f32 hingeAngle;

        // Compute the current orientation difference between the two bodies
        const glm::quat currentOrientationDiff = glm::normalize(bodyTwoOrientation * glm::inverse(bodyOneOrientation));

        // Compute the relative rotation considering the initial orientation difference
        const glm::quat relativeRotation = glm::normalize(currentOrientationDiff * _hingeJointStore.GetInitialOrientationDifferenceInverse(jointEntity));

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
        const f32 dotProduct = glm::dot(relativeRotationToVec3, _hingeJointStore.GetHingeAxisWorldSpace(jointEntity));

        // If the relative rotation axis and the hinge axis are pointing the same direction
        if (dotProduct >= f32(0.0)) {
            hingeAngle = f32(2.0) * std::atan2(sinHalfAngleAbs, cosHalfAngle);
        } else {
            hingeAngle = f32(2.0) * std::atan2(sinHalfAngleAbs, -cosHalfAngle);
        }

        // Convert the angle from range [0; 2*pi] into the range [-pi; pi]
        hingeAngle = computeNormalizedAngle(hingeAngle);

        // Compute and return the corresponding angle near one of the two limits
        return computeCorrespondingAngleNearLimits(hingeAngle, _hingeJointStore.GetLowerLimit(jointEntity), _hingeJointStore.GetUpperLimit(jointEntity));
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
