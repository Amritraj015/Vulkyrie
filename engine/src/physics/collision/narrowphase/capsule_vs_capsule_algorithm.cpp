#include "physics/collision/narrowphase/capsule_vs_capsule_algorithm.h"
#include "physics/collision/shapes/capsule_shape.h"
#include "core/utilities.h"

namespace Vulkyrie {

    bool CapsuleVsCapsuleAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = narrowPhaseDataBatch.Data[i];

            VASSERT(data.ContactPointCount == 0, "Contact points should be cleared before performing collision checks.");
            VASSERT(!data.IsColliding, "Collision state should be false before performing collision checks.");

            // Get the transform from capsule 1 local-space to capsule 2 local-space.
            const TransformComponent capsuleOneToTwoTransform = data.ShapeTwoWorldTransform.Inverse() * data.ShapeOneWorldTransform;

            const auto *capsuleOne = static_cast<const CapsuleShape *>(&data.ShapeOne);
            const auto *capsuleTwo = static_cast<const CapsuleShape *>(&data.ShapeTwo);

            const f32 capsuleOneRadius = capsuleOne->GetRadius();
            const f32 capsuleOneHeight = capsuleOne->GetHeight();
            const f32 capsuleTwoRadius = capsuleTwo->GetRadius();
            const f32 capsuleTwoHeight = capsuleTwo->GetHeight();

            // Compute the end-points of the inner segment of capsule 1 in capsule 2's local space.
            const f32 capsuleOneHalfHeight = capsuleOneHeight * 0.5f;
            const glm::vec3 capsuleOneSegmentStart = capsuleOneToTwoTransform * glm::vec3(0.0f, -capsuleOneHalfHeight, 0.0f);
            const glm::vec3 capsuleOneSegmentEnd = capsuleOneToTwoTransform * glm::vec3(0.0f, capsuleOneHalfHeight, 0.0f);

            // Compute the end-points of the inner segment of capsule 2 in its own local space.
            const f32 capsuleTwoHalfHeight = capsuleTwoHeight * 0.5f;
            const glm::vec3 capsuleTwoSegmentStart(0.0f, -capsuleTwoHalfHeight, 0.0f);
            const glm::vec3 capsuleTwoSegmentEnd(0.0f, capsuleTwoHalfHeight, 0.0f);

            const glm::vec3 segmentOne = capsuleOneSegmentEnd - capsuleOneSegmentStart;
            const glm::vec3 segmentTwo = capsuleTwoSegmentEnd - capsuleTwoSegmentStart;

            const f32 radiusSum = capsuleOneRadius + capsuleTwoRadius;
            const bool segmentsAreParallel = AreParallelVectors(segmentOne, segmentTwo);

