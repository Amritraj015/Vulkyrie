#include "physics/collision/narrowphase/sat/sat_algorithm.h"
#include "core/utilities.h"
#include <limits>

namespace Vulkyrie {

    SATAlgorithm::SATAlgorithm(bool clipWithPreviousAxisIfStillColliding)
        : _clipWithPreviousAxisIfStillColliding(clipWithPreviousAxisIfStillColliding) {
    }

    bool SATAlgorithm::PerformSphereVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        (void)batch;
        (void)batchStartIndex;
        (void)batchItemsCount;

        return collisionDetected;
    }

    bool SATAlgorithm::PerformCapsuleVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchIndex) {
        bool collisionDetected = false;

        (void)batch;
        (void)batchIndex;

        return collisionDetected;
    }

    bool SATAlgorithm::PerformConvexPolyhedronVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount) {
        bool collisionDetected = false;

        (void)batch;
        (void)batchStartIndex;
        (void)batchItemsCount;

        return collisionDetected;
    }

    bool SATAlgorithm::testEdgesBuildMinkowskiFace(const ConvexPolyhedronShape &polyhedronOne,
                                                   const HalfEdgeMesh::Edge &edgeOne,
                                                   const ConvexPolyhedronShape &polyhedronTwo,
                                                   const HalfEdgeMesh::Edge &edgeTwo,
                                                   const TransformComponent &polyhedronOneToTwo) const {
        const glm::vec3 a = polyhedronOneToTwo.Rotation * polyhedronOne.GetFaceNormal(edgeOne.FaceIndex);
        const glm::vec3 b = polyhedronOneToTwo.Rotation * polyhedronOne.GetFaceNormal(polyhedronOne.GetHalfEdge(edgeOne.TwinEdgeIndex).FaceIndex);

        const glm::vec3 c = polyhedronTwo.GetFaceNormal(edgeTwo.FaceIndex);
        const glm::vec3 d = polyhedronTwo.GetFaceNormal(polyhedronTwo.GetHalfEdge(edgeTwo.TwinEdgeIndex).FaceIndex);

        // Compute b.cross(a) using the edge direction.
        const glm::vec3 edgeOneVertexOne = polyhedronOne.GetVertexPosition(edgeOne.StartVertexIndex);
        const glm::vec3 edgeOneVertexTwo = polyhedronOne.GetVertexPosition(polyhedronOne.GetHalfEdge(edgeOne.TwinEdgeIndex).StartVertexIndex);
        const glm::vec3 bCrossA = polyhedronOneToTwo.Rotation * (edgeOneVertexOne - edgeOneVertexTwo);

        // Compute d.cross(c) using the edge direction.
        const glm::vec3 edgeTwoVertexOne = polyhedronTwo.GetVertexPosition(edgeTwo.StartVertexIndex);
        const glm::vec3 edgeTwoVertexTwo = polyhedronTwo.GetVertexPosition(polyhedronTwo.GetHalfEdge(edgeTwo.TwinEdgeIndex).StartVertexIndex);
        const glm::vec3 dCrossC = edgeTwoVertexOne - edgeTwoVertexTwo;

        // Test if the two arcs of the Gauss Map intersect (therefore forming a minkowski face)
        // Note that we negate the normals of the second polyhedron because we are looking at the
        // Gauss map of the minkowski difference of the polyhedrons.
        return testGaussMapArcsIntersect(a, b, -c, -d, bCrossA, dCrossC);
    }

    bool SATAlgorithm::testGaussMapArcsIntersect(
        const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d, const glm::vec3 &bCrossA, const glm::vec3 &dCrossC) const {

        const f32 cba = glm::dot(c, bCrossA);
        const f32 dba = glm::dot(d, bCrossA);
        const f32 adc = glm::dot(a, dCrossC);
        const f32 bdc = glm::dot(b, dCrossC);

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

        bool collisionDetected = false;

        (void)isMinPenetrationFaceNormalPolyhedronOne;
        (void)polyhedronOne;
        (void)polyhedronTwo;
        (void)polyhedronOneToTwo;
        (void)polyhedronTwoToOne;
        (void)minFaceIndex;
        (void)batch;
        (void)batchIndex;

        return collisionDetected;
    }

} // namespace Vulkyrie
