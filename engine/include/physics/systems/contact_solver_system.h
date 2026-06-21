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

    class ContactSolverSystem {
        struct ContactPointSolver {
            ContactPoint *ExternalPoint;
            glm::vec3 Normal;
            glm::vec3 R1;
            glm::vec3 R2;
            f32 PenetrationDepth;
            f32 RestitutionBias;
            f32 PenetrationImpulse;
            f32 PenetrationSplitImpulse;

            /// Inverse of the matrix K for the penetration
            f32 inversePenetrationMass;

            /// Cross product of r1 with the contact normal
            glm::vec3 i1TimesR1CrossN;

            /// Cross product of r2 with the contact normal
            glm::vec3 i2TimesR2CrossN;

            /// True if the contact was existing last time step
            bool IsRestingContact;
        };

        struct ContactManifoldSolver {
            ContactManifold *ExternalContactManifold;
            glm::mat3 InverseInertiaTensorOfBodyOne;
            glm::mat3 InverseInertiaTensorOfBodyTwo;
            glm::vec3 LinearLockAxisFactorOfBodyOne;
            glm::vec3 LinearLockAxisFactorOfBodyTwo;
            glm::vec3 AngularLockAxisFactorOfBodyOne;
            glm::vec3 AngularLockAxisFactorOfBodyTwo;
            size_t RigidBodyComponentIndexOfBodyOne;
            size_t RigidBodyComponentIndexOfBodyTwo;
            f32 MassInverseOfBodyOne;
            f32 MassInverseOfBodyTwo;
            f32 FrictionCoefficient;
        };

    public:
        ContactSolverSystem(Islands &islands,
                            BodyComponentStore &bodyStore,
                            RigidBodyComponentStore &rigidBodyStore,
                            ColliderComponentStore &colliderStore,
                            f32 &restitutionVelocityThreshold);

        VE_DELETE_MOVE_AND_COPY(ContactSolverSystem);

        ~ContactSolverSystem() = default;

        void Initialize(std::vector<ContactManifold> *contactManifolds, std::vector<ContactPoint> *contactPoints);
        void InitializeForIsland(size_t islandIndex);
        void StoreImpulses();
        void Solve(Timestep timestep);
        void Reset();
        bool IsSplitImpulseActive() const;
        void SetSplitImpulseActiveFlag(bool active);

    private:
        static constexpr f32 BETA = f32(0.2);
        static constexpr f32 BETA_SPLIT_IMPULSE = f32(0.2);
        static constexpr f32 SLOP = f32(0.01);

        std::vector<ContactManifoldSolver> _contactConstraints;
        std::vector<ContactPointSolver> _contactPoints;

        f32 &_restitutionVelocityThreshold;
        Islands &_islands;

        std::vector<ContactManifold> *_allContactManifolds;
        std::vector<ContactPoint> *_allContactPoints;

        BodyComponentStore &_bodyStore;
        RigidBodyComponentStore &_rigidBodyStore;
        ColliderComponentStore &_colliderStore;

        size_t _totalContactPoints;
        size_t _totalContactManifolds;
        // Timestep _timestep;
        bool _splitImpulseActive;

        f32 computeMixedRestitutionFactor(const Material &material1, const Material &material2) const;
        f32 computeMixedFrictionCoefficient(const Material &material1, const Material &material2) const;
        void computeFrictionVectors(const glm::vec3 &deltaVelocity, ContactManifoldSolver &contactPoint) const;
        void warmStart();
    };
} // namespace Vulkyrie
