#include "physics/systems/dynamics_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    DynamicsSystem::DynamicsSystem(PhysicsWorld &physicsWorld, bool &enableGravity, glm::vec3 &gravity)
        : _colliderComponentStore(physicsWorld.GetColliderComponentStore())
        , _rigidBodyComponentStore(physicsWorld.GetRigidBodyComponentStore())
        , _transformComponentStore(physicsWorld.GetTransformComponentStore())
        , _enableGravity(enableGravity)
        , _gravity(gravity) {
    }

    // Integrates constrained velocities (plus optional split-impulse velocities) into new constrained
    // positions and orientations using first-order symplectic Euler integration.
    //
    // Position:    p' = p + v * dt
    // Orientation: q' = normalize(q + 0.5 * dt * ω_q * q)   where ω_q = glm::quat(0, ω)
    //
    // The results are stored as constrained values and are not yet visible to other systems;
    // call UpdateStates() to commit them to the live body and transform state.
    void DynamicsSystem::IntegratePositions(Timestep timeStep, bool isSplitImpulseActive) {
        // When split-impulse correction is active, blend the split velocity into the integration
        // velocity so that penetration is resolved through position correction rather than by
        // altering the primary velocity state.
        const f32 splitImpulseFactor = isSplitImpulseActive ? f32(1.0) : f32(0.0);
        const size_t activeBodyCount = _rigidBodyComponentStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeBodyCount; ++i) {
            // Start from the post-solver constrained velocities and optionally add split-impulse correction.
            glm::vec3 newLinearVelocity = _rigidBodyComponentStore.GetConstrainedLinearVelocityAtIndex(i);
            glm::vec3 newAngularVelocity = _rigidBodyComponentStore.GetConstrainedAngularVelocityAtIndex(i);

            newLinearVelocity += splitImpulseFactor * _rigidBodyComponentStore.GetSplitLinearVelocityAtIndex(i);
            newAngularVelocity += splitImpulseFactor * _rigidBodyComponentStore.GetSplitAngularVelocityAtIndex(i);

            // Read the pre-step position and orientation (committed by the previous step's UpdateStates).
            const glm::vec3 currentPosition = _rigidBodyComponentStore.GetWorldCenterOfMassAtIndex(i);
            const glm::quat currentOrientation = _transformComponentStore.GetTransform(_rigidBodyComponentStore.GetEntityAtIndex(i)).Rotation;

            // Integrate position: p' = p + v * dt.
            const glm::vec3 newPosition = currentPosition + newLinearVelocity * timeStep.GetSeconds();

            // Integrate orientation using the quaternion derivative: dq/dt = 0.5 * ω_q * q.
            // First-order approximation: q' = normalize(q + 0.5 * dt * ω_q * q).
            const glm::quat newOrientation =
                glm::normalize(currentOrientation + 0.5F * glm::quat(0.0F, newAngularVelocity) * currentOrientation * timeStep.GetSeconds());

            _rigidBodyComponentStore.SetConstrainedPositionAtIndex(i, newPosition);
            _rigidBodyComponentStore.SetConstrainedOrientationAtIndex(i, newOrientation);
        }
    }

    // Computes constrained velocities for the current step by integrating forces, applying gravity,
    // and applying velocity damping — all in a single pass over the active rigid bodies.
    //
    // Force/torque integration (Newton's 2nd law):
    //   v'  = v  + dt * (1/m) * lockFactor * F
    //   ω'  = ω  + dt * lockFactor * I⁻¹ * τ
    //
    // Gravity (uniform acceleration; mass terms cancel so only the lock factor scales g):
    //   Δv  = dt * lockFactor * g
    //
    // Velocity damping (Padé approximation of the exact e^(-c*dt) exponential decay):
    //   v'' = v' * 1 / (1 + c * dt)
    //
    // All three stages are merged into one loop to avoid write-read roundtrips on the
    // constrained velocity arrays that would occur with separate loops.
    void DynamicsSystem::IntegrateVelocities(Timestep timeStep) {
        // Split velocities must be zeroed at the start of each step; they are written
        // by the contact solver and read by IntegratePositions for penetration correction.
        ResetSplitVelocities();

        const f32 dt = timeStep.GetSeconds();
        const size_t activeComponentCount = _rigidBodyComponentStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeComponentCount; ++i) {
            VASSERT(_rigidBodyComponentStore.GetSplitLinearVelocityAtIndex(i) == glm::vec3(0.0F),
                    "Split linear velocity was not reset to zero before integrating velocities.");
            VASSERT(_rigidBodyComponentStore.GetSplitAngularVelocityAtIndex(i) == glm::vec3(0.0F),
                    "Split angular velocity was not reset to zero before integrating velocities.");

            // Stage 1 — integrate external forces and torques.
            // v' = v + dt * (1/m) * lockFactor * F
            glm::vec3 newLinearVelocity = _rigidBodyComponentStore.GetLinearVelocityAtIndex(i) +
                                          dt * _rigidBodyComponentStore.GetInverseMassAtIndex(i) * _rigidBodyComponentStore.GetLinearLockAxisFactorAtIndex(i) *
                                              _rigidBodyComponentStore.GetExternalForceAtIndex(i);

            // ω' = ω + dt * lockFactor * I⁻¹ * τ
            glm::vec3 newAngularVelocity =
                _rigidBodyComponentStore.GetAngularVelocityAtIndex(i) +
                dt * _rigidBodyComponentStore.GetAngularLockAxisFactorAtIndex(i) *
                    (_rigidBodyComponentStore.GetInverseWorldInertiaTensorAtIndex(i) * _rigidBodyComponentStore.GetExternalTorqueAtIndex(i));

            // Stage 2 — apply gravity.
            // Because gravity is a uniform acceleration (F = mg), the mass terms cancel:
            // Δv = dt * (1/m) * lockFactor * (m * g) = dt * lockFactor * g.
            if (_enableGravity && _rigidBodyComponentStore.IsGravityEnabledAtIndex(i)) {
                newLinearVelocity += dt * _rigidBodyComponentStore.GetLinearLockAxisFactorAtIndex(i) * _gravity;
            }

            // Stage 3 — apply velocity damping.
            // Exact solution: v(t+dt) = v(t) * e^(-c*dt).
            // Padé approximant (first-order): e^(-c*dt) ≈ 1 / (1 + c*dt).
            newLinearVelocity *= f32(1.0) / (f32(1.0) + _rigidBodyComponentStore.GetLinearDampingAtIndex(i) * dt);
            newAngularVelocity *= f32(1.0) / (f32(1.0) + _rigidBodyComponentStore.GetAngularDampingAtIndex(i) * dt);

            _rigidBodyComponentStore.SetConstrainedLinearVelocityAtIndex(i, newLinearVelocity);
            _rigidBodyComponentStore.SetConstrainedAngularVelocityAtIndex(i, newAngularVelocity);
        }
    }

    // Commits the constrained state computed during this step (positions, orientations, velocities) back
    // to the live body and collider stores so that the rest of the engine sees the updated transforms.
    //
    // Body update (per active rigid body):
    //   1. Live velocities  <- constrained velocities  (post-solver)
    //   2. World center of mass <- constrained position
    //   3. Transform rotation   <- normalize(constrained orientation)
    //   4. Transform origin     <- worldCoM - rotation * localCoM
    //
    // Collider update (per active collider):
    //   localToWorld = bodyTransform * localToBodyTransform
    void DynamicsSystem::UpdateStates() {
        const size_t activeBodyCount = _rigidBodyComponentStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeBodyCount; ++i) {
            // Update the linear and angular velocities of the body to match the constrained velocities computed by the constraint solver.
            _rigidBodyComponentStore.SetLinearVelocityAtIndex(i, _rigidBodyComponentStore.GetConstrainedLinearVelocityAtIndex(i));
            _rigidBodyComponentStore.SetAngularVelocityAtIndex(i, _rigidBodyComponentStore.GetConstrainedAngularVelocityAtIndex(i));

            // Update the world center of mass of the body to match the constrained position computed by the constraint solver.
            // This ensures that any rendering or other systems that rely on the world center of mass will reflect the
            // position changes caused by constraints such as contacts and joints.
            _rigidBodyComponentStore.SetWorldCenterOfMassAtIndex(i, _rigidBodyComponentStore.GetConstrainedPositionAtIndex(i));

            // Update the transform component of the body to match the constrained orientation and world center of mass.
            // This ensures that the body's visual representation will reflect the orientation changes caused by constraints
            // such as contacts and joints, and that the position of the transform will be consistent with the position of the center of mass.
            const glm::quat &constrainedOrientation = _rigidBodyComponentStore.GetConstrainedOrientationAtIndex(i);
            TransformComponent &transform = _transformComponentStore.GetTransform(_rigidBodyComponentStore.GetEntityAtIndex(i));

            const glm::vec3 &centerOfMassWorld = _rigidBodyComponentStore.GetWorldCenterOfMassAtIndex(i);
            const glm::vec3 &centerOfMassLocal = _rigidBodyComponentStore.GetLocalCenterOfMassAtIndex(i);

            transform.Rotation = glm::normalize(constrainedOrientation);
            transform.Position = centerOfMassWorld - transform.Rotation * centerOfMassLocal;
        }

        // Update the local-to-world transforms of all colliders based on the updated transforms of their associated bodies.
        // This ensures that the colliders will be correctly positioned and oriented for collision detection and response in the next simulation step.
        const size_t activeColliderComponentCount = _colliderComponentStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeColliderComponentCount; i++) {
            const Entity bodyEntity = _colliderComponentStore.GetBodyEntityAtIndex(i);
            const TransformComponent transform = _transformComponentStore.GetTransform(bodyEntity) * _colliderComponentStore.GetLocalToBodyTransformAtIndex(i);

            _colliderComponentStore.SetLocalToWorldTransformAtIndex(i, transform);
        }
    }

    // Clears external forces and torques on all rigid bodies (active and inactive) so that forces
    // applied during a step do not persist into subsequent steps unless explicitly reapplied.
    void DynamicsSystem::ResetForcesAndTorques() {
        const size_t totalComponentCount = _rigidBodyComponentStore.GetTotalComponentCount();

        for (size_t i = 0; i < totalComponentCount; ++i) {
            _rigidBodyComponentStore.SetExternalForceAtIndex(i, glm::vec3(0.0F));
            _rigidBodyComponentStore.SetExternalTorqueAtIndex(i, glm::vec3(0.0F));
        }
    }

    // Zeroes the split-impulse velocities for all active rigid bodies.
    // Split velocities are a separate channel used solely for position-level penetration correction.
    // They are cleared here at the start of IntegrateVelocities(), before the contact solver runs,
    // so that split impulses written by the previous step's solver do not carry over.
    // Called automatically at the start of IntegrateVelocities().
    void DynamicsSystem::ResetSplitVelocities() {
        const size_t activeComponentCount = _rigidBodyComponentStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeComponentCount; ++i) {
            _rigidBodyComponentStore.SetSplitLinearVelocityAtIndex(i, glm::vec3(0.0F));
            _rigidBodyComponentStore.SetSplitAngularVelocityAtIndex(i, glm::vec3(0.0F));
        }
    }

} // namespace Vulkyrie
