#pragma once

#include "vlkypch.h"
#include "core/time_step.h"
#include "physics/components/body_component_store.h"
#include "physics/components/collider_component_store.h"
#include "physics/components/rigid_body_component_store.h"
#include "physics/types/contact_manifold.h"
#include "physics/types/contact_point.h"
#include "physics/types/islands.h"
#include "physics/types/material.h"

namespace Vulkyrie {

    class PhysicsWorld;

    /** @brief Sequential-impulse solver for all contact constraints of the world.
     *
     * For every contact manifold produced by collision detection, the solver enforces one non-penetration
     * constraint per contact point plus three friction constraints applied at the center of the manifold
     * (two tangential directions and a twist around the contact normal, clamped to the Coulomb cone).
     * The solver follows the standard pipeline shared with the joint solvers: Initialize() builds the
     * internal constraint data for every island and warm-starts it with the impulses accumulated during
     * the previous step, Solve() is iterated (Gauss-Seidel) to drive the relative velocities to zero,
     * StoreImpulses() saves the accumulated impulses back into the external contacts for the next step's
     * warm start, and Reset() clears the external contacts once the step is done.
     *
     * Residual penetration is corrected either with Baumgarte stabilization (folded into the velocity
     * constraint) or with split impulses solved against separate pseudo velocities, depending on the
     * split-impulse flag. The system holds references to the component stores owned by the PhysicsWorld
     * and is non-copyable and non-movable. */
    class ContactSolverSystem {
        /** @brief Internal solver state of a single contact point (one non-penetration constraint). */
        struct ContactPointConstraint {
            /** @brief World-space contact normal of the point. */
            glm::vec3 Normal;

            /** @brief Lever arm from body one's center of mass to the contact point, in world space. */
            glm::vec3 R1;

            /** @brief Lever arm from body two's center of mass to the contact point, in world space. */
            glm::vec3 R2;

            /** @brief The external contact point this constraint was built from; used to persist the accumulated impulse across steps. */
            ContactPoint *ExternalPoint;

            /** @brief Penetration depth of the contact point at the beginning of the step. */
            f32 PenetrationDepth;

            /** @brief Restitution velocity bias, computed from the relative normal velocity at the beginning of the step. */
            f32 RestitutionBias;

            /** @brief Accumulated impulse along the contact normal (always non-negative). */
            f32 PenetrationImpulse;

            /** @brief Accumulated split impulse used for the position correction (always non-negative). */
            f32 PenetrationSplitImpulse;

            /** @brief Inverse effective mass 1/K of the non-penetration constraint (0 when both bodies are immovable). */
            f32 InversePenetrationMass;

            /** @brief Precomputed I1^-1 * (R1 x N), applied to body one's angular velocity on every solver iteration. */
            glm::vec3 i1TimesR1CrossN;

            /** @brief Precomputed I2^-1 * (R2 x N), applied to body two's angular velocity on every solver iteration. */
            glm::vec3 i2TimesR2CrossN;

            /** @brief Whether the contact point already existed during the previous step; only resting contacts are warm-started. */
            bool IsRestingContact;
        };

        /** @brief Internal solver state of a contact manifold: per-body data shared by its contact points and
         * the three friction constraints applied at the center of the manifold. */
        struct ContactManifoldConstraint {
            /** @brief World-space inverse inertia tensor of body one. */
            glm::mat3 InverseInertiaTensorOfBody1;

            /** @brief World-space inverse inertia tensor of body two. */
            glm::mat3 InverseInertiaTensorOfBody2;

            /** @brief Per-axis factor (0 or 1) masking body one's linear velocity response along locked axes. */
            glm::vec3 LinearLockAxisFactorOfBody1;

            /** @brief Per-axis factor (0 or 1) masking body two's linear velocity response along locked axes. */
            glm::vec3 LinearLockAxisFactorOfBody2;

            /** @brief Per-axis factor (0 or 1) masking body one's angular velocity response around locked axes. */
            glm::vec3 AngularLockAxisFactorOfBody1;

            /** @brief Per-axis factor (0 or 1) masking body two's angular velocity response around locked axes. */
            glm::vec3 AngularLockAxisFactorOfBody2;

            /** @brief The external contact manifold this constraint was built from; used to persist the friction impulses across steps. */
            ContactManifold *ExternalContactManifold;

            /** @brief Index of body one in the RigidBodyComponentStore. */
            size_t RigidBodyComponentIndexOfBody1;

            /** @brief Index of body two in the RigidBodyComponentStore. */
            size_t RigidBodyComponentIndexOfBody2;

            /** @brief Inverse mass of body one. */
            f32 MassInverseOfBody1;

            /** @brief Inverse mass of body two. */
            f32 MassInverseOfBody2;

            /** @brief Mixed friction coefficient of the two colliders' materials. */
            f32 FrictionCoefficient;

            // ---- Data of the friction constraints applied at the center of the manifold. ---- //

