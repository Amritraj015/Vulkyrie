#include "physics/collision/narrowphase/gjk/gjk_algorithm.h"

namespace Vulkyrie {

    bool GJKAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseInfoBatch,
                                             size_t batchStartIndex,
                                             size_t totalBatchItems,
                                             std::vector<GJKResult> &gjkResults) {
        (void)narrowPhaseInfoBatch;
        (void)batchStartIndex;
        (void)totalBatchItems;
        (void)gjkResults;
        bool collisionDetected = false;

        return collisionDetected;
    }

} // namespace Vulkyrie
