#pragma once

#include "physics/types/contact_manifold.h"
#include "physics/types/contact_point.h"

namespace Vulkyrie {

    class ContactSolverSystem {
        struct ContactPointSolver {
            ContactPoint *ExternalPoint;
            glm::vec3 Normal;
            glm::vec3 BodyOneCenterToContactPointVector;
            glm::vec3 BodyTwoCenterToContactPointVector;
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
        void SetIsSplitImpulseActive(bool active);

    private:
        /// Beta value for the penetration depth position correction without split impulses
        static constexpr f32 BETA = 0.2f;

        /// Beta value for the penetration depth position correction with split impulses
        static constexpr f32 BETA_SPLIT_IMPULSE = 0.2f;

        /// Slop distance (allowed penetration distance between bodies)
        static constexpr f32 SLOP = 0.01f;
    };
} // namespace Vulkyrie
