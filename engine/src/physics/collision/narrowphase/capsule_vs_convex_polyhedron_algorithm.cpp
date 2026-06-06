#include "physics/collision/narrowphase/capsule_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/gjk/gjk_algorithm.h"
#include "physics/collision/narrowphase/sat/sat_algorithm.h"
#include "core/utilities.h"

namespace Vulkyrie {

    bool CapsuleVsConvexPolyhedronAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &batch,
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

        VASSERT(results.size() == batchItemsCount, "results size must be equal to batchItemsCount");

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = batch.Data[i];

            VASSERT((data.ShapeOne.GetType() == CollisionShapeType::Capsule && data.ShapeTwo.GetType() == CollisionShapeType::ConvexPolyhedron) ||
                        (data.ShapeTwo.GetType() == CollisionShapeType::Capsule && data.ShapeOne.GetType() == CollisionShapeType::ConvexPolyhedron),
                    "Collision pair must have a capsule and a convex polyhedron shapes.");

            LastFrameCollisionData &lastFrameCollisionData = data.LastFrameCollisionData;

            // Record which algorithm handled this pair for the next frame's temporal coherence check.
            // Default to GJK; overridden to SAT below if we fall through to deep penetration.
            lastFrameCollisionData.WasUsingGJKAlgorithm = true;
            lastFrameCollisionData.WasUsingSATAlgorithm = false;

            // Shallow penetration: the shapes overlap only within their collision margins.
            // GJK produced a single contact point. For the capsule case we go one step further:
            // if a polyhedron face normal happens to be parallel to the contact normal AND orthogonal
            // to the capsule inner segment, the capsule is lying flat against that face. In that
            // situation we replace the single GJK contact point with up to two points (one per
            // endpoint of the inner segment) so the resting contact is more stable.
            if (results[i] == GJKAlgorithm::GJKResult::CollideInMargin) {
                if (data.ReportContacts) {
                    VASSERT(data.ContactPointCount > 0, "Total contact points must be more than 0.");

                    ContactPointData &contactPoint = data.ContactPoints[0];
                    const bool isShapeOneCapsule = data.ShapeOne.GetType() == CollisionShapeType::Capsule;

                    const auto *capsuleShape = static_cast<CapsuleShape *>(isShapeOneCapsule ? &data.ShapeOne : &data.ShapeTwo);
                    const auto *polyhedronShape = static_cast<ConvexPolyhedronShape *>(isShapeOneCapsule ? &data.ShapeTwo : &data.ShapeOne);

                    // Search for a polyhedron face whose normal is parallel to the GJK contact normal
                    // and orthogonal to the capsule inner segment. Only the first matching face is used.
                    for (size_t f = 0; f < polyhedronShape->GetFacesCount(); ++f) {
                        const TransformComponent &polyhedronToWorld = isShapeOneCapsule ? data.ShapeTwoToWorldTransform : data.ShapeOneToWorldTransform;
                        const TransformComponent &capsuleToWorld = isShapeOneCapsule ? data.ShapeOneToWorldTransform : data.ShapeTwoToWorldTransform;

                        const glm::vec3 faceNormal = polyhedronShape->GetFaceNormal(f);
                        glm::vec3 faceNormalInWorldSpace = polyhedronToWorld.Rotation * faceNormal;

                        const f32 capsuleHalfHeight = capsuleShape->GetHalfHeight();
                        const glm::vec3 capsuleSegmentStart(0, -capsuleHalfHeight, 0);
                        const glm::vec3 capsuleSegmentEnd(0, capsuleHalfHeight, 0);
                        const glm::vec3 capsuleInnerSegmentDirection = glm::normalize(capsuleToWorld.Rotation * (capsuleSegmentEnd - capsuleSegmentStart));

                        // The contact normal points from shape two toward shape one, so the face normal
                        // must oppose the contact normal when the capsule is shape one (the polyhedron
                        // pushes the capsule away) and agree with it when the capsule is shape two.
                        const bool isFaceNormalInDirectionOfContactNormal = glm::dot(faceNormalInWorldSpace, contactPoint.WorldSpaceContactNormal) > 0.0f;
                        const bool isFaceNormalInContactDirection =
                            (isShapeOneCapsule && !isFaceNormalInDirectionOfContactNormal) || (!isShapeOneCapsule && isFaceNormalInDirectionOfContactNormal);

                        // Only proceed if this face is facing the capsule (correct direction), the capsule
                        // is lying flat against it (inner segment orthogonal to face normal), and the face
                        // normal is parallel to the GJK contact normal.
                        if (isFaceNormalInContactDirection && AreOrthogonalVectors(faceNormalInWorldSpace, capsuleInnerSegmentDirection) &&
                            AreParallelVectors(faceNormalInWorldSpace, contactPoint.WorldSpaceContactNormal)) {

                            const TransformComponent &polyhedronToCapsuleTransform = capsuleToWorld.Inverse() * polyhedronToWorld;
                            const TransformComponent &capsuleToPolyhedronTransform = polyhedronToCapsuleTransform.Inverse();

                            // Convert the capsule inner segment endpoints into polyhedron local space for clipping.
                            const glm::vec3 capsuleSegmentStartInPolyhedronSpace = capsuleToPolyhedronTransform * capsuleSegmentStart;
                            const glm::vec3 capsuleSegmentEndInPolyhedronSpace = capsuleToPolyhedronTransform * capsuleSegmentEnd;

                            // The separating axis is the face normal expressed in capsule local space.
                            const glm::vec3 separatingAxisInCapsuleSpace = polyhedronToCapsuleTransform.Rotation * faceNormal;

                            // The contact normal must point from the polyhedron toward the capsule (i.e. toward shape one when the capsule is shape one).
                            if (isShapeOneCapsule) {
                                faceNormalInWorldSpace = -faceNormalInWorldSpace;
                            }

                            // Clip the capsule segment against the face and write up to two contact points.
                            const bool contactFound = satAlgorithm.ComputeCapsulePolyhedronFaceContactPoints(f,
                                                                                                             capsuleShape->GetRadius(),
                                                                                                             *polyhedronShape,
                                                                                                             contactPoint.PenetrationDepth,
                                                                                                             polyhedronToCapsuleTransform,
                                                                                                             faceNormalInWorldSpace,
                                                                                                             separatingAxisInCapsuleSpace,
                                                                                                             capsuleSegmentStartInPolyhedronSpace,
                                                                                                             capsuleSegmentEndInPolyhedronSpace,
                                                                                                             batch,
                                                                                                             i,
                                                                                                             isShapeOneCapsule);

                            if (contactFound) {
                                break;
                            }
                        }
                    }
                }

                data.IsColliding = true;
                collisionDetected = true;
                continue;
            }

            // Deep penetration: the shapes interpenetrate beyond their margins, which GJK cannot resolve.
            // Fall back to SAT to find the minimum-penetration axis and compute accurate contact points.
            if (results[i] == GJKAlgorithm::GJKResult::Interpenetrate) {
                data.IsColliding = satAlgorithm.PerformCapsuleVsConvexPolyhedronCollisionCheck(batch, i);

                lastFrameCollisionData.WasUsingGJKAlgorithm = false;
                lastFrameCollisionData.WasUsingSATAlgorithm = true;

                if (data.IsColliding) {
                    collisionDetected = true;
                }

                // GJKResult::Separated — the shapes are not in contact. No action needed; data.IsColliding
                // remains false and collisionDetected is unchanged.
            }
        }

        return collisionDetected;
    }

} // namespace Vulkyrie
