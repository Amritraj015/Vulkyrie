#pragma once

#include "vlkypch.h"
#include "physics/physics_constants.h"

namespace Vulkyrie {

    struct ContactManifoldData {
        /**
         * @brief The unique identifier for the overlapping pair to which this contact manifold data belongs.
         * Used to associate this manifold with a specific pair of potentially colliding objects.
         */
        u64 PairID;

        /**
         * @brief Indices into the narrow-phase contact point buffer for all potential contact points in this manifold.
         * Each index refers to a contact point currently considered for collision response.
         * The array size is defined by MAX_CONTACT_POINTS_IN_POTENTIAL_MANIFOLD.
         */
        u32 PotentialContactPointsIndices[MAX_CONTACT_POINTS_IN_POTENTIAL_MANIFOLD];

        /**
         * @brief The number of potential contact points identified during broad-phase collision detection for this pair.
         * Indicates how many entries in PotentialContactPointsIndices are valid.
         */
        u8 TotalPotentialContactPoints;

        /**
         * @brief Constructor. Initializes the manifold for a given pair ID, zeroes the contact indices, and sets the count to zero.
         * @param pairID The unique identifier for the overlapping pair.
         */
        ContactManifoldData(u64 pairID)
            : PairID(pairID)
            , PotentialContactPointsIndices{ 0 }
            , TotalPotentialContactPoints(0) {
        }
    };

} // namespace Vulkyrie
