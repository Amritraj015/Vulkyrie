#include "physics/collision/narrowphase/convex_polyhedron_vs_convex_polyhedron_algorithm.h"

namespace Vulkyrie {

    bool ConvexPolyhedronVsConvexPolyhedronAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &batch,
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
