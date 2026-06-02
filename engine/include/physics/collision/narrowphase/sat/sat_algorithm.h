#pragma once

#include "physics/collision/narrowphase/narrow_phase_data_batch.h"
#include "physics/collision/shapes/capsule_shape.h"
#include "physics/collision/shapes/convex_polyhedron_shape.h"
#include "physics/collision/shapes/sphere_shape.h"

namespace Vulkyrie {

    class SATAlgorithm final {
    public:
        SATAlgorithm(bool clipWithPreviousAxisIfStillColliding);

        // Delete the copy constructor and copy assignment operator.
        SATAlgorithm(const SATAlgorithm &) = delete;
        SATAlgorithm &operator=(const SATAlgorithm &) = delete;

        // Delete the move constructor and move assignment operator.
        SATAlgorithm(SATAlgorithm &&) = delete;
        SATAlgorithm &operator=(SATAlgorithm &&) = delete;

        /** @brief Default destructor. */
        ~SATAlgorithm() = default;

        bool PerformSphereVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);
        bool PerformCapsuleVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchIndex);
        bool PerformConvexPolyhedronVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);

        bool ComputeCapsulePolyhedronFaceContactPoints(size_t referenceFaceIndex,
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
                                                       bool isShapeOneCapsule) const;

        // This method returns true if an edge of a polyhedron and a capsule forms a face of the Minkowski Difference
        bool IsMinkowskiFaceCapsuleVsEdge(const glm::vec3 &capsuleSegment,
                                          const glm::vec3 &edgeAdjacentFaceOneNormal,
                                          const glm::vec3 &edgeAdjacentFace2Normal) const;

    private:
        constexpr static f32 SEPARATING_AXIS_RELATIVE_TOLERANCE = f32(1.002);
        constexpr static f32 SEPARATING_AXIS_ABSOLUTE_TOLERANCE = f32(0.0005);

        bool _clipWithPreviousAxisIfStillColliding;

        bool testEdgesBuildMinkowskiFace(const ConvexPolyhedronShape &polyhedronOne,
                                         const HalfEdgeMesh::Edge &edgeOne,
                                         const ConvexPolyhedronShape &polyhedronTwo,
                                         const HalfEdgeMesh::Edge &edgeTwo,
                                         const TransformComponent &polyhedronOneToTwo) const;

        bool testGaussMapArcsIntersect(
            const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d, const glm::vec3 &bCrossA, const glm::vec3 &dCrossC) const;

        f32 computeDistanceBetweenEdges(const glm::vec3 &edgeOneA,
                                        const glm::vec3 &edgeTwoA,
                                        const glm::vec3 &polyhedronOneCentroid,
                                        const glm::vec3 &polyhedronTwoCentroid,
                                        const glm::vec3 &edgeOneDirection,
                                        const glm::vec3 &edgeTwoDirection,
                                        bool isShapeOneTriangle,
                                        glm::vec3 &outSeparatingAxis) const;

        f32 testSingleFaceDirectionPolyhedronVsPolyhedron(const ConvexPolyhedronShape &polyhedronOne,
                                                          const ConvexPolyhedronShape &polyhedronTwo,
                                                          const TransformComponent &polyhedronOneToTwoTransform,
                                                          size_t faceIndex) const;

        f32 testFacesDirectionPolyhedronVsPolyhedron(const ConvexPolyhedronShape &polyhedronOne,
                                                     const ConvexPolyhedronShape &polyhedronTwo,
                                                     const TransformComponent &polyhedronOneToTwoTransform,
                                                     size_t &minFaceIndex) const;

        f32 computePolyhedronFaceVsSpherePenetrationDepth(size_t faceIndex,
                                                          const ConvexPolyhedronShape &polyhedron,
                                                          const SphereShape &sphere,
                                                          const glm::vec3 &sphereCenter) const;

        f32 computePolyhedronFaceVsCapsulePenetrationDepth(size_t polyhedronFaceIndex,
                                                           const ConvexPolyhedronShape &polyhedron,
                                                           const CapsuleShape &capsule,
                                                           const TransformComponent &polyhedronToCapsuleTransform,
                                                           glm::vec3 &outFaceNormalCapsuleSpace) const;

        f32 computeEdgeVsCapsuleInnerSegmentPenetrationDepth(const ConvexPolyhedronShape &polyhedron,
                                                             const CapsuleShape &capsule,
                                                             const glm::vec3 &capsuleSegmentAxis,
                                                             const glm::vec3 &edgeVertexOne,
                                                             const glm::vec3 &edgeDirectionCapsuleSpace,
                                                             const TransformComponent &polyhedronToCapsuleTransform,
                                                             glm::vec3 &outAxis) const;

        bool computePolyhedronVsPolyhedronFaceContactPoints(bool isMinPenetrationFaceNormalPolyhedronOne,
                                                            const ConvexPolyhedronShape *polyhedronOne,
                                                            const ConvexPolyhedronShape *polyhedronTwo,
                                                            const TransformComponent &polyhedronOneToTwo,
                                                            const TransformComponent &polyhedronTwoToOne,
                                                            size_t minFaceIndex,
                                                            NarrowPhaseDataBatch &batch,
                                                            size_t batchIndex) const;
    };

} // namespace Vulkyrie
