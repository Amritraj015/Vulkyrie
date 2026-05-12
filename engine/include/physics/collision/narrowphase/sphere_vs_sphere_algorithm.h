#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class SphereVsSphereAlgorithm final {
        public:
            SphereVsSphereAlgorithm() = default;

            SphereVsSphereAlgorithm(const SphereVsSphereAlgorithm &) = delete;
            SphereVsSphereAlgorithm &operator=(const SphereVsSphereAlgorithm &) = delete;

            SphereVsSphereAlgorithm(SphereVsSphereAlgorithm &&) = delete;
            SphereVsSphereAlgorithm &operator=(SphereVsSphereAlgorithm &&) = delete;

            ~SphereVsSphereAlgorithm() = default;

            /**
             * Performs narrow phase collision detection for a batch of sphere pairs.
             * For each pair, checks if the spheres are intersecting and, if so, computes contact information.
             * @param narrowPhaseDataBatch Batch of narrow phase data for collision pairs.
             * @param batchStartIndex Index of the first pair in the batch.
             * @param batchItemsCount Number of pairs to process in the batch.
             * @return True if any collision is detected in the batch, false otherwise.
             */
            bool PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
