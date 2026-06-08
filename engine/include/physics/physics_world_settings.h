#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    struct PhysicsWorldSettings {
    public:
        PhysicsWorldSettings(const std::string &name)
            : Name(name) {
            Gravity = glm::vec3(0, -9.81f, 0);
            PersistentContactDistanceThresholdSquared = 0.03f * 0.03f;
            FrictionCoefficient = 0.3f;
            BounceVelocityThreshold = 0.5f;
            RestitutionCoefficient = 0.5f;
            EnableSleeping = true;
            VelocitySolverIterations = 6;
            PositionSolverIterations = 3;
            TimeToSleep = 1.0f;
            DefaultSleepLinearVelocity = f32(0.02);
            DefaultSleepAngularVelocity = f32(3.0) * (std::numbers::pi_v<f32> / f32(180.0));
            CosAngleSimilarContactManifold = f32(0.95);
        }

        std::string Name;
        glm::vec3 Gravity;
        f32 PersistentContactDistanceThresholdSquared;
        f32 FrictionCoefficient;
        f32 BounceVelocityThreshold;
        f32 RestitutionCoefficient;

        /// Number of iterations when solving the velocity constraints of the Sequential Impulse technique
        u16 VelocitySolverIterations;

        /// Number of iterations when solving the position constraints of the Sequential Impulse technique
        u16 PositionSolverIterations;

        /// Time (in seconds) that a body must stay still to be considered sleeping
        f32 TimeToSleep;

        /// A body with a linear velocity smaller than the sleep linear velocity (in m/s)
        /// might enter sleeping mode.
        f32 DefaultSleepLinearVelocity;

        /// A body with angular velocity smaller than the sleep angular velocity (in rad/s)
        /// might enter sleeping mode
        f32 DefaultSleepAngularVelocity;

        /// This is used to test if two contact manifold are similar (same contact normal) in order to
        /// merge them. If the cosine of the angle between the normals of the two manifold are larger
        /// than the value bellow, the manifold are considered to be similar.
        f32 CosAngleSimilarContactManifold;

        /// True if the sleeping technique is enabled
        bool EnableSleeping;
    };

} // namespace Vulkyrie
