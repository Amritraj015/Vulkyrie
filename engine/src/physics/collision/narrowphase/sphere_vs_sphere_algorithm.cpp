#include "physics/collision/narrowphase/sphere_vs_sphere_algorithm.h"
#include "physics/collision/shapes/sphere_shape.h"
#include "core/constants.h"

namespace Vulkyrie {

    bool SphereVsSphereAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseDataBatch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        // Iterate over each pair in the batch.
        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = narrowPhaseDataBatch.Data[i];

            // Sanity checks to ensure data is in a valid state for collision checking.
            VASSERT(data.ContactPointCount == 0, "Contact points should be cleared before performing collision checks.");
            VASSERT(!data.IsColliding, "Collision state should be false before performing collision checks.");

            // Get world transforms for both shapes.
            const TransformComponent &shapeOneTransform = data.ShapeOneWorldTransform;
            const TransformComponent &shapeTwoTransform = data.ShapeTwoWorldTransform;

            // Compute the vector between sphere centers and its squared length.
            glm::vec3 vectorBetweenCenters = shapeTwoTransform.Position - shapeOneTransform.Position;
            f32 distanceSquared = glm::length2(vectorBetweenCenters);

            // Get the radii of the two spheres.
            const f32 sphereOneRadius = static_cast<const SphereShape *>(&data.ShapeOne)->GetRadius();
            const f32 sphereTwoRadius = static_cast<const SphereShape *>(&data.ShapeTwo)->GetRadius();

            // Compute the sum of radii and its square.
            const f32 radiusSum = sphereOneRadius + sphereTwoRadius;
            const f32 radiusSumSquared = radiusSum * radiusSum;

            // Check for intersection: if the squared distance is less than squared sum of radii, spheres overlap.
            if (distanceSquared < radiusSumSquared) {
                // Calculate penetration depth (how much the spheres overlap).
                const f32 penetrationDepth = radiusSum - glm::sqrt(distanceSquared);

                if (penetrationDepth > f32(0.0)) {
                    // If contact reporting is enabled, compute contact information.
                    if (data.ReportContacts) {
                        // Compute inverse transforms for both shapes (for local space contact points).
                        const TransformComponent transformOneInverse = shapeOneTransform.Inverse();
                        const TransformComponent transformTwoInverse = shapeTwoTransform.Inverse();

                        glm::vec3 intersectionBodyOne;
                        glm::vec3 intersectionBodyTwo;
                        glm::vec3 contactNormal(0.0f);

                        // If the centers are not extremely close, compute contact normal and intersection points normally.
                        if (distanceSquared > VE_MACHINE_EPSILON) {
                            // Transform the center of each sphere into the other's local space.
                            const glm::vec3 centerSphereTwoInBodyOneLocalSpace = transformOneInverse * shapeTwoTransform.Position;
                            const glm::vec3 centerSphereOneInBodyTwoLocalSpace = transformTwoInverse * shapeOneTransform.Position;

                            // Intersection points on the surface of each sphere in local space.
                            intersectionBodyOne = sphereOneRadius * glm::normalize(centerSphereTwoInBodyOneLocalSpace);
                            intersectionBodyTwo = sphereTwoRadius * glm::normalize(centerSphereOneInBodyTwoLocalSpace);

                            // Contact normal in world space (from sphere one to sphere two).
                            contactNormal = glm::normalize(vectorBetweenCenters);
                        } else {
                            // The centers are extremely close together (degenerate case).
                            // To avoid instability, use an arbitrary normal (e.g., world up).
                            contactNormal.y = 1.0f;

                            // Place intersection points along the chosen normal.
                            intersectionBodyOne = sphereOneRadius * transformOneInverse.Rotation * contactNormal;
                            intersectionBodyTwo = sphereTwoRadius * transformTwoInverse.Rotation * contactNormal;
                        }

                        // Add the computed contact point to the batch.
                        narrowPhaseDataBatch.AddContactPoint(i, contactNormal, penetrationDepth, intersectionBodyOne, intersectionBodyTwo);
                    }

                    // Mark collision as detected for this pair.
                    collisionDetected = data.IsColliding = true;
                }
            }
        }

        // Return true if any collision was detected in the batch.
        return collisionDetected;
    }

} // namespace Vulkyrie