            /** @brief Average contact normal of the manifold (normalized); the friction and twist constraints act around it. */
            glm::vec3 Normal;

            /** @brief Average of the manifold's world-space contact points on body one; the friction constraints are applied here. */
            glm::vec3 FrictionPointBody1;

            /** @brief Average of the manifold's world-space contact points on body two; the friction constraints are applied here. */
            glm::vec3 FrictionPointBody2;

            /** @brief Lever arm from body one's center of mass to the friction point, in world space. */
            glm::vec3 r1Friction;

            /** @brief Lever arm from body two's center of mass to the friction point, in world space. */
            glm::vec3 r2Friction;

            /** @brief Precomputed r1Friction x FrictionVector1. */
            glm::vec3 r1CrossT1;

            /** @brief Precomputed r1Friction x FrictionVector2. */
            glm::vec3 r1CrossT2;

            /** @brief Precomputed r2Friction x FrictionVector1. */
            glm::vec3 r2CrossT1;

            /** @brief Precomputed r2Friction x FrictionVector2. */
            glm::vec3 r2CrossT2;

            /** @brief First friction direction, aligned with the tangential sliding velocity when there is one. */
            glm::vec3 FrictionVector1;

            /** @brief Second friction direction; (FrictionVector1, FrictionVector2, Normal) forms an orthonormal basis. */
            glm::vec3 FrictionVector2;

            /** @brief First friction direction of the previous step, used to project the warm-started friction impulses. */
            glm::vec3 OldFrictionVector1;

            /** @brief Second friction direction of the previous step, used to project the warm-started friction impulses. */
            glm::vec3 OldFrictionVector2;

            /** @brief Inverse effective mass 1/K of the first tangential friction constraint (0 when both bodies are immovable). */
            f32 InverseFriction1Mass;

            /** @brief Inverse effective mass 1/K of the second tangential friction constraint (0 when both bodies are immovable). */
            f32 InverseFriction2Mass;

            /** @brief Inverse effective mass 1/K of the twist friction constraint (0 when both bodies are immovable). */
            f32 InverseTwistFrictionMass;

            /** @brief Accumulated impulse along the first friction direction. */
            f32 Friction1Impulse;

            /** @brief Accumulated impulse along the second friction direction. */
            f32 Friction2Impulse;

            /** @brief Accumulated twist friction impulse around the manifold normal. */
            f32 FrictionTwistImpulse;

            /** @brief Number of contact points in the manifold. */
            u8 TotalContactPoints;
        };

    public:
        /** @brief Constructs the solver, binding it to the stores it operates on.
         * @param world The physics world whose component stores supply the bodies and colliders to solve.
         * @param islands Reference to the world's islands, used to iterate the contact manifolds island by island.
         * @param restitutionVelocityThreshold Reference to the world's threshold below which an impact velocity
         *        produces no restitution (the contact is treated as resting). */
        explicit ContactSolverSystem(PhysicsWorld &world, Islands &islands, f32 &restitutionVelocityThreshold);

        VE_DELETE_MOVE_AND_COPY(ContactSolverSystem);

        ~ContactSolverSystem() = default;

        /** @brief Tells whether the split-impulse position correction is active.
         * @returns True when residual penetration is corrected with split impulses, false when Baumgarte
         *          stabilization is folded into the velocity constraint instead. */
        [[nodiscard]] VE_INLINE bool IsSplitImpulseActive() const {
            return _splitImpulseActive;
        }

        /** @brief Enables or disables the split-impulse position correction.
         * @param active True to correct residual penetration with split impulses, false to use Baumgarte stabilization. */
        VE_INLINE void SetSplitImpulseActiveFlag(bool active) {
            _splitImpulseActive = active;
        }

        /** @brief Builds the internal solver constraints from the external contacts and warm-starts them.
         *
         * Clears the solver data of the previous step, builds one manifold constraint per contact manifold and
         * one point constraint per contact point for every island (see initializeForIsland()), then re-applies
         * the impulses accumulated during the previous step. Must be called once per step before Solve().
         * @param contactManifolds The contact manifolds of the step, owned by the CollisionSystem. Must stay
         *        alive and unmodified until Reset() is called.
         * @param contactPoints The contact points of the step, owned by the CollisionSystem. Must stay alive
         *        and unmodified until Reset() is called. */
        void Initialize(std::vector<ContactManifold> *contactManifolds, std::vector<ContactPoint> *contactPoints);

        /** @brief Saves the accumulated impulses back into the external contacts.
         *
         * Writes the penetration impulse of every contact point and the friction impulses and friction vectors
         * of every manifold back to the external contacts, so the next step can warm start from them. Must be
         * called after the last Solve() iteration of the step. */
        void StoreImpulses();

