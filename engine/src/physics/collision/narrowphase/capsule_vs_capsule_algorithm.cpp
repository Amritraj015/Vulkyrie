#include "physics/collision/narrowphase/capsule_vs_capsule_algorithm.h"

namespace Vulkyrie {

    bool CapsuleVsCapsuleAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount) {
        (void)narrowPhaseDataBatch;
        (void)batchStartIndex;
        (void)batchItemsCount;
        bool collisionDetected = false;
        return collisionDetected;
    }

} // namespace Vulkyrie
