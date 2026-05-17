#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class SphereVsCapsuleAlgorithm final {
    public:
        SphereVsCapsuleAlgorithm() = default;

        SphereVsCapsuleAlgorithm(const SphereVsCapsuleAlgorithm &) = delete;
        SphereVsCapsuleAlgorithm &operator=(const SphereVsCapsuleAlgorithm &) = delete;

        SphereVsCapsuleAlgorithm(SphereVsCapsuleAlgorithm &&) = delete;
        SphereVsCapsuleAlgorithm &operator=(SphereVsCapsuleAlgorithm &&) = delete;

        ~SphereVsCapsuleAlgorithm() = default;

        bool PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