            // If the two capsule inner segments are parallel, we can generate two contact points.
            if (segmentsAreParallel) {

                // Compute the perpendicular (squared) distance between the two infinite lines containing the segments.
                const f32 segmentDistanceSquared = ComputeDistanceSquaredPointToLine(capsuleOneSegmentStart, capsuleOneSegmentEnd, capsuleTwoSegmentStart);

                // If the perpendicular distance is larger than the sum of radii, there is no overlap.
                if (segmentDistanceSquared >= radiusSum * radiusSum) {
                    continue;
                }

                // Compute the planes that go through the extreme points of capsule 1's inner segment.
                const f32 d1 = glm::dot(segmentOne, capsuleOneSegmentStart);
                const f32 d2 = -glm::dot(segmentOne, capsuleOneSegmentEnd);

                // Clip capsule 2's inner segment with the two planes through capsule 1's endpoints.
                f32 t1 = ComputePlaneSegmentIntersection(capsuleTwoSegmentEnd, capsuleTwoSegmentStart, d1, segmentOne);
                f32 t2 = ComputePlaneSegmentIntersection(capsuleTwoSegmentStart, capsuleTwoSegmentEnd, d2, -segmentOne);

                // If the segments were overlapping (the clipped segment is valid).
                if (t1 > 0.0f && t2 > 0.0f) {

                    if (data.ReportContacts) {
                        if (t1 > 1.0f) t1 = 1.0f;
                        const glm::vec3 clipPointA = capsuleTwoSegmentEnd - t1 * segmentTwo;
                        if (t2 > 1.0f) t2 = 1.0f;
                        const glm::vec3 clipPointB = capsuleTwoSegmentStart + t2 * segmentTwo;

                        // Project capsule 2's segment start onto the line of capsule 1's inner segment.
                        const glm::vec3 segmentOneNormalized = glm::normalize(segmentOne);
                        const glm::vec3 pointOnSegmentOne =
                            capsuleOneSegmentStart + glm::dot(segmentOneNormalized, capsuleTwoSegmentStart - capsuleOneSegmentStart) * segmentOneNormalized;

                        glm::vec3 normalCapsuleTwoSpaceNormalized;
                        glm::vec3 segmentOneToTwo;

                        if (segmentDistanceSquared > VE_MACHINE_EPSILON * VE_MACHINE_EPSILON) {
                            // Compute a perpendicular vector from segment 1 to segment 2.
                            segmentOneToTwo = capsuleTwoSegmentStart - pointOnSegmentOne;
                            normalCapsuleTwoSpaceNormalized = glm::normalize(segmentOneToTwo);
                        } else {
                            // The inner segments are overlapping (degenerate case): use any axis
                            // perpendicular to the segment direction as the contact normal.
                            const glm::vec3 vec1(1, 0, 0);
                            const glm::vec3 vec2(0, 1, 0);

                            const glm::vec3 segmentTwoNormalized = glm::normalize(segmentTwo);

                            // Pick the axis most orthogonal to segment 2 (smallest absolute dot product).
                            const f32 cosA1 = std::abs(segmentTwoNormalized.x); // abs(vec1.dot(seg2))
                            const f32 cosA2 = std::abs(segmentTwoNormalized.y); // abs(vec2.dot(seg2))

                            segmentOneToTwo = glm::vec3(0.0f);
                            normalCapsuleTwoSpaceNormalized = cosA1 < cosA2 ? glm::cross(segmentTwoNormalized, vec1) : glm::cross(segmentTwoNormalized, vec2);
                        }

                        const TransformComponent capsuleTwoToOneTransform = capsuleOneToTwoTransform.Inverse();
                        const glm::vec3 contactPointACapsuleOneLocal =
                            capsuleTwoToOneTransform * (clipPointA - segmentOneToTwo + normalCapsuleTwoSpaceNormalized * capsuleOneRadius);
                        const glm::vec3 contactPointBCapsuleOneLocal =
                            capsuleTwoToOneTransform * (clipPointB - segmentOneToTwo + normalCapsuleTwoSpaceNormalized * capsuleOneRadius);
                        const glm::vec3 contactPointACapsuleTwoLocal = clipPointA - normalCapsuleTwoSpaceNormalized * capsuleTwoRadius;
                        const glm::vec3 contactPointBCapsuleTwoLocal = clipPointB - normalCapsuleTwoSpaceNormalized * capsuleTwoRadius;

                        const f32 penetrationDepth = radiusSum - std::sqrt(segmentDistanceSquared);

                        const glm::vec3 normalWorld = data.ShapeTwoWorldTransform.Rotation * normalCapsuleTwoSpaceNormalized;

                        narrowPhaseDataBatch.AddContactPoint(i, normalWorld, penetrationDepth, contactPointACapsuleOneLocal, contactPointACapsuleTwoLocal);
                        narrowPhaseDataBatch.AddContactPoint(i, normalWorld, penetrationDepth, contactPointBCapsuleOneLocal, contactPointBCapsuleTwoLocal);
                    }

                    data.IsColliding = true;
                    collisionDetected = true;
                    continue;
                }
            }

            // Compute the closest points between the two inner capsule segments.
            glm::vec3 closestPointCapsuleOneSegment;
            glm::vec3 closestPointCapsuleTwoSegment;

            ComputeClosestPointBetweenTwoSegments(capsuleOneSegmentStart,
                                                  capsuleOneSegmentEnd,
                                                  capsuleTwoSegmentStart,
                                                  capsuleTwoSegmentEnd,
                                                  closestPointCapsuleOneSegment,
                                                  closestPointCapsuleTwoSegment);

