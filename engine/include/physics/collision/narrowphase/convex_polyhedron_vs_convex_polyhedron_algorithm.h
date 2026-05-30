#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class ConvexPolyhedronVsConvexPolyhedronAlgorithm final {
    public:
        ConvexPolyhedronVsConvexPolyhedronAlgorithm() = default;

        ConvexPolyhedronVsConvexPolyhedronAlgorithm(const ConvexPolyhedronVsConvexPolyhedronAlgorithm &) = delete;
        ConvexPolyhedronVsConvexPolyhedronAlgorithm &operator=(const ConvexPolyhedronVsConvexPolyhedronAlgorithm &) = delete;

        ConvexPolyhedronVsConvexPolyhedronAlgorithm(ConvexPolyhedronVsConvexPolyhedronAlgorithm &&) = delete;
        ConvexPolyhedronVsConvexPolyhedronAlgorithm &operator=(ConvexPolyhedronVsConvexPolyhedronAlgorithm &&) = delete;

        ~ConvexPolyhedronVsConvexPolyhedronAlgorithm() = default;

        bool PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount, bool clipWithPreviousAxisIfStillColliding);
    };

} // namespace Vulkyrie
