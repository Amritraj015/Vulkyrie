#include "physics/collision/narrowphase/gjk/gjk_algorithm.h"
#include "core/constants.h"
#include "physics/collision/narrowphase/gjk/voronoi_simplex.h"
#include "physics/collision/shapes/convex_shape.h"
#include "physics/collision/shapes/triangle_shape.h"

namespace Vulkyrie {

    // Relative error tolerance for GJK convergence checks
    // Used to determine when the distance improvement is negligible
    constexpr f32 REL_ERROR = f32(1.0e-3);
    constexpr f32 REL_ERROR_SQUARE = REL_ERROR * REL_ERROR;

    // Maximum iterations for GJK raycast (currently unused)
    // constexpr i32 MAX_ITERATIONS_GJK_RAYCAST = 32;

    void GJKAlgorithm::PerformCollisionCheck(NarrowPhaseDataBatch &narrowPhaseInfoBatch,
                                             size_t batchStartIndex,
                                             size_t totalBatchItems,
                                             std::vector<GJKResult> &gjkResults) {
        // Process each collision pair in the batch
        for (size_t i = batchStartIndex; i < batchStartIndex + totalBatchItems; i++) {
            // ===== Local Variables for GJK Algorithm =====

            glm::vec3 supportPointA;         // Support point on shape A (in A's local space)
            glm::vec3 supportPointB;         // Support point on shape B (transformed to A's local space)
            glm::vec3 minkowskiSupportPoint; // Support point on Minkowski difference A-B
            glm::vec3 closestPointObjectA;   // Closest point on object A (with margin)
            glm::vec3 closestPointObjectB;   // Closest point on object B (with margin)
            f32 supportProjection;           // Projection of support point onto search direction (v·w)
            f32 previousDistanceSquared;     // Previous iteration's squared distance (for convergence check)
            bool collisionDetected = false;  // Flag indicating if collision/contact was found

            // ===== Retrieve Collision Pair Data =====

            NarrowPhaseData &data = narrowPhaseInfoBatch.Data[i];

            // GJK only works with convex shapes
            VASSERT(data.ShapeOne.IsConvex() && data.ShapeTwo.IsConvex(), "GJK algorithm only supports convex shapes.");

            const ConvexShape *shapeOne = static_cast<ConvexShape *>(&data.ShapeOne);
            const ConvexShape *shapeTwo = static_cast<ConvexShape *>(&data.ShapeTwo);

            // ===== Setup Coordinate Transformations =====

            // GJK operates in the local space of object A for numerical stability
            const TransformComponent &transformOne = data.ShapeOneWorldTransform;
            const TransformComponent &transformTwo = data.ShapeTwoWorldTransform;

            // Compute transformation from world space to object A's local space
            const TransformComponent transformOneInverse = transformOne.Inverse();

            // Compute transformation from object B's local space to object A's local space
            const TransformComponent transformTwoToOne = transformTwo * transformOneInverse;

            // Quaternion to rotate directions from object A's local space to object B's local space
            // This is used when querying support points on shape B
            glm::quat rotationOneToTwo = glm::conjugate(transformTwo.Rotation) * transformOne.Rotation;

            // ===== Compute Collision Margins =====

            // Total margin is the sum of both shapes' margins
            // Margins provide numerical stability and allow for shallow penetration detection
            f32 margin = shapeOne->GetMargin() + shapeTwo->GetMargin();
            f32 marginSquared = margin * margin;

            VASSERT(margin > 0.0f, "GJK algorithm requires positive margins for numerical stability.");

            // ===== Initialize Simplex and Frame Coherence =====

            VoronoiSimplex simplex; // Simplex structure to track closest feature
            LastFrameCollisionData &lastFrameData = data.LastFrameCollisionInfo;

            // Initial search direction 'v' points from origin toward closest point on Minkowski difference
            glm::vec3 v;

            // Use cached separating axis from previous frame if available (frame coherence optimization)
            if (lastFrameData.IsValid && lastFrameData.WasUsingGJKAlgorithm) {
                v = lastFrameData.GJKSeparatingAxis;

                VASSERT(glm::length2(v) > VE_MACHINE_EPSILON, "Invalid separating axis from last frame: length is too small.");
            } else {
                // If no cached data, use arbitrary initial direction (upward)
                v = glm::vec3(0.0f, 1.0f, 0.0f);
            }

            // Initialize squared distance to maximum (will converge downward)
            f32 distanceSquared = std::numeric_limits<f32>::max();
            bool noIntersection = false;

            // ===== Main GJK Iteration Loop =====

            do {
                // ----- Step 1: Compute Support Points -----

                // Get support point on shape A in direction -v (farthest point in opposite direction)
                supportPointA = shapeOne->GetLocalSupportPointWithoutMargin(-v);

                // Get support point on shape B in direction v
                // Note: Direction must be transformed to B's local space using rotationOneToTwo
                // Result must be transformed to A's local space using transformTwoToOne
                supportPointB = transformTwoToOne * shapeTwo->GetLocalSupportPointWithoutMargin(rotationOneToTwo * v);

                // Compute support point on Minkowski difference A-B
                // This is the farthest point on (A-B) in direction v
                minkowskiSupportPoint = supportPointA - supportPointB;

                // ----- Step 2: Early Separation Test -----

                // Compute projection of support point onto search direction
                supportProjection = glm::dot(v, minkowskiSupportPoint);

                // If v·w > 0, the origin is outside the Minkowski difference
                // Additionally check if distance exceeds the margin threshold
                if (supportProjection > 0.0f && supportProjection * supportProjection > distanceSquared * marginSquared) {

                    // Cache the current separating axis for next frame (frame coherence)
                    lastFrameData.GJKSeparatingAxis = v;

                    // No intersection, we return
                    VASSERT(gjkResults.size() == i, "GJK results vector size must be equal to i");

                    gjkResults.push_back(GJKResult::Separated);
                    noIntersection = true;

                    break;
                }

                // ----- Step 3: Check for Convergence -----

                // Check if support point is already in simplex (duplicate point)
                // OR if distance improvement is negligible (convergence reached)
                if (simplex.IsPointInSimplex(minkowskiSupportPoint) || distanceSquared - supportProjection <= distanceSquared * REL_ERROR_SQUARE) {

                    // Objects are touching/penetrating - contact point has been found
                    collisionDetected = true;
                    break;
                }

                // ----- Step 4: Add Support Point to Simplex -----

                // Add the new support point to the simplex along with the original support points
                simplex.AddPoint(minkowskiSupportPoint, supportPointA, supportPointB);

                // ----- Step 5: Check for Degenerate Simplex -----

                // A simplex is affinely dependent if its points are coplanar (for 4 points)
                // or collinear (for 3 points), indicating numerical issues or special geometry
                if (simplex.IsAffinelyDependent()) {
                    // Degenerate case - treat as contact
                    collisionDetected = true;

                    break;
                }

                // ----- Step 6: Compute Closest Point -----

                // Find the point on the simplex closest to the origin
                // This becomes the new search direction v
                // Returns false if computation fails (indicates contact)
                if (!simplex.ComputeClosestPoint(v)) {
                    // Failed to compute closest point - objects must be touching
                    collisionDetected = true;

                    break;
                }

                // ----- Step 7: Check for Sufficient Progress -----

                // Store previous distance for convergence check
                previousDistanceSquared = distanceSquared;
                distanceSquared = glm::length2(v);

                // If distance improvement is too small, we've converged
                // This prevents infinite loops when progress stalls due to numerical precision
                if (previousDistanceSquared - distanceSquared <= VE_MACHINE_EPSILON * previousDistanceSquared) {
                    // Back up the closest point from simplex
                    simplex.BackupClosestPointInSimplex(v);

                    // Recompute squared distance with backed-up point
                    distanceSquared = glm::length2(v);

                    // Treat as contact
                    collisionDetected = true;

                    break;
                }

            } while (!simplex.IsFull() && distanceSquared > VE_MACHINE_EPSILON * simplex.GetMaxLengthSquareOfAPoint());
            // Loop continues while:
            // 1. Simplex has room for more points (not full = less than 4 vertices)
            // 2. Distance to origin is above epsilon threshold (relative to simplex size)

            // ===== Post-GJK Processing =====

            // If objects are separated, skip to next pair
            if (noIntersection) {
                continue;
            }

            // ----- Handle Shallow Penetration (Contact in Margin) -----

            if (collisionDetected && distanceSquared > VE_MACHINE_EPSILON) {
                // Objects are close but not deeply penetrating

                // Compute the closest points on both objects (without margins)
                simplex.ComputeClosestPointsOfAandB(closestPointObjectA, closestPointObjectB);

                // Project closest points onto the margin surfaces
                // This gives the actual contact points on the enlarged (with margin) shapes
                f32 dist = std::sqrt(distanceSquared);
                VASSERT(dist > 0.0f, "Distance must be greater than zero for valid contact point projection.");

                // Move point A inward by margin amount along direction v
                closestPointObjectA = (closestPointObjectA - (shapeOne->GetMargin() / dist) * v);

                // Move point B outward by margin amount along direction v
                // Transform back to world space (or shape B's space)
                closestPointObjectB = transformTwoToOne.Inverse() * (closestPointObjectB + (shapeTwo->GetMargin() / dist) * v);

                // ----- Compute Contact Information -----

                // Contact normal points from B to A (normalized -v direction in world space)
                glm::vec3 normal = transformOne.Rotation * glm::normalize(-v);

                // Penetration depth is how much the margins overlap
                f32 penetrationDepth = margin - dist;

                // Reject contact if penetration is negative (numerical error)
                if (penetrationDepth <= 0.0f) {
                    VASSERT(gjkResults.size() == i, "GJK results vector size must be equal to i");
                    gjkResults.push_back(GJKResult::Separated);

                    continue;
                }

                // Reject contact if normal is degenerate (zero length)
                if (glm::length2(normal) < VE_MACHINE_EPSILON) {
                    VASSERT(gjkResults.size() == i, "GJK results vector size must be equal to i");
                    gjkResults.push_back(GJKResult::Separated);

                    continue;
                }

                // ----- Generate Contact Point -----

                if (data.ReportContacts) {

                    // Apply triangle mesh smoothing if one shape is a triangle
                    // This prevents bumpy collisions on triangle meshes by interpolating normals
                    TriangleShape::ComputeSmoothTriangleMeshContact(
                        shapeOne, shapeTwo, closestPointObjectA, closestPointObjectB, transformOne, transformTwo, penetrationDepth, normal);

                    // Add the contact point to the narrow phase results
                    narrowPhaseInfoBatch.AddContactPoint(i, normal, penetrationDepth, closestPointObjectA, closestPointObjectB);
                }

                VASSERT(gjkResults.size() == i, "GJK results vector size must be equal to i");
                gjkResults.push_back(GJKResult::CollideInMargin);

                continue;
            }

            // ----- Handle Deep Penetration -----

            // If we reach here, the objects are deeply interpenetrating (even without margins)
            // This requires EPA (Expanding Polytope Algorithm) or SAT for accurate contact resolution
            VASSERT(gjkResults.size() == i, "GJK results vector size does not match batch index. Expected size: {}, actual size: {}.", i, gjkResults.size());
            gjkResults.push_back(GJKResult::Interpenetrate);
        }
    }

} // namespace Vulkyrie
