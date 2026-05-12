#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class SphereVsConvexPolyhedronAlgorithm final {
        public:
            SphereVsConvexPolyhedronAlgorithm() = default;

            SphereVsConvexPolyhedronAlgorithm(const SphereVsConvexPolyhedronAlgorithm &) = delete;
            SphereVsConvexPolyhedronAlgorithm &operator=(const SphereVsConvexPolyhedronAlgorithm &) = delete;

            SphereVsConvexPolyhedronAlgorithm(SphereVsConvexPolyhedronAlgorithm &&) = delete;
            SphereVsConvexPolyhedronAlgorithm &operator=(SphereVsConvexPolyhedronAlgorithm &&) = delete;

            ~SphereVsConvexPolyhedronAlgorithm() = default;

            bool PerformCollisionCheck(NarrowPhaseData &narrowPhaseDataBatch,
                                       size_t batchStartIndex,
                                       size_t batchItemsCount,
                                       bool clipWithPreviousAxisIfStillColliding);
    };

} // namespace Vulkyrie
