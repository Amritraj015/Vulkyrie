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

    class ContactSolverSystem {
        struct ContactPointSolver {
            glm::vec3 Normal;
            glm::vec3 R1;
            glm::vec3 R2;
            ContactPoint *ExternalPoint;
            f32 PenetrationDepth;
            f32 RestitutionBias;
            f32 PenetrationImpulse;
            f32 PenetrationSplitImpulse;

            /// Inverse of the matrix K for the penetration
            f32 InversePenetrationMass;

            /// Cross product of r1 with the contact normal
            glm::vec3 i1TimesR1CrossN;

            /// Cross product of r2 with the contact normal
            glm::vec3 i2TimesR2CrossN;

            /// True if the contact was existing last time step
            bool IsRestingContact;
        };

        struct ContactManifoldSolver {
            glm::mat3 InverseInertiaTensorOfBodyOne;
            glm::mat3 InverseInertiaTensorOfBodyTwo;
            glm::vec3 LinearLockAxisFactorOfBodyOne;
            glm::vec3 LinearLockAxisFactorOfBodyTwo;
            glm::vec3 AngularLockAxisFactorOfBodyOne;
            glm::vec3 AngularLockAxisFactorOfBodyTwo;
            ContactManifold *ExternalContactManifold;
            size_t RigidBodyComponentIndexOfBodyOne;
            size_t RigidBodyComponentIndexOfBodyTwo;
            f32 MassInverseOfBodyOne;
            f32 MassInverseOfBodyTwo;
            f32 FrictionCoefficient;

            // - Variables used when friction constraints are apply at the center of the manifold-//
            glm::vec3 Normal;
            glm::vec3 FrictionPointBodyOne;
            glm::vec3 FrictionPointBodyTwo;
            glm::vec3 r1Friction;
            glm::vec3 r2Friction;
            glm::vec3 r1CrossT1;
            glm::vec3 r1CrossT2;
            glm::vec3 r2CrossT1;
            glm::vec3 r2CrossT2;
            glm::vec3 FrictionVectorOne;
            glm::vec3 FrictionVectorTwo;
            glm::vec3 OldFrictionVectorOne;
            glm::vec3 OldFrictionVectorTwo;
            f32 InverseFriction1Mass;
            f32 InverseFriction2Mass;
            f32 InverseTwistFrictionMass;
            f32 Friction1Impulse;
            f32 Friction2Impulse;
            f32 FrictionTwistImpulse;
            u8 TotalContactPoints;
        };

    public:
        explicit ContactSolverSystem(PhysicsWorld &world, Islands &islands, f32 &restitutionVelocityThreshold);

        VE_DELETE_MOVE_AND_COPY(ContactSolverSystem);

        ~ContactSolverSystem() = default;

        [[nodiscard]] VE_INLINE bool IsSplitImpulseActive() const {
            return _splitImpulseActive;
        }

        VE_INLINE void SetSplitImpulseActiveFlag(bool active) {
            _splitImpulseActive = active;
        }

        void Initialize(std::vector<ContactManifold> *contactManifolds, std::vector<ContactPoint> *contactPoints);
        void StoreImpulses();
        void Solve(Timestep timestep);
        void Reset();

    private:
        static constexpr f32 BETA = f32(0.2);
        static constexpr f32 BETA_SPLIT_IMPULSE = f32(0.2);
        static constexpr f32 SLOP = f32(0.01);

        std::vector<ContactManifoldSolver> _contactConstraints;
        std::vector<ContactPointSolver> _contactPoints;

        [[maybe_unused]] f32 &_restitutionVelocityThreshold;
        Islands &_islands;

        std::vector<ContactManifold> *_allContactManifolds;
        std::vector<ContactPoint> *_allContactPoints;

        BodyComponentStore &_bodyStore;
        RigidBodyComponentStore &_rigidBodyStore;
        ColliderComponentStore &_colliderStore;

        size_t _totalContactPoints;
        size_t _totalContactManifolds;
        bool _splitImpulseActive;

        [[nodiscard]] VE_INLINE f32 computeMixedRestitutionFactor(const Material &material1, const Material &material2) const {
            const f32 restitution1 = material1.GetRestitutionCoefficient();
            const f32 restitution2 = material2.GetRestitutionCoefficient();

            return (restitution1 > restitution2) ? restitution1 : restitution2;
        }

        [[nodiscard]] VE_INLINE f32 computeMixedFrictionCoefficient(const Material &material1, const Material &material2) const {
            return material1.GetFrictionCoefficientSquareRoot() * material2.GetFrictionCoefficientSquareRoot();
        }

        void initializeForIsland(size_t islandIndex);
        void computeFrictionVectors(const glm::vec3 &deltaVelocity, ContactManifoldSolver &contactPoint) const;
        void warmStart();
    };
} // namespace Vulkyrie
