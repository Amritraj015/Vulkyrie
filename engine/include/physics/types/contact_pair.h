#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/entity.h"
#include "physics/physics_constants.h"

namespace Vulkyrie {

    /**
     * @brief Represents a confirmed collision between two colliders and all associated contact data.
     *
     * Produced after narrow-phase detection confirms a collision between an overlapping pair.
     * Tracks the contact manifolds and contact points generated for the pair, references into
     * the global manifold and contact point buffers, and state needed by the constraint solver
     * (island membership, trigger classification, and previous-frame collision status for
     * enter/stay/exit event dispatch). Potential manifold indices accumulate during the
     * collision pipeline and can be removed via RemovePotentialManifoldAtIndex.
     */
    struct ContactPair {
        /** @brief The unique identifier of the broad-phase overlapping pair this contact belongs to. */
        u64 PairID;

        /** @brief The entity representing the first rigid body involved in this contact. */
        Entity BodyOneEntity;

        /** @brief The entity representing the second rigid body involved in this contact. */
        Entity BodyTwoEntity;

        /** @brief The entity representing the first collider involved in this contact. */
        Entity ColliderOneEntity;

        /** @brief The entity representing the second collider involved in this contact. */
        Entity ColliderTwoEntity;

        // 4-byte fields (offset 40..59, no padding)

        /** @brief The index of this contact pair in the global contact pair buffer. */
        u32 ContactPairIndex;

        /** @brief The index into the global contact manifold buffer where this pair's manifolds begin. */
        u32 ContactManifoldIndex;

        /** @brief The number of contact manifolds currently active for this pair. */
        u32 ContactManifoldCount;

        /** @brief The index into the global contact point buffer where this pair's contact points begin. */
        u32 ContactPointIndex;

        /** @brief The total number of contact points across all manifolds for this pair. */
        u32 ContactPointCount;

        /** @brief The number of valid entries in PotentialContactManifoldIndices. */
        u8 PotentialContactManifoldsCount;

        /** @brief Whether this contact pair has already been added to a simulation island during constraint solving. */
        bool IsAlreadyInIsland;

        /** @brief Whether the two colliders were colliding during the previous simulation frame.
         * Used to dispatch collision enter/stay/exit events. */
        bool CollidingInPreviousFrame;

        /** @brief Whether this contact pair involves at least one trigger collider.
         * Trigger contacts generate collision events but do not produce constraint impulses. */
        bool IsTrigger;

        /** @brief Indices into the global contact manifold buffer for all potential manifolds accumulated during the
         * collision pipeline for this pair. Valid entries are in the range [0, PotentialContactManifoldsCount). */
        u32 PotentialContactManifoldIndices[MAX_POTENTIAL_CONTACT_MANIFOLDS];

        /**
         * @brief Constructs a ContactPair for the given overlapping pair and collider entities.
         * @param pairId The unique identifier of the broad-phase overlapping pair.
         * @param bodyOneEntity The first rigid body entity.
         * @param bodyTwoEntity The second rigid body entity.
         * @param colliderOneEntity The first collider entity.
         * @param colliderTwoEntity The second collider entity.
         * @param contactPairIndex The index of this pair in the global contact pair buffer.
         * @param collidingInPreviousFrame Whether the pair was colliding in the previous simulation frame.
         * @param isTrigger Whether at least one of the colliders is a trigger.
         */
        ContactPair(u64 pairId,
                    Entity bodyOneEntity,
                    Entity bodyTwoEntity,
                    Entity colliderOneEntity,
                    Entity colliderTwoEntity,
                    u32 contactPairIndex,
                    bool collidingInPreviousFrame,
                    bool isTrigger)
            : PairID(pairId)
            , BodyOneEntity(bodyOneEntity)
            , BodyTwoEntity(bodyTwoEntity)
            , ColliderOneEntity(colliderOneEntity)
            , ColliderTwoEntity(colliderTwoEntity)
            , ContactPairIndex(contactPairIndex)
            , ContactManifoldIndex(0)
            , ContactManifoldCount(0)
            , ContactPointIndex(0)
            , ContactPointCount(0)
            , PotentialContactManifoldsCount(0)
            , IsAlreadyInIsland(false)
            , CollidingInPreviousFrame(collidingInPreviousFrame)
            , IsTrigger(isTrigger)
            , PotentialContactManifoldIndices{ 0 } {
        }

        ContactPair(const ContactPair &) = delete;
        ContactPair &operator=(const ContactPair &) = delete;

        ContactPair(ContactPair &&) = default;
        ContactPair &operator=(ContactPair &&) = delete;

        /**
         * @brief Removes the potential manifold index at the given position using a swap-with-last strategy.
         *
         * Overwrites the entry at @p index with the last valid entry and decrements
         * PotentialContactManifoldsCount, avoiding any shifts. Order is not preserved.
         * @param index The position in PotentialContactManifoldIndices to remove. Must be less than PotentialContactManifoldsCount.
         */
        VE_FORCE_INLINE void RemovePotentialManifoldAtIndex(u32 index) {
            VASSERT(index < PotentialContactManifoldsCount, "Index out of bounds when trying to remove potential manifold index.");

            PotentialContactManifoldIndices[index] = PotentialContactManifoldIndices[PotentialContactManifoldsCount - 1];
            PotentialContactManifoldsCount--;
        }
    };

} // namespace Vulkyrie
