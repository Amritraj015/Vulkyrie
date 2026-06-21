#pragma once

#include "vlkypch.h"
#include "core/entity.h"

namespace Vulkyrie {

    /**
     * @brief Represents a resolved contact between two colliders after narrow-phase detection.
     *
     * Contains all data required for constraint-based collision response, including
     * the involved rigid body and collider entities, contact point references, accumulated
     * friction impulses, and friction direction vectors. Used by the constraint solver
     * during island-based sequential impulse resolution.
     */
    struct ContactManifold {
        /**
         * @brief The first friction direction vector at the contact point, used for friction constraint resolution.
         */
        glm::vec3 FrictionVectorOne;

        /**
         * @brief The second friction direction vector at the contact point, orthogonal to FrictionVectorOne.
         */
        glm::vec3 FrictionVectorTwo;

        /**
         * @brief The entity representing the first rigid body in the contact pair.
         */
        Entity BodyOneEntity;

        /**
         * @brief The entity representing the second rigid body in the contact pair.
         */
        Entity BodyTwoEntity;

        /**
         * @brief The entity representing the first collider in the contact pair.
         */
        Entity ColliderOneEntity;

        /**
         * @brief The entity representing the second collider in the contact pair.
         */
        Entity ColliderTwoEntity;

        /**
         * @brief The index into the global contact point buffer for this manifold's contact point(s).
         */
        u32 ContactPointIndex;

        /**
         * @brief The accumulated friction impulse along FrictionVectorOne.
         */
        f32 FrictionImpulseOne;

        /**
         * @brief The accumulated friction impulse along FrictionVectorTwo.
         */
        f32 FrictionImpulseTwo;

        /**
         * @brief The accumulated twist (torsional) friction impulse at the contact point.
         */
        f32 FrictionTwistImpulse;

        /**
         * @brief The number of contact points in this manifold.
         */
        u8 ContactPointCount;

        /**
         * @brief Whether this manifold is already part of an island during constraint solving.
         */
        bool IsAlreadyInIsland;

        /**
         * @brief Constructor. Initializes all friction impulses and vectors to zero, sets entity and contact point info, and marks as not in island.
         * @param bodyOneEntity The first rigid body entity.
         * @param bodyTwoEntity The second rigid body entity.
         * @param colliderOneEntity The first collider entity.
         * @param colliderTwoEntity The second collider entity.
         * @param contactPointsIndex The index into the contact point buffer.
         * @param nbContactPoints The number of contact points in this manifold.
         */
        ContactManifold(
            Entity bodyOneEntity, Entity bodyTwoEntity, Entity colliderOneEntity, Entity colliderTwoEntity, u32 contactPointsIndex, u8 nbContactPoints)
            : FrictionVectorOne(0.0f)
            , FrictionVectorTwo(0.0f)
            , BodyOneEntity(bodyOneEntity)
            , BodyTwoEntity(bodyTwoEntity)
            , ColliderOneEntity(colliderOneEntity)
            , ColliderTwoEntity(colliderTwoEntity)
            , ContactPointIndex(contactPointsIndex)
            , FrictionImpulseOne(0.0f)
            , FrictionImpulseTwo(0.0f)
            , FrictionTwistImpulse(0.0f)
            , ContactPointCount(nbContactPoints)
            , IsAlreadyInIsland(false) {
        }

        VE_DELETE_COPY(ContactManifold);

        ContactManifold(ContactManifold &&) = default;
        ContactManifold &operator=(ContactManifold &&) = delete;

        ~ContactManifold() = default;
    };

} // namespace Vulkyrie
