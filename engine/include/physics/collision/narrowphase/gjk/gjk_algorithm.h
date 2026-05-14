#pragma once

#include "vlkypch.h"
#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {
    class GJKAlgorithm final {
        public:
            enum class GJKResult : i32 { NoCollision, CollisionDetected, DegenerateCase };

            GJKAlgorithm() = default;

            GJKAlgorithm(const GJKAlgorithm &) = delete;
            GJKAlgorithm &operator=(const GJKAlgorithm &) = delete;

            GJKAlgorithm(GJKAlgorithm &&) = delete;
            GJKAlgorithm &operator=(GJKAlgorithm &&) = delete;

            ~GJKAlgorithm() = default;

            bool
            PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseInfoBatch, size_t batchStartIndex, size_t batchNbItems, std::vector<GJKResult> &gjkResults);
    };

} // namespace Vulkyrie
