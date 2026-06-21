#pragma once

#include "core/time_step.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class PhysicsWorld;

    /**
     * @brief Computes and applies Newtonian dynamics for all active rigid bodies each simulation step.
     *
     * The intended call order within a single physics step is:
     *   1. IntegrateVelocities  — integrate external forces/torques into constrained velocities,
     *                             then apply gravity and damping.
     *   2. <constraint / contact solver>  — correct the constrained velocities to satisfy constraints.
     *   3. IntegratePositions   — advance positions and orientations from the corrected velocities.
     *   4. UpdateStates         — commit constrained state back to the body and collider stores.
     *   5. ResetForcesAndTorques — clear accumulated forces/torques ready for the next step.
     *
     * Split-impulse position correction (used to resolve penetration without affecting velocities)
     * is supported by IntegratePositions via the @p isSplitImpulseActive flag.
     */
    class DynamicsSystem {
    public:
        /**
         * @brief Constructs a DynamicsSystem that operates on the stores owned by @p physicsWorld.
         * @param physicsWorld  The world whose component stores this system will read and write.
         * @param enableGravity Reference to the world-level gravity toggle; read each step.
         * @param gravity       Reference to the world gravity vector; read each step.
         */
        DynamicsSystem(PhysicsWorld &physicsWorld, bool &enableGravity, glm::vec3 &gravity);

        // Delete the copy constructor and copy assignment operator.
        DynamicsSystem(const DynamicsSystem &) = delete;
        DynamicsSystem &operator=(const DynamicsSystem &) = delete;

        // Delete the move constructor and move assignment operator.
        DynamicsSystem(DynamicsSystem &&) = delete;
        DynamicsSystem &operator=(DynamicsSystem &&) = delete;

        /** @brief Default destructor for DynamicsSystem. */
        ~DynamicsSystem() = default;

        /**
         * @brief Integrates constrained velocities to produce new constrained positions and orientations.
         *
         * Uses symplectic (semi-implicit) Euler integration:
         *   - position:    p' = p + v * dt
         *   - orientation: q' = normalize(q + 0.5 * dt * ω_q * q)  where ω_q = {0, ω}
         *
         * When @p isSplitImpulseActive is true, the split-impulse velocities (which correct penetration
         * without affecting the primary velocity state) are added before integration.
         *
         * Results are written to the constrained position/orientation slots and are not yet committed
         * to the live state; call UpdateStates() to do that.
         *
         * @param timeStep            Duration of the current simulation step.
         * @param isSplitImpulseActive Whether to include split-impulse velocities in the integration.
         */
        void IntegratePositions(Timestep timeStep, bool isSplitImpulseActive);

        /**
         * @brief Integrates external forces and torques into constrained velocities for the current step.
         *
         * For each active rigid body the method performs three operations in a single pass:
         *   1. Force/torque integration:  v' = v + dt * (1/m) * lockFactor * F
         *                                 ω' = ω + dt * lockFactor * I⁻¹ * τ
         *   2. Gravity application (if enabled for the body): Δv = dt * lockFactor * g
         *   3. Velocity damping (Padé approximation of e^(-c*dt)):  v' *= 1 / (1 + c * dt)
         *
         * The resulting velocities are stored as constrained velocities and will be further corrected
         * by the constraint/contact solver before being committed by UpdateStates().
         *
         * Also resets split velocities to zero at the start of the call.
         *
         * @param timeStep Duration of the current simulation step.
         */
        void IntegrateVelocities(Timestep timeStep);

        /**
         * @brief Commits constrained positions, orientations, and velocities back to the live body state.
         *
         * For each active rigid body:
         *   - Copies constrained linear/angular velocities to the live velocity fields.
         *   - Copies the constrained position to the world center-of-mass.
         *   - Normalises and writes the constrained orientation to the transform component.
         *   - Recomputes the transform origin: origin = worldCoM - rotation * localCoM.
         *
         * After updating all body transforms, recomputes the local-to-world transform of every active
         * collider so that collision detection sees correct geometry in the next step.
         */
        void UpdateStates();

        /**
         * @brief Clears accumulated external forces and torques on all rigid bodies (active and inactive).
         *
         * Should be called at the end of each simulation step so that forces applied via the public API
         * do not carry over to subsequent steps unless explicitly reapplied.
         */
        void ResetForcesAndTorques();

        /**
         * @brief Zeroes the split-impulse linear and angular velocities of all active rigid bodies.
         *
         * Split velocities are used exclusively for penetration correction and must be cleared at the
         * start of each velocity integration pass. This method is called automatically by
         * IntegrateVelocities(); there is generally no need to call it directly.
         */
        void ResetSplitVelocities();

    private:
        /** @brief Store that holds collider components; used to update local-to-world transforms in UpdateStates(). */
        ColliderComponentStore &_colliderStore;

        /** @brief Store that holds all rigid body state: velocities, masses, inertia tensors, constrained values, etc. */
        RigidBodyComponentStore &_rigidBodyStore;

        /** @brief Store that holds the position/rotation transform for each body entity. */
        TransformComponentStore &_transformStore;

        /** @brief World-level gravity toggle; when false the gravity step in IntegrateVelocities() is skipped entirely. */
        bool &_enableGravity;

        /** @brief World gravity acceleration vector (m/s²), applied to every body with per-body gravity enabled. */
        glm::vec3 &_gravity;
    };

} // namespace Vulkyrie
