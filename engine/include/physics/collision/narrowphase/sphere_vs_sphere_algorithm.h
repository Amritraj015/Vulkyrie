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

            bool PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
