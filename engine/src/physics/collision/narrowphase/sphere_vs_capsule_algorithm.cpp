#include "physics/collision/narrowphase/sphere_vs_capsule_algorithm.h"
#include "physics/collision/shapes/sphere_shape.h"
#include "physics/collision/shapes/capsule_shape.h"
#include "core/utilities.h"

namespace Vulkyrie {

    bool SphereVsCapsuleAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = batch.Data[i];

            VASSERT(data.ContactPointCount == 0, "Contact points should be cleared before performing collision checks.");
            VASSERT(!data.IsColliding, "Collision state should be false before performing collision checks.");

            // Identify which shape is the sphere and which is the capsule.
            const bool isShapeOneSphere = CollisionShapeType::Sphere == data.ShapeOne.GetType();

            const auto *sphereShape = static_cast<const SphereShape *>(isShapeOneSphere ? &data.ShapeOne : &data.ShapeTwo);
            const auto *capsuleShape = static_cast<const CapsuleShape *>(isShapeOneSphere ? &data.ShapeTwo : &data.ShapeOne);

            const f32 sphereRadius = sphereShape->GetRadius();
            const f32 capsuleRadius = capsuleShape->GetRadius();

            // Get the world transforms for both shapes.
            const TransformComponent &sphereWorldTransform = isShapeOneSphere ? data.ShapeOneToWorldTransform : data.ShapeTwoToWorldTransform;
            const TransformComponent &capsuleWorldTransform = isShapeOneSphere ? data.ShapeTwoToWorldTransform : data.ShapeOneToWorldTransform;

            // Transform the sphere center into the capsule's local space.
            const TransformComponent worldToCapsuleTransform = capsuleWorldTransform.Inverse();
            const TransformComponent sphereToCapsuleLocalSpace = worldToCapsuleTransform * sphereWorldTransform;

            const glm::vec3 sphereCenter = sphereToCapsuleLocalSpace.Position;

            // Compute the end-points of the capsule's inner segment (along the Y axis in local space).
            const f32 halfCapsuleHeight = capsuleShape->GetHalfHeight();
            const glm::vec3 capsuleSegmentStart(0.0f, -halfCapsuleHeight, 0.0f);
            const glm::vec3 capsuleSegmentEnd(0.0f, halfCapsuleHeight, 0.0f);

            // Compute the closest point on the capsule's inner segment to the sphere center.
            const glm::vec3 closestPointOnCapsuleSegment = ComputeClosestPointOnLineSegment(capsuleSegmentStart, capsuleSegmentEnd, sphereCenter);

            // Compute the squared distance between the sphere center and the closest point on the segment.
            glm::vec3 vectorBetweenSphereAndCapsule = closestPointOnCapsuleSegment - sphereCenter;
            const f32 distanceSquared = glm::length2(vectorBetweenSphereAndCapsule);

            // The shapes overlap if the distance is less than the sum of their radii.
            const f32 radiusSum = sphereRadius + capsuleRadius;

            if (distanceSquared < radiusSum * radiusSum) {
                if (data.ReportContacts) {
                    f32 penetrationDepth;
                    glm::vec3 contactNormal;
                    glm::vec3 contactPointSphereLocal;
                    glm::vec3 contactPointCapsuleLocal;

                    if (distanceSquared > VE_K_MACHINE_EPSILON) {
                        // General case: sphere center is not on the capsule inner segment.
                        const f32 distance = glm::sqrt(distanceSquared);
                        vectorBetweenSphereAndCapsule = glm::normalize(vectorBetweenSphereAndCapsule);

                        penetrationDepth = radiusSum - distance;
                        contactPointSphereLocal = sphereToCapsuleLocalSpace.Inverse() * (sphereCenter + vectorBetweenSphereAndCapsule * sphereRadius);
                        contactPointCapsuleLocal = closestPointOnCapsuleSegment - vectorBetweenSphereAndCapsule * capsuleRadius;

                        // Contact normal points from the capsule surface toward the sphere, in world space.
                        contactNormal = capsuleWorldTransform.Rotation * vectorBetweenSphereAndCapsule;

                        if (!isShapeOneSphere) {
                            contactNormal = -contactNormal;
                        }

                    } else {

                        // Sphere center is on the capsule inner segment (degenerate case)
                        // The sphere center lies exactly on the capsule axis, so there is no well-defined
                        // separating direction. Use any axis perpendicular to the inner segment instead.

                        // Capsule inner segment.
                        const glm::vec3 capsuleSegment = glm::normalize(capsuleSegmentEnd - capsuleSegmentStart);

                        const glm::vec3 vec1(1, 0, 0);
                        const glm::vec3 vec2(0, 1, 0);

                        // Pick whichever of vec1/vec2 is most orthogonal to the segment (smallest absolute dot product).
                        const f32 cosA1 = std::abs(capsuleSegment.x); // abs(vec1.dot(seg))
                        const f32 cosA2 = std::abs(capsuleSegment.y); // abs(vec2.dot(seg))

                        penetrationDepth = radiusSum;

                        // Cross the segment with the most orthogonal axis to get a perpendicular contact normal.
                        const glm::vec3 normalCapsuleSpace = cosA1 < cosA2 ? glm::cross(capsuleSegment, vec1) : glm::cross(capsuleSegment, vec2);
                        contactNormal = capsuleWorldTransform.Rotation * normalCapsuleSpace;

                        // Compute the two local contact points.
                        contactPointSphereLocal = sphereToCapsuleLocalSpace.Inverse() * (sphereCenter + normalCapsuleSpace * sphereRadius);
                        contactPointCapsuleLocal = sphereCenter - normalCapsuleSpace * capsuleRadius;
                    }

                    // If the penetration depth is negative or zero,
                    // this means the shapes are just touching or separated,
                    // so we skip contact generation.
                    if (penetrationDepth <= 0.0f) {
                        continue;
                    }

                    // Add the computed contact point to the batch.
                    batch.AddContactPoint(i,
                                          contactNormal,
                                          penetrationDepth,
                                          isShapeOneSphere ? contactPointSphereLocal : contactPointCapsuleLocal,
                                          isShapeOneSphere ? contactPointCapsuleLocal : contactPointSphereLocal);
                }

                collisionDetected = true;
                data.IsColliding = true;
            }
        }

        return collisionDetected;
    }

} // namespace Vulkyrie
