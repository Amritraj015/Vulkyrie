#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"

namespace Vulkyrie {

    class CapsuleVsCapsuleAlgorithm final {
    public:
        /** @brief Default constructor. */
        CapsuleVsCapsuleAlgorithm() = default;

        // Delete the copy constructor and copy assignment operator.
        CapsuleVsCapsuleAlgorithm(const CapsuleVsCapsuleAlgorithm &) = delete;
        CapsuleVsCapsuleAlgorithm &operator=(const CapsuleVsCapsuleAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        CapsuleVsCapsuleAlgorithm(CapsuleVsCapsuleAlgorithm &&) = delete;
        CapsuleVsCapsuleAlgorithm &operator=(CapsuleVsCapsuleAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~CapsuleVsCapsuleAlgorithm() = default;

        /** @brief Performs narrow phase collision detection for a batch of capsule pairs.
         * For each pair, checks if the capsules are intersecting and, if so, computes contact information.
         * @param batch Batch of narrow phase data for collision pairs.
         * @param batchStartIndex Index of the first pair in the batch.
         * @param batchItemsCount Number of pairs to process in the batch.
         * @returns True if any collision is detected in the batch, false otherwise.
         */
        bool PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);
    };

} // namespace Vulkyrie