        /** @brief Performs one velocity-constraint solver iteration over all contact constraints.
         *
         * For each manifold, solves the non-penetration constraint of every contact point (plus its split-impulse
         * counterpart when active), then the two tangential friction constraints and the twist friction constraint
         * at the center of the manifold, clamping the friction impulses to the Coulomb cone. Typically called
         * multiple times per step by the constraint solver.
         * @param timestep The duration of the current simulation step, used by the Baumgarte bias. */
        void Solve(Timestep timestep);

        /** @brief Clears the external contacts of the step that was just solved.
         *
         * The external vectors are owned by the CollisionSystem, which refills them during the next collision
         * detection; the internal solver data is cleared by the next Initialize() call. */
        void Reset();

    private:
        /** @brief Baumgarte position-correction factor: the fraction of the residual penetration corrected per step. */
        static constexpr f32 BETA = f32(0.2);

        /** @brief Position-correction factor used by the split-impulse constraint when split impulses are active. */
        static constexpr f32 BETA_SPLIT_IMPULSE = f32(0.2);

        /** @brief Penetration depth tolerated without position correction, to avoid jitter on resting contacts. */
        static constexpr f32 SLOP = f32(0.01);

        /** @brief Internal manifold constraints of the current step, rebuilt by Initialize(). */
        std::vector<ContactManifoldConstraint> _contactConstraints;

        /** @brief Internal contact point constraints of the current step, rebuilt by Initialize(). Points of the
         * same manifold are contiguous and ordered like _contactConstraints. */
        std::vector<ContactPointConstraint> _contactPoints;

        /** @brief Reference to the world's threshold below which an impact velocity produces no restitution. */
        f32 &_restitutionVelocityThreshold;

        /** @brief Reference to the world's islands. */
        Islands &_islands;

        /** @brief The external contact manifolds of the step, owned by the CollisionSystem. Set by Initialize(). */
        std::vector<ContactManifold> *_allContactManifolds;

        /** @brief The external contact points of the step, owned by the CollisionSystem. Set by Initialize(). */
        std::vector<ContactPoint> *_allContactPoints;

        /** @brief A reference to BodyComponentStore. */
        BodyComponentStore &_bodyStore;

        /** @brief A reference to RigidBodyComponentStore. */
        RigidBodyComponentStore &_rigidBodyStore;

        /** @brief A reference to ColliderComponentStore. */
        ColliderComponentStore &_colliderStore;

        /** @brief Whether residual penetration is corrected with split impulses instead of Baumgarte stabilization. */
        bool _splitImpulseActive;

        /** @brief Mixes the restitution coefficients of two colliding materials.
         * @param material1 The material of the first collider.
         * @param material2 The material of the second collider.
         * @returns The mixed restitution coefficient used for the contact. */
        [[nodiscard]] VE_INLINE f32 computeMixedRestitutionFactor(const Material &material1, const Material &material2) const {
            const f32 restitution1 = material1.GetRestitutionCoefficient();
            const f32 restitution2 = material2.GetRestitutionCoefficient();

            // The most bouncy of the two materials dominates the collision.
            return (restitution1 > restitution2) ? restitution1 : restitution2;
        }

        /** @brief Mixes the friction coefficients of two colliding materials.
         * @param material1 The material of the first collider.
         * @param material2 The material of the second collider.
         * @returns The mixed friction coefficient used for the contact. */
        [[nodiscard]] VE_INLINE f32 computeMixedFrictionCoefficient(const Material &material1, const Material &material2) const {
            // Geometric mean of the two friction coefficients.
            return material1.GetFrictionCoefficientSquareRoot() * material2.GetFrictionCoefficientSquareRoot();
        }

        /** @brief Builds the internal solver constraints for every contact manifold of one island.
         *
         * Snapshots the per-body data shared by the manifold's contact points, precomputes the per-point
         * constraint data (lever arms, effective masses, restitution bias), and derives the friction data at
         * the center of the manifold (friction point, friction vectors, effective masses).
         * @param islandIndex The index of the island whose contact manifolds are initialized. */
        void initializeForIsland(size_t islandIndex);

        /** @brief Computes the two friction vectors of a manifold from the tangential relative velocity.
         *
         * The first friction vector points along the tangential velocity when there is one (so the first
         * friction constraint directly opposes sliding); otherwise an arbitrary unit vector orthogonal to the
         * normal is used. The second vector completes the (t1, t2, n) orthonormal basis.
         * @param deltaVelocity The relative velocity of the two bodies at the friction point.
         * @param contactManifold The manifold constraint whose FrictionVector1/FrictionVector2 are written. */
        void computeFrictionVectors(const glm::vec3 &deltaVelocity, ContactManifoldConstraint &contactManifold) const;

        /** @brief Re-applies the impulses accumulated during the previous step to converge faster.
         *
         * For resting contact points, re-applies the penetration impulse; for manifolds with at least one
         * resting point, projects the old friction impulses onto the new friction vectors and re-applies them.
         * Impulses of new contacts are reset to zero instead. Called by Initialize(). */
        void warmStart();
    };

} // namespace Vulkyrie