            // Vector from the closest point on segment 1 to the closest point on segment 2.
            glm::vec3 closestPointSegmentOneToTwo = closestPointCapsuleTwoSegment - closestPointCapsuleOneSegment;
            const f32 closestPointsDistanceSquare = glm::length2(closestPointSegmentOneToTwo);

            // If the collision shapes overlap.
            if (closestPointsDistanceSquare < radiusSum * radiusSum) {

                if (data.ReportContacts) {

                    if (closestPointsDistanceSquare > VE_MACHINE_EPSILON) {
                        // General case: the inner segments have a well-defined separating direction.
                        const f32 closestPointsDistance = std::sqrt(closestPointsDistanceSquare);
                        closestPointSegmentOneToTwo /= closestPointsDistance;

                        const glm::vec3 contactPointCapsuleOneLocal =
                            capsuleOneToTwoTransform.Inverse() * (closestPointCapsuleOneSegment + closestPointSegmentOneToTwo * capsuleOneRadius);
                        const glm::vec3 contactPointCapsuleTwoLocal = closestPointCapsuleTwoSegment - closestPointSegmentOneToTwo * capsuleTwoRadius;

                        const glm::vec3 normalWorld = data.ShapeTwoWorldTransform.Rotation * closestPointSegmentOneToTwo;

                        const f32 penetrationDepth = std::max(radiusSum - closestPointsDistance, VE_MACHINE_EPSILON);

                        narrowPhaseDataBatch.AddContactPoint(i, normalWorld, penetrationDepth, contactPointCapsuleOneLocal, contactPointCapsuleTwoLocal);

                    } else {
                        // Degenerate case: the closest points on the inner segments coincide.
                        if (segmentsAreParallel) {
                            // Segments are parallel and their endpoints touch.
                            // Use the vector from the nearest extreme point of segment 1 to the
                            // closest point on segment 2 as the contact normal.
                            const f32 squareDistToSegStart = glm::length2(capsuleOneSegmentStart - closestPointCapsuleTwoSegment);
                            const glm::vec3 capsuleOneExtremePoint = squareDistToSegStart > VE_MACHINE_EPSILON ? capsuleOneSegmentStart : capsuleOneSegmentEnd;

                            const glm::vec3 normalCapsuleTwoSpace = glm::normalize(closestPointCapsuleTwoSegment - capsuleOneExtremePoint);

                            const glm::vec3 contactPointCapsuleOneLocal =
                                capsuleOneToTwoTransform.Inverse() * (closestPointCapsuleOneSegment + normalCapsuleTwoSpace * capsuleOneRadius);
                            const glm::vec3 contactPointCapsuleTwoLocal = closestPointCapsuleTwoSegment - normalCapsuleTwoSpace * capsuleTwoRadius;

                            const glm::vec3 normalWorld = data.ShapeTwoWorldTransform.Rotation * normalCapsuleTwoSpace;

                            narrowPhaseDataBatch.AddContactPoint(i, normalWorld, radiusSum, contactPointCapsuleOneLocal, contactPointCapsuleTwoLocal);

                        } else {
                            // Segments are not parallel and cross at a single point.
                            // Use the cross product of both segment directions as the contact normal.
                            const glm::vec3 normalCapsuleTwoSpace = glm::normalize(glm::cross(segmentOne, segmentTwo));

                            const glm::vec3 contactPointCapsuleOneLocal =
                                capsuleOneToTwoTransform.Inverse() * (closestPointCapsuleOneSegment + normalCapsuleTwoSpace * capsuleOneRadius);
                            const glm::vec3 contactPointCapsuleTwoLocal = closestPointCapsuleTwoSegment - normalCapsuleTwoSpace * capsuleTwoRadius;

                            const glm::vec3 normalWorld = data.ShapeTwoWorldTransform.Rotation * normalCapsuleTwoSpace;

                            narrowPhaseDataBatch.AddContactPoint(i, normalWorld, radiusSum, contactPointCapsuleOneLocal, contactPointCapsuleTwoLocal);
                        }
                    }
                }

                data.IsColliding = true;
                collisionDetected = true;
            }
        }

        return collisionDetected;
    }

} // namespace Vulkyrie