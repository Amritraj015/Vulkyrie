#include "physics/systems/slider_joint_solver_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    SliderJointSolverSystem::SliderJointSolverSystem(PhysicsWorld &world, bool &enableWarmStartup)
        : _rigidBodyStore(world.GetRigidBodyComponentStore())
        , _transformStore(world.GetTransformComponentStore())
        , _jointStore(world.GetJointComponentStore())
        , _sliderJointStore(world.GetSliderJointComponentStore())
        , _enableWarmStartup(enableWarmStartup) {
    }

    void SliderJointSolverSystem::InitializeBeforeSolving(f32 biasFactor) {
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
        }

        (void)_rigidBodyStore;
        (void)_transformStore;
        (void)_jointStore;
        (void)_sliderJointStore;
        (void)_enableWarmStartup;
        (void)biasFactor;
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
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
        }

        (void)timestep;
    }

    void SliderJointSolverSystem::SolvePositionConstraint() {
        for (size_t i = 0; i < _sliderJointStore.GetActiveComponentCount(); ++i) {
        }
    }

} // namespace Vulkyrie
