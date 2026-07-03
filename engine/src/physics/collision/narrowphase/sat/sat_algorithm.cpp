#include "physics/collision/narrowphase/sat/sat_algorithm.h"
#include "core/utilities.h"
#include "physics/collision/shapes/triangle_shape.h"
#include <limits>

namespace Vulkyrie {

    SATAlgorithm::SATAlgorithm(bool clipWithPreviousAxisIfStillColliding)
        : _clipWithPreviousAxisIfStillColliding(clipWithPreviousAxisIfStillColliding) {
    }

    bool SATAlgorithm::PerformSphereVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = batch.Data[i];

            const CollisionShape *shapeOne = &data.ShapeOne;
            const CollisionShape *shapeTwo = &data.ShapeTwo;
            const bool isShapeOneSphere = shapeOne->GetType() == CollisionShapeType::Sphere;

            VASSERT((shapeOne->GetType() == CollisionShapeType::Sphere && shapeTwo->GetType() == CollisionShapeType::ConvexPolyhedron) ||
                        (shapeTwo->GetType() == CollisionShapeType::Sphere && shapeOne->GetType() == CollisionShapeType::ConvexPolyhedron),
                    "SATAlgorithm::PerformSphereVsConvexPolyhedronCollisionCheck only supports Sphere vs Convex Polyhedron collision checks.");

            // Resolve which shape is the sphere and which is the polyhedron regardless of the pair order.
            const auto *sphereShape = static_cast<const SphereShape *>(isShapeOneSphere ? shapeOne : shapeTwo);
            const auto *polyhedronShape = static_cast<const ConvexPolyhedronShape *>(isShapeOneSphere ? shapeTwo : shapeOne);

            const TransformComponent &sphereToWorldTransform = isShapeOneSphere ? data.ShapeOneToWorldTransform : data.ShapeTwoToWorldTransform;
            const TransformComponent &polyhedronToWorldTransform = isShapeOneSphere ? data.ShapeTwoToWorldTransform : data.ShapeOneToWorldTransform;

            // Transform the sphere center into polyhedron local space so all per-face tests can be done
            // without repeatedly applying the world transform inside the loop.
            const TransformComponent worldToPolyhedronTransform = polyhedronToWorldTransform.Inverse();
            const TransformComponent sphereToPolyhedronTransform = worldToPolyhedronTransform * sphereToWorldTransform;

            const glm::vec3 &sphereCenter = sphereToPolyhedronTransform.Position;

            f32 minPenetrationDepth = std::numeric_limits<f32>::max();
            size_t minFaceIndex = 0;
            bool noContact = false;

            // Test every face normal as a candidate separating axis.
            // A non-positive penetration depth means this face normal separates the shapes — early out immediately.
            for (size_t f = 0; f < polyhedronShape->GetFacesCount(); ++f) {
                f32 penetrationDepth = computePolyhedronFaceVsSpherePenetrationDepth(f, *polyhedronShape, *sphereShape, sphereCenter);

                if (penetrationDepth <= 0.0f) {
                    noContact = true;
                    break;
                }

                if (penetrationDepth < minPenetrationDepth) {
                    minPenetrationDepth = penetrationDepth;
                    minFaceIndex = f;
                }
            }

            if (noContact) {
                continue;
            }

            // All face normals show penetration — the shapes are overlapping. Build contact information.
            if (data.ReportContacts) {
                const glm::vec3 minFaceNormal = polyhedronShape->GetFaceNormal(minFaceIndex);
                const glm::vec3 minFaceNormalWorld = polyhedronToWorldTransform.Rotation * minFaceNormal;

                // Contact point on the sphere surface in sphere local space: move from center by radius in the
                // direction of the face normal (the face normal points into the sphere at the contact).
                glm::vec3 contactPointSphereLocal = sphereToWorldTransform.Inverse().Rotation * (-minFaceNormalWorld * sphereShape->GetRadius());

                // Contact point on the polyhedron surface in polyhedron local space: move inward from the sphere
                // center by the amount the sphere protrudes past the face.
                glm::vec3 contactPointPolyhedronLocal = sphereCenter + minFaceNormal * (minPenetrationDepth - sphereShape->GetRadius());

                // The contact normal points from the polyhedron toward the sphere (i.e. toward shape one when shape one is the sphere).
                glm::vec3 contactNormal = isShapeOneSphere ? -minFaceNormalWorld : minFaceNormalWorld;

                TriangleShape::ComputeSmoothTriangleMeshContact(&data.ShapeOne,
                                                                &data.ShapeTwo,
                                                                isShapeOneSphere ? contactPointSphereLocal : contactPointPolyhedronLocal,
                                                                isShapeOneSphere ? contactPointPolyhedronLocal : contactPointSphereLocal,
                                                                data.ShapeOneToWorldTransform,
                                                                data.ShapeTwoToWorldTransform,
                                                                minPenetrationDepth,
                                                                contactNormal);

                batch.AddContactPoint(i,
                                      contactNormal,
                                      minPenetrationDepth,
                                      isShapeOneSphere ? contactPointSphereLocal : contactPointPolyhedronLocal,
                                      isShapeOneSphere ? contactPointPolyhedronLocal : contactPointSphereLocal

                );
            }

