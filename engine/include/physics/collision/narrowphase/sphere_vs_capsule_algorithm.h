#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class SphereVsCapsuleAlgorithm final {
    public:
        /** @brief Default constructor. */
        SphereVsCapsuleAlgorithm() = default;

        // Delete the copy constructor and copy assignment operator.
        SphereVsCapsuleAlgorithm(const SphereVsCapsuleAlgorithm &) = delete;
        SphereVsCapsuleAlgorithm &operator=(const SphereVsCapsuleAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        SphereVsCapsuleAlgorithm(SphereVsCapsuleAlgorithm &&) = delete;
        SphereVsCapsuleAlgorithm &operator=(SphereVsCapsuleAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~SphereVsCapsuleAlgorithm() = default;

        /** Performs narrow phase collision detection for a batch of sphere vs capsule pairs.
         * For each pair, checks if the shapes are intersecting and, if so, computes contact information.
         * @param batch Batch of narrow phase data for collision pairs.
         * @param batchStartIndex Index of the first pair in the batch.
         * @param batchItemsCount Number of pairs to process in the batch.
         * @returns True if any collision is detected in the batch, false otherwise.
         */
        bool PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
