#include "physics/collision/narrowphase/convex_polyhedron_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sat/sat_algorithm.h"

namespace Vulkyrie {

    bool ConvexPolyhedronVsConvexPolyhedronAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &batch,
                                                                            size_t batchStartIndex,
                                                                            size_t batchItemsCount,
                                                                            bool clipWithPreviousAxisIfStillColliding) {

        // For convex polyhedron vs convex polyhedron collision checks, we will directly use the SAT algorithm without attempting GJK first, since GJK does not
        // provide contact information and would require additional steps to compute it (e.g. EPA or MPR) if a collision is detected. By contrast, SAT can
        // compute contact information directly as part of the algorithm, so we can avoid the overhead of running two separate algorithms and potentially
        // redundant computations.
        SATAlgorithm satAlgorithm(clipWithPreviousAxisIfStillColliding);
        const bool collisionDetected = satAlgorithm.PerformConvexPolyhedronVsConvexPolyhedronCollisionCheck(batch, batchStartIndex, batchItemsCount);

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            LastFrameCollisionData &lastFrameCollisionData = batch.Data[i].LastFrameCollisionData;

            lastFrameCollisionData.WasUsingGJKAlgorithm = false;
            lastFrameCollisionData.WasUsingSATAlgorithm = true;
        }

        return collisionDetected;
    }

} // namespace Vulkyrie
