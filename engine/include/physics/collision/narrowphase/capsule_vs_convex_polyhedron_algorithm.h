#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class CapsuleVsConvexPolyhedronAlgorithm final {
        public:
            CapsuleVsConvexPolyhedronAlgorithm() = default;

            CapsuleVsConvexPolyhedronAlgorithm(const CapsuleVsConvexPolyhedronAlgorithm &) = delete;
            CapsuleVsConvexPolyhedronAlgorithm &operator=(const CapsuleVsConvexPolyhedronAlgorithm &) = delete;

            CapsuleVsConvexPolyhedronAlgorithm(CapsuleVsConvexPolyhedronAlgorithm &&) = delete;
            CapsuleVsConvexPolyhedronAlgorithm &operator=(CapsuleVsConvexPolyhedronAlgorithm &&) = delete;

            ~CapsuleVsConvexPolyhedronAlgorithm() = default;

            bool PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch,
                                       size_t batchStartIndex,
                                       size_t batchItemsCount,
                                       bool clipWithPreviousAxisIfStillColliding);
    };

} // namespace Vulkyrie
