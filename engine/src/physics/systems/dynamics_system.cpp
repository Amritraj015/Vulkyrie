#include "physics/systems/dynamics_system.h"
#include "physics/physics_world.h"

namespace Vulkyrie {

    DynamicsSystem::DynamicsSystem(PhysicsWorld &physicsWorld, bool &enableGravity, glm::vec3 &gravity)
        : _colliderStore(physicsWorld.GetColliderComponentStore())
        , _rigidBodyStore(physicsWorld.GetRigidBodyComponentStore())
        , _transformStore(physicsWorld.GetTransformComponentStore())
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
        const size_t activeBodyCount = _rigidBodyStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeBodyCount; ++i) {
            // Start from the post-solver constrained velocities and optionally add split-impulse correction.
            glm::vec3 newLinearVelocity = _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(i);
            glm::vec3 newAngularVelocity = _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(i);

            newLinearVelocity += splitImpulseFactor * _rigidBodyStore.GetSplitLinearVelocityAtIndex(i);
            newAngularVelocity += splitImpulseFactor * _rigidBodyStore.GetSplitAngularVelocityAtIndex(i);

            // Read the pre-step position and orientation (committed by the previous step's UpdateStates).
            const glm::vec3 currentPosition = _rigidBodyStore.GetWorldCenterOfMassAtIndex(i);
            const glm::quat currentOrientation = _transformStore.GetTransform(_rigidBodyStore.GetEntityAtIndex(i)).Rotation;

            // Integrate position: p' = p + v * dt.
            const glm::vec3 newPosition = currentPosition + newLinearVelocity * timeStep.GetSeconds();

            // Integrate orientation using the quaternion derivative: dq/dt = 0.5 * ω_q * q.
            // First-order approximation: q' = normalize(q + 0.5 * dt * ω_q * q).
            const glm::quat newOrientation =
                glm::normalize(currentOrientation + 0.5F * glm::quat(0.0F, newAngularVelocity) * currentOrientation * timeStep.GetSeconds());

            _rigidBodyStore.SetConstrainedPositionAtIndex(i, newPosition);
            _rigidBodyStore.SetConstrainedOrientationAtIndex(i, newOrientation);
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
        const size_t activeComponentCount = _rigidBodyStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeComponentCount; ++i) {
            VASSERT(_rigidBodyStore.GetSplitLinearVelocityAtIndex(i) == glm::vec3(0.0F),
                    "Split linear velocity was not reset to zero before integrating velocities.");
            VASSERT(_rigidBodyStore.GetSplitAngularVelocityAtIndex(i) == glm::vec3(0.0F),
                    "Split angular velocity was not reset to zero before integrating velocities.");

            // Stage 1 — integrate external forces and torques.
            // v' = v + dt * (1/m) * lockFactor * F
            glm::vec3 newLinearVelocity = _rigidBodyStore.GetLinearVelocityAtIndex(i) + dt * _rigidBodyStore.GetInverseMassAtIndex(i) *
                                                                                            _rigidBodyStore.GetLinearLockAxisFactorAtIndex(i) *
                                                                                            _rigidBodyStore.GetExternalForceAtIndex(i);

            // ω' = ω + dt * lockFactor * I⁻¹ * τ
            glm::vec3 newAngularVelocity = _rigidBodyStore.GetAngularVelocityAtIndex(i) +
                                           dt * _rigidBodyStore.GetAngularLockAxisFactorAtIndex(i) *
                                               (_rigidBodyStore.GetInverseWorldInertiaTensorAtIndex(i) * _rigidBodyStore.GetExternalTorqueAtIndex(i));

            // Stage 2 — apply gravity.
            // Because gravity is a uniform acceleration (F = mg), the mass terms cancel:
            // Δv = dt * (1/m) * lockFactor * (m * g) = dt * lockFactor * g.
            if (_enableGravity && _rigidBodyStore.IsGravityEnabledAtIndex(i)) {
                newLinearVelocity += dt * _rigidBodyStore.GetLinearLockAxisFactorAtIndex(i) * _gravity;
            }

            // Stage 3 — apply velocity damping.
            // Exact solution: v(t+dt) = v(t) * e^(-c*dt).
            // Padé approximant (first-order): e^(-c*dt) ≈ 1 / (1 + c*dt).
            newLinearVelocity *= f32(1.0) / (f32(1.0) + _rigidBodyStore.GetLinearDampingAtIndex(i) * dt);
            newAngularVelocity *= f32(1.0) / (f32(1.0) + _rigidBodyStore.GetAngularDampingAtIndex(i) * dt);

            _rigidBodyStore.SetConstrainedLinearVelocityAtIndex(i, newLinearVelocity);
            _rigidBodyStore.SetConstrainedAngularVelocityAtIndex(i, newAngularVelocity);
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
        const size_t activeBodyCount = _rigidBodyStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeBodyCount; ++i) {
            // Update the linear and angular velocities of the body to match the constrained velocities computed by the constraint solver.
            _rigidBodyStore.SetLinearVelocityAtIndex(i, _rigidBodyStore.GetConstrainedLinearVelocityAtIndex(i));
            _rigidBodyStore.SetAngularVelocityAtIndex(i, _rigidBodyStore.GetConstrainedAngularVelocityAtIndex(i));

            // Update the world center of mass of the body to match the constrained position computed by the constraint solver.
            // This ensures that any rendering or other systems that rely on the world center of mass will reflect the
            // position changes caused by constraints such as contacts and joints.
            _rigidBodyStore.SetWorldCenterOfMassAtIndex(i, _rigidBodyStore.GetConstrainedPositionAtIndex(i));

            // Update the transform component of the body to match the constrained orientation and world center of mass.
            // This ensures that the body's visual representation will reflect the orientation changes caused by constraints
            // such as contacts and joints, and that the position of the transform will be consistent with the position of the center of mass.
            const glm::quat &constrainedOrientation = _rigidBodyStore.GetConstrainedOrientationAtIndex(i);
            TransformComponent &transform = _transformStore.GetTransform(_rigidBodyStore.GetEntityAtIndex(i));

            const glm::vec3 &centerOfMassWorld = _rigidBodyStore.GetWorldCenterOfMassAtIndex(i);
            const glm::vec3 &centerOfMassLocal = _rigidBodyStore.GetLocalCenterOfMassAtIndex(i);

            transform.Rotation = glm::normalize(constrainedOrientation);
            transform.Position = centerOfMassWorld - transform.Rotation * centerOfMassLocal;
        }

        // Update the local-to-world transforms of all colliders based on the updated transforms of their associated bodies.
        // This ensures that the colliders will be correctly positioned and oriented for collision detection and response in the next simulation step.
        const size_t activeColliderComponentCount = _colliderStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeColliderComponentCount; i++) {
            const Entity bodyEntity = _colliderStore.GetBodyEntityAtIndex(i);
            const TransformComponent transform = _transformStore.GetTransform(bodyEntity) * _colliderStore.GetLocalToBodyTransformAtIndex(i);

            _colliderStore.SetLocalToWorldTransformAtIndex(i, transform);
        }
    }

    // Clears external forces and torques on all rigid bodies (active and inactive) so that forces
    // applied during a step do not persist into subsequent steps unless explicitly reapplied.
    void DynamicsSystem::ResetForcesAndTorques() {
        const size_t totalComponentCount = _rigidBodyStore.GetTotalComponentCount();

        for (size_t i = 0; i < totalComponentCount; ++i) {
            _rigidBodyStore.SetExternalForceAtIndex(i, glm::vec3(0.0F));
            _rigidBodyStore.SetExternalTorqueAtIndex(i, glm::vec3(0.0F));
        }
    }

    // Zeroes the split-impulse velocities for all active rigid bodies.
    // Split velocities are a separate channel used solely for position-level penetration correction.
    // They are cleared here at the start of IntegrateVelocities(), before the contact solver runs,
    // so that split impulses written by the previous step's solver do not carry over.
    // Called automatically at the start of IntegrateVelocities().
    void DynamicsSystem::ResetSplitVelocities() {
        const size_t activeComponentCount = _rigidBodyStore.GetActiveComponentCount();

        for (size_t i = 0; i < activeComponentCount; ++i) {
            _rigidBodyStore.SetSplitLinearVelocityAtIndex(i, glm::vec3(0.0F));
            _rigidBodyStore.SetSplitAngularVelocityAtIndex(i, glm::vec3(0.0F));
        }
    }

} // namespace Vulkyrie
