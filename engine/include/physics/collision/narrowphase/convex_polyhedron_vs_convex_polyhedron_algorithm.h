#pragma once

#include "vlkypch.h"
#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class ConvexPolyhedronVsConvexPolyhedronAlgorithm final {
    public:
        /** @brief Default constructor. */
        ConvexPolyhedronVsConvexPolyhedronAlgorithm() = default;

        VE_DELETE_MOVE_AND_COPY(ConvexPolyhedronVsConvexPolyhedronAlgorithm);

        /** @brief Default destructor. */
        ~ConvexPolyhedronVsConvexPolyhedronAlgorithm() = default;

        /** @brief Performs narrow phase collision detection for a batch of convex polyhedron pairs.
         * For each pair, checks if the shapes are intersecting and, if so, computes contact information.
         * @param batch Batch of narrow phase data for collision pairs.
         * @param batchStartIndex Index of the first pair in the batch.
         * @param batchItemsCount Number of pairs to process in the batch.
         * @param clipWithPreviousAxisIfStillColliding If true, performs additional clipping against the previous separating axis if the initial check still
         * detects a collision. This can help reduce false positives in certain edge cases.
         * @returns True if any collision is detected in the batch, false otherwise.
         */
        bool PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount, bool clipWithPreviousAxisIfStillColliding);
    };

} // namespace Vulkyrie
