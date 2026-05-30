#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class CapsuleVsConvexPolyhedronAlgorithm final {
    public:
        /** @brief Default constructor. */
        CapsuleVsConvexPolyhedronAlgorithm() = default;

        // Delete the copy constructor and copy assignment operator.
        CapsuleVsConvexPolyhedronAlgorithm(const CapsuleVsConvexPolyhedronAlgorithm &) = delete;
        CapsuleVsConvexPolyhedronAlgorithm &operator=(const CapsuleVsConvexPolyhedronAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        CapsuleVsConvexPolyhedronAlgorithm(CapsuleVsConvexPolyhedronAlgorithm &&) = delete;
        CapsuleVsConvexPolyhedronAlgorithm &operator=(CapsuleVsConvexPolyhedronAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~CapsuleVsConvexPolyhedronAlgorithm() = default;

        /** @brief Performs narrow phase collision detection for a batch of capsule vs convex polyhedron pairs.
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
