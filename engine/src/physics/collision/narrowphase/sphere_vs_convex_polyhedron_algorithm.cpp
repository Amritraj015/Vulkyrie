#include "physics/collision/narrowphase/sphere_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/gjk/gjk_algorithm.h"
#include "physics/collision/narrowphase/sat/sat_algorithm.h"

namespace Vulkyrie {

    bool SphereVsConvexPolyhedronAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &batch,
                                                                  size_t batchStartIndex,
                                                                  size_t batchItemsCount,
                                                                  bool clipWithPreviousAxisIfStillColliding) {
        bool collisionDetected = false;

        // Run GJK first. It handles the common cases — separated shapes and shallow (margin-only) contacts —
        // cheaply without needing to test every face of the polyhedron.
        GJKAlgorithm gjkAlgorithm;
        SATAlgorithm satAlgorithm(clipWithPreviousAxisIfStillColliding);

        std::vector<GJKAlgorithm::GJKResult> results;
        results.reserve(batchItemsCount);

        gjkAlgorithm.PerformCollisionCheck(batch, batchStartIndex, batchItemsCount, results);

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = batch.Data[i];

            VASSERT((data.ShapeOne.GetType() == CollisionShapeType::Sphere && data.ShapeTwo.GetType() == CollisionShapeType::ConvexPolyhedron) ||
                        (data.ShapeTwo.GetType() == CollisionShapeType::Sphere && data.ShapeOne.GetType() == CollisionShapeType::ConvexPolyhedron),
                    "Collision pair must have a sphere and a convex polyhedron shapes.");

            LastFrameCollisionData &lastFrameData = data.LastFrameCollisionData;

            // Record which algorithm handled this pair for the next frame's temporal coherence check.
            // Default to GJK; overridden to SAT below if we fall through to deep penetration.
            lastFrameData.WasUsingGJKAlgorithm = true;
            lastFrameData.WasUsingSATAlgorithm = false;

            // Shallow penetration: the shapes overlap only within their collision margins.
            // GJK already computed a valid contact point, so no further work is needed.
            if (results[i] == GJKAlgorithm::GJKResult::CollideInMargin) {
                data.IsColliding = true;
                collisionDetected = true;

                continue;
            }

            // Deep penetration: the shapes interpenetrate beyond their margins, which GJK cannot resolve.
            // Fall back to SAT to find the minimum-penetration axis and compute an accurate contact point.
            if (results[i] == GJKAlgorithm::GJKResult::Interpenetrate) {
                collisionDetected |= satAlgorithm.PerformSphereVsConvexPolyhedronCollisionCheck(batch, i, 1);
                lastFrameData.WasUsingGJKAlgorithm = false;
                lastFrameData.WasUsingSATAlgorithm = true;

                continue;
            }

            // GJKResult::Separated — the shapes are not in contact. No action needed; data.IsColliding
            // remains false and collisionDetected is unchanged.
        }

        return collisionDetected;
    }

} // namespace Vulkyrie
