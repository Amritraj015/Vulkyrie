#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class SphereVsSphereAlgorithm final {
    public:
        /** @brief Default constructor. */
        SphereVsSphereAlgorithm() = default;

        // Delete the copy constructor and copy assignment operator.
        SphereVsSphereAlgorithm(const SphereVsSphereAlgorithm &) = delete;
        SphereVsSphereAlgorithm &operator=(const SphereVsSphereAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        SphereVsSphereAlgorithm(SphereVsSphereAlgorithm &&) = delete;
        SphereVsSphereAlgorithm &operator=(SphereVsSphereAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~SphereVsSphereAlgorithm() = default;

        /**
         * Performs narrow phase collision detection for a batch of sphere pairs.
         * For each pair, checks if the spheres are intersecting and, if so, computes contact information.
         * @param batch Batch of narrow phase data for collision pairs.
         * @param batchStartIndex Index of the first pair in the batch.
         * @param batchItemsCount Number of pairs to process in the batch.
         * @returns True if any collision is detected in the batch, false otherwise.
         */
        bool PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