            data.IsColliding = true;
            collisionDetected = true;
        }

        return collisionDetected;
    }

    bool SATAlgorithm::PerformCapsuleVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchIndex) {
        NarrowPhaseData &data = batch.Data[batchIndex];

        VASSERT((data.ShapeOne.GetType() == CollisionShapeType::Capsule && data.ShapeTwo.GetType() == CollisionShapeType::ConvexPolyhedron) ||
                    (data.ShapeTwo.GetType() == CollisionShapeType::Capsule && data.ShapeOne.GetType() == CollisionShapeType::ConvexPolyhedron),
                "SATAlgorithm::PerformCapsuleVsConvexPolyhedronCollisionCheck only supports Capsule vs Convex Polyhedron collision checks.");

        // Resolve which shape is the capsule and which is the polyhedron regardless of the pair order.
        const bool isShapeOneCapsule = data.ShapeOne.GetType() == CollisionShapeType::Capsule;
        const auto *capsuleShape = static_cast<const CapsuleShape *>(isShapeOneCapsule ? &data.ShapeOne : &data.ShapeTwo);
        const auto *polyhedronShape = static_cast<const ConvexPolyhedronShape *>(isShapeOneCapsule ? &data.ShapeTwo : &data.ShapeOne);

        const TransformComponent &capsuleToWorld = isShapeOneCapsule ? data.ShapeOneToWorldTransform : data.ShapeTwoToWorldTransform;
        const TransformComponent &polyhedronToWorld = isShapeOneCapsule ? data.ShapeTwoToWorldTransform : data.ShapeOneToWorldTransform;
        const TransformComponent polyhedronToCapsule = capsuleToWorld.Inverse() * polyhedronToWorld;

        // The capsule inner segment lies along the Y axis in capsule local space.
        const glm::vec3 capsuleSegmentStart(0.0f, -capsuleShape->GetHalfHeight(), 0.0f);
        const glm::vec3 capsuleSegmentEnd(0.0f, capsuleShape->GetHalfHeight(), 0.0f);
        const glm::vec3 capsuleSegmentAxis = capsuleSegmentEnd - capsuleSegmentStart;

        f32 minPenetrationDepth = std::numeric_limits<f32>::max();
        size_t minFaceIndex = 0;
        bool isMinPenetrationFaceNormal = false;
        glm::vec3 separatingAxisCapsuleSpace;
        glm::vec3 separatingPolyhedronEdgeVertexOne;
        glm::vec3 separatingPolyhedronEdgeVertexTwo;

        // Test every face normal as a candidate separating axis.
        // A non-positive penetration depth means this face normal separates the shapes — early out immediately.
        for (size_t f = 0; f < polyhedronShape->GetFacesCount(); ++f) {
            glm::vec3 outFaceNormalCapsuleSpace;

            const f32 penetrationDepth =
                computePolyhedronFaceVsCapsulePenetrationDepth(f, *polyhedronShape, *capsuleShape, polyhedronToCapsule, outFaceNormalCapsuleSpace);

            if (penetrationDepth <= 0.0f) {
                return false;
            }

            if (penetrationDepth < minPenetrationDepth) {
                minPenetrationDepth = penetrationDepth;
                minFaceIndex = f;
                isMinPenetrationFaceNormal = true;
                separatingAxisCapsuleSpace = outFaceNormalCapsuleSpace;
            }
        }

        // Test each unique polyhedron edge against the capsule inner segment. Iterate with step 2 to visit
        // only one half-edge per geometric edge and avoid testing each edge twice via its twin.
        // The Gauss Map test filters out edge pairs whose cross product cannot be a separating axis.
        for (size_t e = 0; e < polyhedronShape->GetHalfEdgesCount(); e += 2) {
            const HalfEdgeMesh::Edge &edge = polyhedronShape->GetHalfEdge(e);
            const glm::vec3 edgeVertexOne = polyhedronShape->GetVertexPosition(edge.StartVertexIndex);
            const glm::vec3 edgeVertexTwo = polyhedronShape->GetVertexPosition(polyhedronShape->GetHalfEdge(edge.NextEdgeIndex).StartVertexIndex);

            const HalfEdgeMesh::Edge &twinEdge = polyhedronShape->GetHalfEdge(edge.TwinEdgeIndex);
            const glm::vec3 &adjacentFaceOneNormal = polyhedronToCapsule.Rotation * polyhedronShape->GetFaceNormal(edge.FaceIndex);
            const glm::vec3 &adjacentFaceTwoNormal = polyhedronToCapsule.Rotation * polyhedronShape->GetFaceNormal(twinEdge.FaceIndex);

            if (IsMinkowskiFaceCapsuleVsEdge(capsuleSegmentAxis, adjacentFaceOneNormal, adjacentFaceTwoNormal)) {
                const glm::vec3 edgeDirectionCapsuleSpace = polyhedronToCapsule.Rotation * (edgeVertexTwo - edgeVertexOne);

                glm::vec3 outSeparatingAxis;

                const f32 penetrationDepth = computeEdgeVsCapsuleInnerSegmentPenetrationDepth(
                    *polyhedronShape, *capsuleShape, capsuleSegmentAxis, edgeVertexOne, edgeDirectionCapsuleSpace, polyhedronToCapsule, outSeparatingAxis);

                if (penetrationDepth <= 0.0f) {
                    return false;
                }

                if (penetrationDepth < minPenetrationDepth) {
                    minPenetrationDepth = penetrationDepth;
                    isMinPenetrationFaceNormal = false;
                    separatingAxisCapsuleSpace = outSeparatingAxis;
                    separatingPolyhedronEdgeVertexOne = edgeVertexOne;
                    separatingPolyhedronEdgeVertexTwo = edgeVertexTwo;
                }
            }
        }

        // No separating axis found — the shapes are overlapping. Convert the capsule segment endpoints to
        // polyhedron local space and the separating axis (which is in capsule space) to world space.
        const TransformComponent capsuleToPolyhedron = polyhedronToCapsule.Inverse();
        const glm::vec3 capsuleSegmentStartPolyhedronSpace = capsuleToPolyhedron * capsuleSegmentStart;
        const glm::vec3 capsuleSegmentEndPolyhedronSpace = capsuleToPolyhedron * capsuleSegmentEnd;
        glm::vec3 contactNormal = capsuleToWorld.Rotation * separatingAxisCapsuleSpace;

        // The contact normal must point from shape two toward shape one.
        if (isShapeOneCapsule) {
            contactNormal = -contactNormal;
        }

        const f32 capsuleRadius = capsuleShape->GetRadius();

        if (isMinPenetrationFaceNormal) {
            if (data.ReportContacts) {
                return ComputeCapsulePolyhedronFaceContactPoints(minFaceIndex,
                                                                 capsuleRadius,
                                                                 *polyhedronShape,
                                                                 minPenetrationDepth,
                                                                 polyhedronToCapsule,
                                                                 contactNormal,
                                                                 separatingAxisCapsuleSpace,
                                                                 capsuleSegmentStartPolyhedronSpace,
                                                                 capsuleSegmentEndPolyhedronSpace,
                                                                 batch,
                                                                 batchIndex,
                                                                 isShapeOneCapsule);
            }
        } else {
            if (data.ReportContacts) {

                glm::vec3 closestPointPolyhedronEdge;
                glm::vec3 closestPointCapsuleSegment;

                ComputeClosestPointBetweenTwoSegments(capsuleSegmentStartPolyhedronSpace,
                                                      capsuleSegmentEndPolyhedronSpace,
                                                      separatingPolyhedronEdgeVertexOne,
                                                      separatingPolyhedronEdgeVertexTwo,
                                                      closestPointCapsuleSegment,
                                                      closestPointPolyhedronEdge);

                glm::vec3 contactPointCapsule = (polyhedronToCapsule * closestPointCapsuleSegment) - separatingAxisCapsuleSpace * capsuleRadius;

                TriangleShape::ComputeSmoothTriangleMeshContact(&data.ShapeOne,
                                                                &data.ShapeTwo,
                                                                isShapeOneCapsule ? contactPointCapsule : closestPointPolyhedronEdge,
                                                                isShapeOneCapsule ? closestPointPolyhedronEdge : contactPointCapsule,
                                                                data.ShapeOneToWorldTransform,
                                                                data.ShapeTwoToWorldTransform,
                                                                minPenetrationDepth,
                                                                contactNormal);

                batch.AddContactPoint(batchIndex,
                                      contactNormal,
                                      minPenetrationDepth,
                                      isShapeOneCapsule ? contactPointCapsule : closestPointPolyhedronEdge,
                                      isShapeOneCapsule ? closestPointPolyhedronEdge : contactPointCapsule);
            }
        }

        return true;
    }

    bool SATAlgorithm::PerformConvexPolyhedronVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        for (size_t i = batchStartIndex; i < batchStartIndex + batchItemsCount; ++i) {
            NarrowPhaseData &data = batch.Data[i];

            VASSERT(
                data.ShapeOne.GetType() == CollisionShapeType::ConvexPolyhedron && data.ShapeTwo.GetType() == CollisionShapeType::ConvexPolyhedron,
                "SATAlgorithm::PerformConvexPolyhedronVsConvexPolyhedronCollisionCheck only supports Convex Polyhedron vs Convex Polyhedron collision checks.");
            VASSERT(data.ContactPointCount == 0, "Contact points should be cleared before performing collision checks.");

            const auto *polyhedronOne = static_cast<const ConvexPolyhedronShape *>(&data.ShapeOne);
            const auto *polyhedronTwo = static_cast<const ConvexPolyhedronShape *>(&data.ShapeTwo);

            const TransformComponent polyhedronOneToTwo = data.ShapeTwoToWorldTransform.Inverse() * data.ShapeOneToWorldTransform;
            const TransformComponent polyhedronTwoToOne = polyhedronOneToTwo.Inverse();

            f32 minPenetrationDepth = std::numeric_limits<f32>::max();
            size_t minFaceIndex = 0;
            bool isMinPenetrationFaceNormal = false;
            bool isMinPenetrationFaceNormalPolyhedronOne = false;
            size_t minSeparatingEdgeOneIndex = 0;
            size_t minSeparatingEdgeTwoIndex = 0;
            glm::vec3 separatingEdgeOneVertexOne;
            glm::vec3 separatingEdgeOneVertexTwo;
            glm::vec3 separatingEdgeTwoVertexOne;
            glm::vec3 separatingEdgeTwoVertexTwo;
            glm::vec3 minEdgeVsEdgeSeparatingAxisInPolyhedronTwoSpace;

            const bool isShapeOneTriangle = polyhedronOne->GetName() == CollisionShapeName::Triangle;

            LastFrameCollisionData &lastFrameCollisionData = data.LastFrameCollisionData;

            if (lastFrameCollisionData.IsValid && lastFrameCollisionData.WasUsingSATAlgorithm) {
                // We perform temporal coherence, we check if there is still an overlapping along the previous minimum separating
                // axis. If it is the case, we directly report the collision without executing the whole SAT algorithm again. If
                // the shapes are still separated along this axis, we directly exit with no collision.

                // If the previous separating axis (or axis with minimum penetration depth)
                // was a face normal of polyhedron 1
                if (lastFrameCollisionData.SATIsAxisFacePolyhedronOne) {

                    const f32 penetrationDepth = testSingleFaceDirectionPolyhedronVsPolyhedron(
                        *polyhedronOne, *polyhedronTwo, polyhedronOneToTwo, lastFrameCollisionData.SATMinAxisFaceIndex);

                    // If the previous axis was a separating axis and is still a separating axis in this frame
                    if (!lastFrameCollisionData.WasColliding && penetrationDepth <= 0.0f) {

                        // Return no collision without running the whole SAT algorithm
                        continue;
                    }

                    // The two shapes were overlapping in the previous frame and still seem to overlap in this one
                    if (lastFrameCollisionData.WasColliding && _clipWithPreviousAxisIfStillColliding && penetrationDepth > 0.0f) {

                        minPenetrationDepth = penetrationDepth;
                        minFaceIndex = lastFrameCollisionData.SATMinAxisFaceIndex;
                        isMinPenetrationFaceNormal = true;
                        isMinPenetrationFaceNormalPolyhedronOne = true;

                        // Compute the contact points between two faces of two convex polyhedra.
                        if (computePolyhedronVsPolyhedronFaceContactPoints(isMinPenetrationFaceNormalPolyhedronOne,
                                                                           polyhedronOne,
                                                                           polyhedronTwo,
                                                                           polyhedronOneToTwo,
                                                                           polyhedronTwoToOne,
                                                                           minFaceIndex,
                                                                           batch,
                                                                           i)) {

                            lastFrameCollisionData.SATIsAxisFacePolyhedronOne = isMinPenetrationFaceNormalPolyhedronOne;
                            lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = !isMinPenetrationFaceNormalPolyhedronOne;
                            lastFrameCollisionData.SATMinAxisFaceIndex = static_cast<u8>(minFaceIndex);

                            // The shapes are still overlapping in the previous axis (the contact manifold is not empty).
                            // Therefore, we can return without running the whole SAT algorithm
                            data.IsColliding = true;
                            collisionDetected = true;
                            continue;
                        }

                        // The contact manifold is empty. Therefore, we have to run the whole SAT algorithm again
                    }
                } else if (lastFrameCollisionData.SATIsAxisFacePolyhedronTwo) { // If the previous separating axis (or axis with minimum penetration depth)
                                                                                // was a face normal of polyhedron 2

                    f32 penetrationDepth = testSingleFaceDirectionPolyhedronVsPolyhedron(
                        *polyhedronTwo, *polyhedronOne, polyhedronTwoToOne, lastFrameCollisionData.SATMinAxisFaceIndex);

                    // If the previous axis was a separating axis and is still a separating axis in this frame
                    if (!lastFrameCollisionData.WasColliding && penetrationDepth <= 0.0f) {

                        // Return no collision without running the whole SAT algorithm
                        continue;
                    }

                    // The two shapes were overlapping in the previous frame and still seem to overlap in this one
                    if (lastFrameCollisionData.WasColliding && _clipWithPreviousAxisIfStillColliding && penetrationDepth > 0.0f) {

                        minPenetrationDepth = penetrationDepth;
                        minFaceIndex = lastFrameCollisionData.SATMinAxisFaceIndex;
                        isMinPenetrationFaceNormal = true;
                        isMinPenetrationFaceNormalPolyhedronOne = false;

                        // Compute the contact points between two faces of two convex polyhedra.
                        if (computePolyhedronVsPolyhedronFaceContactPoints(isMinPenetrationFaceNormalPolyhedronOne,
                                                                           polyhedronOne,
                                                                           polyhedronTwo,
                                                                           polyhedronOneToTwo,
                                                                           polyhedronTwoToOne,
                                                                           minFaceIndex,
                                                                           batch,
                                                                           i)) {

                            lastFrameCollisionData.SATIsAxisFacePolyhedronOne = isMinPenetrationFaceNormalPolyhedronOne;
                            lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = !isMinPenetrationFaceNormalPolyhedronOne;
                            lastFrameCollisionData.SATMinAxisFaceIndex = static_cast<u8>(minFaceIndex);

                            // The shapes are still overlapping in the previous axis (the contact manifold is not empty).
                            // Therefore, we can return without running the whole SAT algorithm
                            data.IsColliding = true;
                            collisionDetected = true;

                            continue;
                        }

                        // The contact manifold is empty. Therefore, we have to run the whole SAT algorithm again
                    }
                } else { // If the previous separating axis (or axis with minimum penetration depth) was the cross product of two edges

                    const HalfEdgeMesh::Edge &edge1 = polyhedronOne->GetHalfEdge(lastFrameCollisionData.SATMinEdgeOneIndex);
                    const HalfEdgeMesh::Edge &edge2 = polyhedronTwo->GetHalfEdge(lastFrameCollisionData.SATMinEdgeTwoIndex);

                    const glm::vec3 edge1A = polyhedronOneToTwo * polyhedronOne->GetVertexPosition(edge1.StartVertexIndex);
                    const glm::vec3 edge1B =
                        polyhedronOneToTwo * polyhedronOne->GetVertexPosition(polyhedronOne->GetHalfEdge(edge1.NextEdgeIndex).StartVertexIndex);
                    const glm::vec3 edge1Direction = edge1B - edge1A;
                    const glm::vec3 edge2A = polyhedronTwo->GetVertexPosition(edge2.StartVertexIndex);
                    const glm::vec3 edge2B = polyhedronTwo->GetVertexPosition(polyhedronTwo->GetHalfEdge(edge2.NextEdgeIndex).StartVertexIndex);
                    const glm::vec3 edge2Direction = edge2B - edge2A;

                    // If the two edges build a minkowski face (and the cross product is
                    // therefore a candidate for separating axis
                    if (testEdgesBuildMinkowskiFace(*polyhedronOne, edge1, *polyhedronTwo, edge2, polyhedronOneToTwo)) {

                        glm::vec3 separatingAxisPolyhedron2Space;

                        // Compute the penetration depth along the previous axis
                        const glm::vec3 polyhedron1Centroid = polyhedronOneToTwo * polyhedronOne->GetCentroid();
                        f32 penetrationDepth = computeDistanceBetweenEdges(edge1A,
                                                                           edge2A,
                                                                           polyhedron1Centroid,
                                                                           polyhedronTwo->GetCentroid(),
                                                                           edge1Direction,
                                                                           edge2Direction,
                                                                           isShapeOneTriangle,
                                                                           separatingAxisPolyhedron2Space);

                        // If the shapes were not overlapping in the previous frame and are still not
                        // overlapping in the current one
                        if (!lastFrameCollisionData.WasColliding && penetrationDepth <= 0.0f) {

                            // We have found a separating axis without running the whole SAT algorithm
                            continue;
                        }

                        // If the shapes were overlapping on the previous axis and still seem to overlap in this frame
                        if (lastFrameCollisionData.WasColliding && _clipWithPreviousAxisIfStillColliding && penetrationDepth > 0.0f &&
                            penetrationDepth < std::numeric_limits<f32>::max()) {

                            // Compute the closest points between the two edges (in the local-space of polyhedron 2)
                            glm::vec3 closestPointPolyhedron1Edge, closestPointPolyhedron2Edge;
                            ComputeClosestPointBetweenTwoSegments(edge1A, edge1B, edge2A, edge2B, closestPointPolyhedron1Edge, closestPointPolyhedron2Edge);

                            // Here we try to project the closest point on edge1 onto the segment of edge 2 to see if
                            // the projected point falls onto the segment. We also try to project the closest point
                            // on edge 2 to see if it falls onto the segment of edge 1. If one of the point does not
                            // fall onto the opposite segment, it means the edges are not colliding (the contact manifold
                            // is empty). Therefore, we need to run the whole SAT algorithm again.
                            const glm::vec3 vec1 = closestPointPolyhedron1Edge - edge2A;
                            const glm::vec3 vec2 = closestPointPolyhedron2Edge - edge1A;
                            const f32 edge1LengthSquare = glm::length2(edge1Direction);
                            const f32 edge2LengthSquare = glm::length2(edge2Direction);
                            f32 t1 = glm::dot(vec1, edge2Direction) / edge2LengthSquare;
                            f32 t2 = glm::dot(vec2, edge1Direction) / edge1LengthSquare;

                            if (t1 >= 0.0f && t1 <= 1.0f && t2 >= 0.0f && t2 <= 1.0f) {

                                // If we need to report contact points
                                if (data.ReportContacts) {

                                    // Compute the contact point on polyhedron 1 edge in the local-space of polyhedron 1
                                    glm::vec3 closestPointPolyhedron1EdgeLocalSpace = polyhedronTwoToOne * closestPointPolyhedron1Edge;

                                    // Compute the world normal
                                    glm::vec3 normalWorld = data.ShapeTwoToWorldTransform.Rotation * separatingAxisPolyhedron2Space;

                                    // Compute smooth triangle mesh contact if one of the two collision shapes is a triangle
                                    TriangleShape::ComputeSmoothTriangleMeshContact(&data.ShapeOne,
                                                                                    &data.ShapeTwo,
                                                                                    closestPointPolyhedron1EdgeLocalSpace,
                                                                                    closestPointPolyhedron2Edge,
                                                                                    data.ShapeOneToWorldTransform,
                                                                                    data.ShapeTwoToWorldTransform,
                                                                                    penetrationDepth,
                                                                                    normalWorld);

                                    // Create the contact point
                                    batch.AddContactPoint(i, normalWorld, penetrationDepth, closestPointPolyhedron1EdgeLocalSpace, closestPointPolyhedron2Edge);
                                }

                                // The shapes are overlapping on the previous axis (the contact manifold is not empty). Therefore
                                // we return without running the whole SAT algorithm
                                data.IsColliding = true;
                                collisionDetected = true;

                                continue;
                            }

                            // The contact manifold is empty. Therefore, we have to run the whole SAT algorithm again
                        }
                    }
                }
            }

            minPenetrationDepth = std::numeric_limits<f32>::max();
            isMinPenetrationFaceNormal = false;

            // Test all the face normals of the polyhedron 1 for separating axis
            size_t faceIndex1;
            f32 penetrationDepth1 = testFacesDirectionPolyhedronVsPolyhedron(*polyhedronOne, *polyhedronTwo, polyhedronOneToTwo, faceIndex1);

            if (penetrationDepth1 <= 0.0f) {
                lastFrameCollisionData.SATIsAxisFacePolyhedronOne = true;
                lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = false;
                lastFrameCollisionData.SATMinAxisFaceIndex = static_cast<u8>(faceIndex1);

                // We have found a separating axis
                continue;
            }

            // Test all the face normals of the polyhedron 2 for separating axis
            size_t faceIndex2;
            f32 penetrationDepth2 = testFacesDirectionPolyhedronVsPolyhedron(*polyhedronTwo, *polyhedronOne, polyhedronTwoToOne, faceIndex2);

            if (penetrationDepth2 <= 0.0f) {
                lastFrameCollisionData.SATIsAxisFacePolyhedronOne = false;
                lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = true;
                lastFrameCollisionData.SATMinAxisFaceIndex = static_cast<u8>(faceIndex2);

                // We have found a separating axis
                continue;
            }

            // Here we know that we have found penetration along both axis of a face of polyhedronOne and a face of
            // polyhedronTwo. If the two penetration depths are almost the same, we need to make sure we always prefer
            // one axis to the other for consistency between frames. This is to prevent the contact manifolds to switch
            // from one reference axis to the other for a face to face resting contact for instance. This is better for
            // stability. To do this, we use a relative and absolute bias to move penetrationDepth2 a little bit to the right.
            // Now if:
            //  penetrationDepth1 < penetrationDepth2: Nothing happens and we use axis of polygon 1
            //  penetrationDepth1 ~ penetrationDepth2: Until penetrationDepth2 becomes significantly less than penetrationDepth1 we still use axis of polygon 1
            //  penetrationDepth1 >> penetrationDepth2: penetrationDepth2 is now significantly less than penetrationDepth1 and we use polygon 2 axis
            if (penetrationDepth1 < penetrationDepth2 * SEPARATING_AXIS_RELATIVE_TOLERANCE + SEPARATING_AXIS_ABSOLUTE_TOLERANCE) {

                // We use penetration axis of polygon 1
                isMinPenetrationFaceNormal = true;
                minPenetrationDepth = std::min(penetrationDepth1, penetrationDepth2);
                minFaceIndex = faceIndex1;
                isMinPenetrationFaceNormalPolyhedronOne = true;
            } else {

                // We use penetration axis of polygon 2
                isMinPenetrationFaceNormal = true;
                minPenetrationDepth = std::min(penetrationDepth1, penetrationDepth2);
                minFaceIndex = faceIndex2;
                isMinPenetrationFaceNormalPolyhedronOne = false;
            }

            bool separatingAxisFound = false;

            // Test the cross products of edges of polyhedron 1 with edges of polyhedron 2 for separating axis
            for (size_t i = 0; i < polyhedronOne->GetHalfEdgesCount(); i += 2) {

                // Get an edge of polyhedron 1
                const HalfEdgeMesh::Edge &edge1 = polyhedronOne->GetHalfEdge(i);

                const glm::vec3 edge1A = polyhedronOneToTwo * polyhedronOne->GetVertexPosition(edge1.StartVertexIndex);
                const glm::vec3 edge1B =
                    polyhedronOneToTwo * polyhedronOne->GetVertexPosition(polyhedronOne->GetHalfEdge(edge1.NextEdgeIndex).StartVertexIndex);
                const glm::vec3 edge1Direction = edge1B - edge1A;

                for (size_t j = 0; j < polyhedronTwo->GetHalfEdgesCount(); j += 2) {

                    // Get an edge of polyhedron 2
                    const HalfEdgeMesh::Edge &edge2 = polyhedronTwo->GetHalfEdge(j);

                    const glm::vec3 edge2A = polyhedronTwo->GetVertexPosition(edge2.StartVertexIndex);
                    const glm::vec3 edge2B = polyhedronTwo->GetVertexPosition(polyhedronTwo->GetHalfEdge(edge2.NextEdgeIndex).StartVertexIndex);
                    const glm::vec3 edge2Direction = edge2B - edge2A;

                    // If the two edges build a minkowski face (and the cross product is therefore a candidate for separating axis
                    if (testEdgesBuildMinkowskiFace(*polyhedronOne, edge1, *polyhedronTwo, edge2, polyhedronOneToTwo)) {

                        glm::vec3 separatingAxisPolyhedron2Space;

                        // Compute the penetration depth
                        const glm::vec3 polyhedron1Centroid = polyhedronOneToTwo * polyhedronOne->GetCentroid();
                        f32 penetrationDepth = computeDistanceBetweenEdges(edge1A,
                                                                           edge2A,
                                                                           polyhedron1Centroid,
                                                                           polyhedronTwo->GetCentroid(),
                                                                           edge1Direction,
                                                                           edge2Direction,
                                                                           isShapeOneTriangle,
                                                                           separatingAxisPolyhedron2Space);

                        if (penetrationDepth <= 0.0f) {
                            lastFrameCollisionData.SATIsAxisFacePolyhedronOne = false;
                            lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = false;
                            lastFrameCollisionData.SATMinEdgeOneIndex = static_cast<u8>(i);
                            lastFrameCollisionData.SATMinEdgeTwoIndex = static_cast<u8>(j);

                            // We have found a separating axis
                            separatingAxisFound = true;

                            break;
                        }

                        // If the current minimum penetration depth is along a face normal axis (isMinPenetrationFaceNormal=true) and we have found a new
                        // smaller penetration depth along an edge-edge cross-product axis we want to favor the face normal axis because contact manifolds
                        // between faces have more contact points and therefore more stable than the single contact point of an edge-edge collision. It means
                        // that if the new minimum penetration depth from the edge-edge contact is only a little bit smaller than the current
                        // minPenetrationDepth (from a face contact), we favor the face contact and do not generate an edge-edge contact. However, if the new
                        // penetration depth from the edge-edge contact is really smaller than the current one, we generate an edge-edge contact. To do this, we
                        // use a relative and absolute bias to increase a little bit the new penetration depth from the edge-edge contact during the comparison
                        // test
                        if ((isMinPenetrationFaceNormal &&
                             penetrationDepth * SEPARATING_AXIS_RELATIVE_TOLERANCE + SEPARATING_AXIS_ABSOLUTE_TOLERANCE < minPenetrationDepth) ||
                            (!isMinPenetrationFaceNormal && penetrationDepth < minPenetrationDepth)) {

                            minPenetrationDepth = penetrationDepth;
                            isMinPenetrationFaceNormalPolyhedronOne = false;
                            isMinPenetrationFaceNormal = false;
                            minSeparatingEdgeOneIndex = i;
                            minSeparatingEdgeTwoIndex = j;
                            separatingEdgeOneVertexOne = edge1A;
                            separatingEdgeOneVertexTwo = edge1B;
                            separatingEdgeTwoVertexOne = edge2A;
                            separatingEdgeTwoVertexTwo = edge2B;
                            minEdgeVsEdgeSeparatingAxisInPolyhedronTwoSpace = separatingAxisPolyhedron2Space;
                        }
                    }
                }

                if (separatingAxisFound) {
                    break;
                }
            }

            if (separatingAxisFound) {
                continue;
            }

            // Here we know the shapes are overlapping on a given minimum separating axis.
            // Now, we will clip the shapes along this axis to find the contact points

            VASSERT(minPenetrationDepth > 0.0f, "minPenetrationDepth must be greater than 0.0f");

            // If the minimum separating axis is a face normal
            if (isMinPenetrationFaceNormal) {

                // Compute the contact points between two faces of two convex polyhedra.
                bool contactsFound = computePolyhedronVsPolyhedronFaceContactPoints(
                    isMinPenetrationFaceNormalPolyhedronOne, polyhedronOne, polyhedronTwo, polyhedronOneToTwo, polyhedronTwoToOne, minFaceIndex, batch, i);

                // There should be clipping points here. If it is not the case, it might be
                // because of a numerical issue
                if (!contactsFound) {

                    lastFrameCollisionData.SATIsAxisFacePolyhedronOne = isMinPenetrationFaceNormalPolyhedronOne;
                    lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = !isMinPenetrationFaceNormalPolyhedronOne;
                    lastFrameCollisionData.SATMinAxisFaceIndex = static_cast<u8>(minFaceIndex);

                    // Return no collision
                    continue;
                }

                lastFrameCollisionData.SATIsAxisFacePolyhedronOne = isMinPenetrationFaceNormalPolyhedronOne;
                lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = !isMinPenetrationFaceNormalPolyhedronOne;
                lastFrameCollisionData.SATMinAxisFaceIndex = static_cast<u8>(minFaceIndex);
            } else { // If we have an edge vs edge contact

                // If we need to report contacts
                if (data.ReportContacts) {
                    // Compute the closest points between the two edges (in the local-space of polyhedron 2)
                    glm::vec3 closestPointPolyhedron1Edge, closestPointPolyhedron2Edge;

                    ComputeClosestPointBetweenTwoSegments(separatingEdgeOneVertexOne,
                                                          separatingEdgeOneVertexTwo,
                                                          separatingEdgeTwoVertexOne,
                                                          separatingEdgeTwoVertexTwo,
                                                          closestPointPolyhedron1Edge,
                                                          closestPointPolyhedron2Edge);

                    // Compute the contact point on polyhedron 1 edge in the local-space of polyhedron 1
                    glm::vec3 closestPointPolyhedron1EdgeLocalSpace = polyhedronTwoToOne * closestPointPolyhedron1Edge;

                    // Compute the world normal
                    glm::vec3 normalWorld = data.ShapeTwoToWorldTransform.Rotation * minEdgeVsEdgeSeparatingAxisInPolyhedronTwoSpace;

                    // Compute smooth triangle mesh contact if one of the two collision shapes is a triangle
                    TriangleShape::ComputeSmoothTriangleMeshContact(&data.ShapeOne,
                                                                    &data.ShapeTwo,
                                                                    closestPointPolyhedron1EdgeLocalSpace,
                                                                    closestPointPolyhedron2Edge,
                                                                    data.ShapeOneToWorldTransform,
                                                                    data.ShapeTwoToWorldTransform,
                                                                    minPenetrationDepth,
                                                                    normalWorld);

                    // Create the contact point
                    batch.AddContactPoint(i, normalWorld, minPenetrationDepth, closestPointPolyhedron1EdgeLocalSpace, closestPointPolyhedron2Edge);
                }

                lastFrameCollisionData.SATIsAxisFacePolyhedronOne = false;
                lastFrameCollisionData.SATIsAxisFacePolyhedronTwo = false;
                lastFrameCollisionData.SATMinEdgeOneIndex = static_cast<u8>(minSeparatingEdgeOneIndex);
                lastFrameCollisionData.SATMinEdgeTwoIndex = static_cast<u8>(minSeparatingEdgeTwoIndex);
            }

            data.IsColliding = true;
            collisionDetected = true;
        }

        return collisionDetected;
    }

    bool SATAlgorithm::ComputeCapsulePolyhedronFaceContactPoints(size_t referenceFaceIndex,
                                                                 f32 capsuleRadius,
                                                                 const ConvexPolyhedronShape &polyhedron,
                                                                 f32 penetrationDepth,
                                                                 const TransformComponent &polyhedronToCapsuleTransform,
                                                                 glm::vec3 &normalWorld,
                                                                 const glm::vec3 &separatingAxisCapsuleSpace,
                                                                 const glm::vec3 &capsuleSegAPolyhedronSpace,
                                                                 const glm::vec3 &capsuleSegBPolyhedronSpace,
                                                                 NarrowPhaseDataBatch &batch,
                                                                 size_t batchIndex,
                                                                 bool isShapeOneCapsule) const {
        NarrowPhaseData &data = batch.Data[batchIndex];
        const HalfEdgeMesh::Face &face = polyhedron.GetFace(referenceFaceIndex);

        // Get the face normal.
        glm::vec3 faceNormal = polyhedron.GetFaceNormal(referenceFaceIndex);

        size_t firstEdgeIndex = face.EdgeIndex;
        size_t edgeIndex = firstEdgeIndex;

        std::vector<glm::vec3> planesPoints;
        std::vector<glm::vec3> planesNormals;
        planesPoints.reserve(2);
        planesNormals.reserve(2);

        // Build one clipping plane per edge of the reference face. Each plane passes through the edge's start
        // vertex and has a normal perpendicular to both the edge direction and the reference face normal, so it
        // cuts off the region outside that edge's lateral boundary.
        do {

            const HalfEdgeMesh::Edge &edge = polyhedron.GetHalfEdge(edgeIndex);
            const HalfEdgeMesh::Edge &twinEdge = polyhedron.GetHalfEdge(edge.TwinEdgeIndex);

            // Compute the edge vertices and edge direction.
            glm::vec3 edgeV1 = polyhedron.GetVertexPosition(edge.StartVertexIndex);
            glm::vec3 edgeV2 = polyhedron.GetVertexPosition(twinEdge.StartVertexIndex);
            glm::vec3 edgeDirection = edgeV2 - edgeV1;

            // The clipping plane normal is perpendicular to the edge direction and the reference face normal,
            // so it points inward (toward the interior of the reference face).
            glm::vec3 clipPlaneNormal = glm::cross(faceNormal, edgeDirection);

            planesPoints.push_back(polyhedron.GetVertexPosition(edge.StartVertexIndex));
            planesNormals.push_back(clipPlaneNormal);

            edgeIndex = edge.NextEdgeIndex;

        } while (edgeIndex != firstEdgeIndex);

        // Clip the capsule inner segment against the lateral boundary planes of the reference face to obtain
        // the portion of the segment that lies within the face's footprint.
        std::vector<glm::vec3> clipSegment = ClipSegmentWithPlanes(capsuleSegAPolyhedronSpace, capsuleSegBPolyhedronSpace, planesPoints, planesNormals);

        // Offset to move a clipped point from its position on the capsule axis to its projection on the face.
        const glm::vec3 delta = faceNormal * (penetrationDepth - capsuleRadius);

        bool contactFound = false;

        // Keep only the clipped points whose penetration depth is consistent with the minimum depth for this
        // separating axis. Points that are far from the face (e.g. endpoints outside the clipping region that
        // were not fully trimmed) are discarded.
        for (size_t i = 0; i < clipSegment.size(); i++) {

            // Measure how far the clipped point is below the reference face plane.
            const f32 clipPointPenDepth = glm::dot(planesPoints[0] - clipSegment[i], faceNormal);

            // Accept the point only if its depth matches the axis penetration depth (within a small tolerance).
            if (clipPointPenDepth > penetrationDepth - capsuleRadius - f32(0.001)) {

                if (!contactFound) {
                    batch.ResetContactPoints(batchIndex);
                }

                contactFound = true;

                glm::vec3 contactPointPolyhedron = clipSegment[i] + delta;

                // Project the clipped point into the capsule bounds
                glm::vec3 contactPointCapsule = (polyhedronToCapsuleTransform * clipSegment[i]) - separatingAxisCapsuleSpace * capsuleRadius;

                // Compute smooth triangle mesh contact if one of the two collision shapes is a triangle
                TriangleShape::ComputeSmoothTriangleMeshContact(&data.ShapeOne,
                                                                &data.ShapeTwo,
                                                                isShapeOneCapsule ? contactPointCapsule : contactPointPolyhedron,
                                                                isShapeOneCapsule ? contactPointPolyhedron : contactPointCapsule,
                                                                data.ShapeOneToWorldTransform,
                                                                data.ShapeTwoToWorldTransform,
                                                                penetrationDepth,
                                                                normalWorld);

                // Create the contact point
                batch.AddContactPoint(batchIndex,
                                      normalWorld,
                                      penetrationDepth,
                                      isShapeOneCapsule ? contactPointCapsule : contactPointPolyhedron,
                                      isShapeOneCapsule ? contactPointPolyhedron : contactPointCapsule);
            }
        }

        return contactFound;
    }

    bool SATAlgorithm::testEdgesBuildMinkowskiFace(const ConvexPolyhedronShape &polyhedronOne,
                                                   const HalfEdgeMesh::Edge &edgeOne,
                                                   const ConvexPolyhedronShape &polyhedronTwo,
                                                   const HalfEdgeMesh::Edge &edgeTwo,
                                                   const TransformComponent &polyhedronOneToTwo) const {
        // Fetch the outward face normals on either side of each edge and rotate them into a shared space
        // (polyhedronTwo local space) so the Gauss Map test can compare them directly.
        const glm::vec3 a = polyhedronOneToTwo.Rotation * polyhedronOne.GetFaceNormal(edgeOne.FaceIndex);
        const glm::vec3 b = polyhedronOneToTwo.Rotation * polyhedronOne.GetFaceNormal(polyhedronOne.GetHalfEdge(edgeOne.TwinEdgeIndex).FaceIndex);

        const glm::vec3 c = polyhedronTwo.GetFaceNormal(edgeTwo.FaceIndex);
        const glm::vec3 d = polyhedronTwo.GetFaceNormal(polyhedronTwo.GetHalfEdge(edgeTwo.TwinEdgeIndex).FaceIndex);

        // Approximate b × a using the edge direction: for a half-edge, (vertex1 - vertex2) is proportional
        // to the cross product of the two adjacent face normals, avoiding a full cross product computation.
        const glm::vec3 edgeOneVertexOne = polyhedronOne.GetVertexPosition(edgeOne.StartVertexIndex);
        const glm::vec3 edgeOneVertexTwo = polyhedronOne.GetVertexPosition(polyhedronOne.GetHalfEdge(edgeOne.TwinEdgeIndex).StartVertexIndex);
        const glm::vec3 bCrossA = polyhedronOneToTwo.Rotation * (edgeOneVertexOne - edgeOneVertexTwo);

        // Same approximation for d × c using edgeTwo's direction.
        const glm::vec3 edgeTwoVertexOne = polyhedronTwo.GetVertexPosition(edgeTwo.StartVertexIndex);
        const glm::vec3 edgeTwoVertexTwo = polyhedronTwo.GetVertexPosition(polyhedronTwo.GetHalfEdge(edgeTwo.TwinEdgeIndex).StartVertexIndex);
        const glm::vec3 dCrossC = edgeTwoVertexOne - edgeTwoVertexTwo;

        // Negate the normals of the second polyhedron because we work with the Gauss map of the Minkowski
        // difference (A - B), where B's Gauss map is reflected.
        return testGaussMapArcsIntersect(a, b, -c, -d, bCrossA, dCrossC);
    }

    bool SATAlgorithm::testGaussMapArcsIntersect(
        const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d, const glm::vec3 &bCrossA, const glm::vec3 &dCrossC) const {

        // Project each arc endpoint onto the great circle plane of the other arc.
        // The four dot products tell us on which side of each great circle the opposite endpoints lie.
        const f32 cba = glm::dot(c, bCrossA);
        const f32 dba = glm::dot(d, bCrossA);
        const f32 adc = glm::dot(a, dCrossC);
        const f32 bdc = glm::dot(b, dCrossC);

        // The arcs intersect if and only if C and D straddle arc AB's great circle (cba * dba < 0),
        // A and B straddle arc CD's great circle (adc * bdc < 0), and the arcs lie on the same hemisphere
        // (cba * bdc > 0) rather than being antipodal.
        return cba * dba < f32(0.0) && adc * bdc < f32(0.0) && cba * bdc > f32(0.0);
    }

    f32 SATAlgorithm::computeDistanceBetweenEdges(const glm::vec3 &edgeOneA,
                                                  const glm::vec3 &edgeTwoA,
                                                  const glm::vec3 &polyhedronOneCentroid,
                                                  const glm::vec3 &polyhedronTwoCentroid,
                                                  const glm::vec3 &edgeOneDirection,
                                                  const glm::vec3 &edgeTwoDirection,
                                                  bool isShapeOneTriangle,
                                                  glm::vec3 &outSeparatingAxis) const {
        // If the two edges are parallel return a large penetration depth to skip those edges.
        if (AreParallelVectors(edgeOneDirection, edgeTwoDirection)) {
            return std::numeric_limits<f32>::max();
        }

        // Compute the candidate separating axis (cross product between two polyhedrons edges).
        glm::vec3 axis = glm::normalize(glm::cross(edgeOneDirection, edgeTwoDirection));

        // Make sure the axis direction is going from first to second polyhedron.
        f32 dotProduct;

        if (isShapeOneTriangle) {

            // The shape 1 is a triangle. It is safer to use a vector from
            // centroid to edge of the shape 2 because for a triangle, we
            // can have a vector that is orthogonal to the axis

            dotProduct = glm::dot(axis, edgeTwoA - polyhedronTwoCentroid);
        } else {

            // The shape 2 might be a triangle. It is safer to use a vector from
            // centroid to edge of the shape 2 because for a triangle, we
            // can have a vector that is orthogonal to the axis

            dotProduct = glm::dot(axis, polyhedronOneCentroid - edgeOneA);
        }

        if (dotProduct > f32(0.0)) {
            axis = -axis;
        }

        outSeparatingAxis = axis;

        // Compute and return the distance between the edges.
        return glm::dot(-axis, edgeTwoA - edgeOneA);
    }

    f32 SATAlgorithm::testSingleFaceDirectionPolyhedronVsPolyhedron(const ConvexPolyhedronShape &polyhedronOne,
                                                                    const ConvexPolyhedronShape &polyhedronTwo,
                                                                    const TransformComponent &polyhedronOneToTwoTransform,
                                                                    size_t faceIndex) const {
        // Get the face.
        const HalfEdgeMesh::Face &face = polyhedronOne.GetFace(faceIndex);

        // Get the face normal.
        const glm::vec3 faceNormal = polyhedronOne.GetFaceNormal(faceIndex);

        // Convert the face normal into the local-space of polyhedron 2.
        const glm::vec3 faceNormalInPolyhedronTwoSpace = polyhedronOneToTwoTransform.Rotation * faceNormal;

        // Get the support point of polyhedron 2 in the inverse direction of face normal.
        const glm::vec3 supportPoint = polyhedronTwo.GetLocalSupportPointWithoutMargin(-faceNormalInPolyhedronTwoSpace);

        // Compute the penetration depth.
        const glm::vec3 faceVertex = polyhedronOneToTwoTransform * polyhedronOne.GetVertexPosition(face.FaceVertices[0]);
        f32 penetrationDepth = glm::dot(faceVertex - supportPoint, faceNormalInPolyhedronTwoSpace);

        return penetrationDepth;
    }

    f32 SATAlgorithm::testFacesDirectionPolyhedronVsPolyhedron(const ConvexPolyhedronShape &polyhedronOne,
                                                               const ConvexPolyhedronShape &polyhedronTwo,
                                                               const TransformComponent &polyhedronOneToTwoTransform,
                                                               size_t &minFaceIndex) const {
        f32 minPenetrationDepth = std::numeric_limits<f32>::max();

        // Test polyhedronOne face normals as separating axes.
        for (size_t f = 0; f < polyhedronOne.GetFacesCount(); f++) {
            f32 penetrationDepth = testSingleFaceDirectionPolyhedronVsPolyhedron(polyhedronOne, polyhedronTwo, polyhedronOneToTwoTransform, f);

            // If the penetration depth is negative, we have found a separating axis.
            if (penetrationDepth <= f32(0.0)) {
                minFaceIndex = f;
                return penetrationDepth;
            }

            // Check if we have found a new minimum penetration axis.
            if (penetrationDepth < minPenetrationDepth) {
                minPenetrationDepth = penetrationDepth;
                minFaceIndex = f;
            }
        }

        return minPenetrationDepth;
    }

    f32 SATAlgorithm::computePolyhedronFaceVsSpherePenetrationDepth(size_t faceIndex,
                                                                    const ConvexPolyhedronShape &polyhedron,
                                                                    const SphereShape &sphere,
                                                                    const glm::vec3 &sphereCenter) const {
        // Get the face.
        const HalfEdgeMesh::Face &face = polyhedron.GetFace(faceIndex);

        // Get the face normal.
        const glm::vec3 faceNormal = polyhedron.GetFaceNormal(faceIndex);

        // Compute the vector from the sphere center to any point on the face (e.g. the first vertex of the face).
        glm::vec3 sphereCenterToFacePoint = polyhedron.GetVertexPosition(face.FaceVertices[0]) - sphereCenter;

        // The penetration depth is the projection of the vector from the sphere center to the face onto the face normal, plus the sphere radius.
        f32 penetrationDepth = glm::dot(sphereCenterToFacePoint, faceNormal) + sphere.GetRadius();

        return penetrationDepth;
    }

    f32 SATAlgorithm::computePolyhedronFaceVsCapsulePenetrationDepth(size_t polyhedronFaceIndex,
                                                                     const ConvexPolyhedronShape &polyhedron,
                                                                     const CapsuleShape &capsule,
                                                                     const TransformComponent &polyhedronToCapsuleTransform,
                                                                     glm::vec3 &outFaceNormalCapsuleSpace) const {
        // Get the face.
        const HalfEdgeMesh::Face &face = polyhedron.GetFace(polyhedronFaceIndex);

        // Get the face normal.
        const glm::vec3 faceNormal = polyhedron.GetFaceNormal(polyhedronFaceIndex);

        // Compute the penetration depth (using the capsule support in the direction opposite to the face normal).
        outFaceNormalCapsuleSpace = polyhedronToCapsuleTransform.Rotation * faceNormal;
        const glm::vec3 capsuleSupportPoint = capsule.GetLocalSupportPointWithMargin(-outFaceNormalCapsuleSpace);
        const glm::vec3 pointOnPolyhedronFace = polyhedronToCapsuleTransform * polyhedron.GetVertexPosition(face.FaceVertices[0]);
        const glm::vec3 capsuleSupportPointToFacePoint = pointOnPolyhedronFace - capsuleSupportPoint;
        const f32 penetrationDepth = glm::dot(capsuleSupportPointToFacePoint, outFaceNormalCapsuleSpace);

        return penetrationDepth;
    }

    f32 SATAlgorithm::computeEdgeVsCapsuleInnerSegmentPenetrationDepth(const ConvexPolyhedronShape &polyhedron,
                                                                       const CapsuleShape &capsule,
                                                                       const glm::vec3 &capsuleSegmentAxis,
                                                                       const glm::vec3 &edgeVertexOne,
                                                                       const glm::vec3 &edgeDirectionCapsuleSpace,
                                                                       const TransformComponent &polyhedronToCapsuleTransform,
                                                                       glm::vec3 &outAxis) const {
        f32 penetrationDepth = std::numeric_limits<f32>::max();

        // Compute the axis to test (cross product between capsule inner segment and polyhedron edge).
        outAxis = glm::cross(capsuleSegmentAxis, edgeDirectionCapsuleSpace);

        // Skip separating axis test if polyhedron edge is parallel to the capsule inner segment.
        if (glm::length2(outAxis) >= f32(0.00001)) {
            const glm::vec3 polyhedronCentroid = polyhedronToCapsuleTransform * polyhedron.GetCentroid();
            const glm::vec3 pointOnPolyhedronEdge = polyhedronToCapsuleTransform * edgeVertexOne;

            // Swap axis direction if necessary such that it points out of the polyhedron.
            if (glm::dot(outAxis, pointOnPolyhedronEdge - polyhedronCentroid) < 0) {
                outAxis = -outAxis;
            }

            // Normalize the axis for distance computation.
            outAxis = glm::normalize(outAxis);

            // Compute the penetration depth.
            const glm::vec3 capsuleSupportPoint = capsule.GetLocalSupportPointWithMargin(-outAxis);
            const glm::vec3 capsuleSupportPointToEdgePoint = pointOnPolyhedronEdge - capsuleSupportPoint;
            penetrationDepth = glm::dot(capsuleSupportPointToEdgePoint, outAxis);
        }

        return penetrationDepth;
    }

    bool SATAlgorithm::computePolyhedronVsPolyhedronFaceContactPoints(bool isMinPenetrationFaceNormalPolyhedronOne,
                                                                      const ConvexPolyhedronShape *polyhedronOne,
                                                                      const ConvexPolyhedronShape *polyhedronTwo,
                                                                      const TransformComponent &polyhedronOneToTwo,
                                                                      const TransformComponent &polyhedronTwoToOne,
                                                                      size_t minFaceIndex,
                                                                      NarrowPhaseDataBatch &batch,
                                                                      size_t batchIndex) const {
        // The reference polyhedron owns the minimum-penetration face normal; the incident polyhedron is clipped against it.
        const ConvexPolyhedronShape *referencePolyhedron;
        const ConvexPolyhedronShape *incidentPolyhedron;

        const TransformComponent &referenceToIncidentTransform = isMinPenetrationFaceNormalPolyhedronOne ? polyhedronOneToTwo : polyhedronTwoToOne;
        const TransformComponent &incidentToReferenceTransform = isMinPenetrationFaceNormalPolyhedronOne ? polyhedronTwoToOne : polyhedronOneToTwo;

        if (isMinPenetrationFaceNormalPolyhedronOne) {
            referencePolyhedron = polyhedronOne;
            incidentPolyhedron = polyhedronTwo;
        } else {
            referencePolyhedron = polyhedronTwo;
            incidentPolyhedron = polyhedronOne;
        }

        const glm::vec3 axisReferenceSpace = referencePolyhedron->GetFaceNormal(minFaceIndex);
        const glm::vec3 axisIncidentSpace = referenceToIncidentTransform.Rotation * axisReferenceSpace;

        NarrowPhaseData &data = batch.Data[batchIndex];

        // The world-space contact normal points from the reference polyhedron toward the incident polyhedron
        // (i.e. from shape one toward shape two when shape one is the reference).
        const glm::vec3 contactNormal = isMinPenetrationFaceNormalPolyhedronOne ? data.ShapeOneToWorldTransform.Rotation * axisReferenceSpace
                                                                                : -(data.ShapeTwoToWorldTransform.Rotation * axisReferenceSpace);

        const HalfEdgeMesh::Face &referenceFace = referencePolyhedron->GetFace(minFaceIndex);

        // Find the incident face: the face on the incident polyhedron most anti-parallel to the reference face normal.
        size_t incidentFaceIndex = incidentPolyhedron->FindMostAntiParallelFaceIndex(axisIncidentSpace);
        const HalfEdgeMesh::Face &incidentFace = incidentPolyhedron->GetFace(incidentFaceIndex);

        const size_t incidentFaceVerticesCount = incidentFace.FaceVertices.size();
        const size_t maxElementsCount = incidentFaceVerticesCount * 2 * referenceFace.FaceVertices.size();

        // Two vertex buffers are ping-ponged during Sutherland-Hodgman clipping so that each clip pass reads
        // from one and writes into the other without allocating a new vector per pass.
        std::vector<glm::vec3> tempVerticesOne;
        std::vector<glm::vec3> tempVerticesTwo;
        tempVerticesOne.reserve(maxElementsCount);
        tempVerticesTwo.reserve(maxElementsCount);

        // Seed the clipping input with the incident face vertices transformed into reference local space.
        for (size_t i = 0; i < incidentFaceVerticesCount; i++) {
            const glm::vec3 faceVertexIncidentSpace = incidentPolyhedron->GetVertexPosition(incidentFace.FaceVertices[i]);
            tempVerticesOne.push_back(incidentToReferenceTransform * faceVertexIncidentSpace);
        }

        // Clip the incident face polygon against each lateral side plane of the reference face in turn
        // (Sutherland-Hodgman algorithm). Each side plane passes through one reference face edge and has
        // a normal perpendicular to the edge direction and the reference face normal.
        const size_t firstEdgeIndex = referenceFace.EdgeIndex;
        bool areVerticesOneInput = false;
        size_t outputVerticesCount;
        size_t currentEdgeIndex;

        const HalfEdgeMesh::Edge *currentEdge = &(referencePolyhedron->GetHalfEdge(firstEdgeIndex));
        glm::vec3 edgeStartVertex = referencePolyhedron->GetVertexPosition(currentEdge->StartVertexIndex);

        do {
            areVerticesOneInput = !areVerticesOneInput;

            const HalfEdgeMesh::Edge *nextEdge = &(referencePolyhedron->GetHalfEdge(currentEdge->NextEdgeIndex));

            const glm::vec3 edgeEndVertex = referencePolyhedron->GetVertexPosition(nextEdge->StartVertexIndex);
            const glm::vec3 edgeDirection = edgeEndVertex - edgeStartVertex;

            const glm::vec3 planeNormal = glm::cross(axisReferenceSpace, edgeDirection);

            VASSERT((areVerticesOneInput && tempVerticesOne.size() > 0) || !areVerticesOneInput, "There should be input vertices to clip.");
            VASSERT((!areVerticesOneInput && tempVerticesTwo.size() > 0) || areVerticesOneInput, "There should be input vertices to clip.");

            ClipPolygonWithPlane(
                areVerticesOneInput ? tempVerticesOne : tempVerticesTwo, edgeStartVertex, planeNormal, areVerticesOneInput ? tempVerticesTwo : tempVerticesOne);

            currentEdgeIndex = currentEdge->NextEdgeIndex;
            currentEdge = nextEdge;
            edgeStartVertex = edgeEndVertex;

            if (areVerticesOneInput) {
                tempVerticesOne.clear();
                outputVerticesCount = tempVerticesTwo.size();
            } else {
                tempVerticesTwo.clear();
                outputVerticesCount = tempVerticesOne.size();
            }
        } while (currentEdgeIndex != firstEdgeIndex && outputVerticesCount > 0);

        // After all clip passes, the surviving vertices are in whichever buffer was last written.
        std::vector<glm::vec3> &clippedPolygonVertices = areVerticesOneInput ? tempVerticesTwo : tempVerticesOne;
        const glm::vec3 referenceFaceVertex = referencePolyhedron->GetVertexPosition(referencePolyhedron->GetHalfEdge(firstEdgeIndex).StartVertexIndex);

        bool contactPointsFound = false;

        // Keep only clipped vertices that lie on or below the reference face plane (penetrationDepth > 0).
        // Vertices above the plane are outside the reference shape and do not correspond to real contacts.
        for (size_t i = 0; i < clippedPolygonVertices.size(); ++i) {
            const f32 penetrationDepth = glm::dot(referenceFaceVertex - clippedPolygonVertices[i], axisReferenceSpace);

            if (penetrationDepth > 0.0f) {
                contactPointsFound = true;

                if (data.ReportContacts) {
                    glm::vec3 outContactNormal = contactNormal;

                    glm::vec3 contactPointIncidentPolyhedron = referenceToIncidentTransform * clippedPolygonVertices[i];

                    glm::vec3 contactPointReferencePolyhedron = ProjectPointOntoPlane(clippedPolygonVertices[i], axisReferenceSpace, referenceFaceVertex);

                    TriangleShape::ComputeSmoothTriangleMeshContact(
                        &data.ShapeOne,
                        &data.ShapeTwo,
                        isMinPenetrationFaceNormalPolyhedronOne ? contactPointReferencePolyhedron : contactPointIncidentPolyhedron,
                        isMinPenetrationFaceNormalPolyhedronOne ? contactPointIncidentPolyhedron : contactPointReferencePolyhedron,
                        data.ShapeOneToWorldTransform,
                        data.ShapeTwoToWorldTransform,
                        penetrationDepth,
                        outContactNormal);

                    batch.AddContactPoint(batchIndex,
                                          outContactNormal,
                                          penetrationDepth,
                                          isMinPenetrationFaceNormalPolyhedronOne ? contactPointReferencePolyhedron : contactPointIncidentPolyhedron,
                                          isMinPenetrationFaceNormalPolyhedronOne ? contactPointIncidentPolyhedron : contactPointReferencePolyhedron);
                }
            }
        }

        return contactPointsFound;
    }

} // namespace Vulkyrie
