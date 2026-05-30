#include "physics/collision/narrowphase/capsule_vs_convex_polyhedron_algorithm.h"

namespace Vulkyrie {

    bool CapsuleVsConvexPolyhedronAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &batch,
                                                                   size_t batchStartIndex,
                                                                   size_t batchItemsCount,
                                                                   bool clipWithPreviousAxisIfStillColliding) {
        (void)batch;
        (void)batchStartIndex;
        (void)batchItemsCount;
        (void)clipWithPreviousAxisIfStillColliding;
        bool collisionDetected = false;
        return collisionDetected;
    }

} // namespace Vulkyrie
