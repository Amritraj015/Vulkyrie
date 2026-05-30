#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class CapsuleVsCapsuleAlgorithm final {
    public:
        CapsuleVsCapsuleAlgorithm() = default;

        CapsuleVsCapsuleAlgorithm(const CapsuleVsCapsuleAlgorithm &) = delete;
        CapsuleVsCapsuleAlgorithm &operator=(const CapsuleVsCapsuleAlgorithm &) = delete;

        CapsuleVsCapsuleAlgorithm(CapsuleVsCapsuleAlgorithm &&) = delete;
        CapsuleVsCapsuleAlgorithm &operator=(CapsuleVsCapsuleAlgorithm &&) = delete;

        ~CapsuleVsCapsuleAlgorithm() = default;

        bool PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
