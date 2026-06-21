#pragma once

#include "vlkypch.h"
#include "physics/collision/narrowphase/narrow_phase_data_batch.h"
#include "physics/collision/shapes/capsule_shape.h"
#include "physics/collision/shapes/convex_polyhedron_shape.h"
#include "physics/collision/shapes/sphere_shape.h"

namespace Vulkyrie {

    /** @brief Separating Axis Theorem (SAT) algorithm for narrow-phase collision detection between convex shapes.
     *
     * Tests for overlap between pairs of convex shapes and, when a collision is found, computes contact points.
     * Supported shape pairs are sphere vs convex polyhedron, capsule vs convex polyhedron, and convex
     * polyhedron vs convex polyhedron.
     *
     * For polyhedron pairs the algorithm searches for a separating axis by testing all face normals of both
     * shapes and the cross products of all unique edge pairs. The Gauss Map is used to prune edge pairs that
     * cannot produce a separating axis before computing the full penetration depth. When no separating axis
     * is found, the axis of minimum penetration depth is selected and Sutherland-Hodgman clipping is applied
     * to the colliding features to build the contact manifold.
     *
     * Temporal coherence: the previous frame's minimum-penetration axis is cached and re-tested at the start
     * of each frame. If it is still valid, the full axis search is skipped, improving performance for shapes
     * in persistent contact. */
    class SATAlgorithm final {
    public:
        /** @brief Constructs a new SATAlgorithm.
         * @param clipWithPreviousAxisIfStillColliding When true, re-uses the previous frame's minimum-penetration
         * axis to build the contact manifold without re-running the full search, provided the shapes are still
         * overlapping on that axis. Should be true for dynamic simulation (better stability via temporal coherence)
         * and false when querying discrete collision tests where the true minimum axis is always required. */
        SATAlgorithm(bool clipWithPreviousAxisIfStillColliding);

        VE_DELETE_MOVE_AND_COPY(SATAlgorithm);

        /** @brief Default destructor. */
        ~SATAlgorithm() = default;

        /** @brief Returns true if a polyhedron edge and the capsule inner segment form a face of the Minkowski
         * difference, meaning their cross product is a valid candidate separating axis. This is the Gauss Map
         * arc-intersection test specialized for the capsule case: the arc on the Gauss Map corresponding to the
         * polyhedron edge must intersect the unit circle plane corresponding to the capsule Gauss Map.
         * @param capsuleSegment Direction vector of the capsule inner segment.
         * @param edgeAdjacentFaceOneNormal Outward normal of the first face adjacent to the polyhedron edge, in capsule local space.
         * @param edgeAdjacentFaceTwoNormal Outward normal of the second face adjacent to the polyhedron edge, in capsule local space.
         * @returns True if the Gauss Map arcs corresponding to the capsule segment and the polyhedron edge intersect. */
        [[nodiscard]] VE_INLINE bool IsMinkowskiFaceCapsuleVsEdge(const glm::vec3 &capsuleSegment,
                                                                  const glm::vec3 &edgeAdjacentFaceOneNormal,
                                                                  const glm::vec3 &edgeAdjacentFaceTwoNormal) const {
            return glm::dot(capsuleSegment, edgeAdjacentFaceOneNormal) * glm::dot(capsuleSegment, edgeAdjacentFaceTwoNormal) < 0.0f;
        }

        /** @brief Tests a batch of sphere vs convex polyhedron shape pairs for collision and computes contact information.
         * Iterates all face normals of each polyhedron as candidate separating axes. If no separating axis is
         * found, the face of minimum penetration depth is used to generate a single contact point.
         * @param batch Batch of narrow-phase data containing shape pairs and their world transforms.
         * @param batchStartIndex Index of the first pair in the batch to process.
         * @param batchItemsCount Number of pairs to process.
         * @returns True if any collision was detected in the processed batch. */
        bool PerformSphereVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);

        /** @brief Tests a single capsule vs convex polyhedron shape pair for collision and computes contact information.
         * Iterates face normals and then unique edge-pair cross products as candidate separating axes. If the
         * minimum-penetration axis is a face normal, up to two contact points are produced by clipping the capsule
         * inner segment against the reference face; if it is an edge-edge axis, a single contact point is produced
         * from the closest points between the two edges.
         * @param batch Batch of narrow-phase data containing shape pairs and their world transforms.
         * @param batchIndex Index of the pair within the batch to test.
         * @returns True if a collision was detected. */
        bool PerformCapsuleVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchIndex);

        /** @brief Tests a batch of convex polyhedron vs convex polyhedron shape pairs for collision and computes
         * contact information. For each pair, face normals of both polyhedra and unique edge-pair cross products
         * are tested as candidate separating axes. Temporal coherence is applied to skip the full search when
         * the previous frame's axis is still valid. Contact manifolds are built by Sutherland-Hodgman clipping
         * for face-face contacts, or from the closest points on the two edges for edge-edge contacts.
         * @param batch Batch of narrow-phase data containing shape pairs and their world transforms.
         * @param batchStartIndex Index of the first pair in the batch to process.
         * @param batchItemsCount Number of pairs to process.
         * @returns True if any collision was detected in the processed batch. */
        bool PerformConvexPolyhedronVsConvexPolyhedronCollisionCheck(NarrowPhaseDataBatch &batch, size_t batchStartIndex, size_t batchItemsCount);

        /** @brief Computes contact points between a capsule and a polyhedron face when the minimum-penetration
         * separating axis is a face normal of the polyhedron. Clips the capsule inner segment against the side
         * planes of the reference face using Sutherland-Hodgman clipping, then projects the surviving endpoints
         * onto the face plane to produce up to two contact points.
         * @param referenceFaceIndex Index of the polyhedron face whose normal is the minimum-penetration axis.
         * @param capsuleRadius Radius of the capsule.
         * @param polyhedron The convex polyhedron shape.
         * @param penetrationDepth Penetration depth along the separating axis.
         * @param polyhedronToCapsuleTransform Transform from polyhedron local space to capsule local space.
         * @param normalWorld Contact normal in world space, pointing from the polyhedron toward the capsule.
         * @param separatingAxisCapsuleSpace Separating axis expressed in capsule local space.
         * @param capsuleSegAPolyhedronSpace Start endpoint of the capsule inner segment in polyhedron local space.
         * @param capsuleSegBPolyhedronSpace End endpoint of the capsule inner segment in polyhedron local space.
         * @param batch Batch of narrow-phase data to write contact points into.
         * @param batchIndex Index of the pair within the batch.
         * @param isShapeOneCapsule True if the capsule is shape one in the pair, false if it is shape two.
         * @returns True if at least one valid contact point was found and added to the batch. */
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

    private:
        constexpr static f32 SEPARATING_AXIS_RELATIVE_TOLERANCE = f32(1.002);
        constexpr static f32 SEPARATING_AXIS_ABSOLUTE_TOLERANCE = f32(0.0005);

        bool _clipWithPreviousAxisIfStillColliding;

        /** @brief Returns true if the cross product of two half-edges (one from each polyhedron) is a valid
         * candidate separating axis, i.e., the edges form a face of the Minkowski difference. Uses the Gauss
         * Map arc-intersection test to determine this efficiently without computing the full penetration depth.
         * @param polyhedronOne The first convex polyhedron.
         * @param edgeOne A half-edge from polyhedronOne.
         * @param polyhedronTwo The second convex polyhedron.
         * @param edgeTwo A half-edge from polyhedronTwo.
         * @param polyhedronOneToTwo Transform from polyhedronOne local space to polyhedronTwo local space.
         * @returns True if the two edges form a Minkowski face and their cross product should be tested as a separating axis. */
        bool testEdgesBuildMinkowskiFace(const ConvexPolyhedronShape &polyhedronOne,
                                         const HalfEdgeMesh::Edge &edgeOne,
                                         const ConvexPolyhedronShape &polyhedronTwo,
                                         const HalfEdgeMesh::Edge &edgeTwo,
                                         const TransformComponent &polyhedronOneToTwo) const;

        /** @brief Returns true if arc AB and arc CD on the Gauss Map (unit sphere) intersect. An intersection
         * means the cross product of the two corresponding edges is a valid candidate separating axis for the
         * Minkowski difference of the two polyhedra.
         * @param a Normal of the first face adjacent to edge AB on polyhedronOne, in shared space.
         * @param b Normal of the second face adjacent to edge AB on polyhedronOne, in shared space.
         * @param c Negated normal of the first face adjacent to edge CD on polyhedronTwo.
         * @param d Negated normal of the second face adjacent to edge CD on polyhedronTwo.
         * @param bCrossA Cross product b × a, approximated using the direction of edge AB.
         * @param dCrossC Cross product d × c, approximated using the direction of edge CD.
         * @returns True if arcs AB and CD intersect on the Gauss Map. */
        bool testGaussMapArcsIntersect(
            const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d, const glm::vec3 &bCrossA, const glm::vec3 &dCrossC) const;

        /** @brief Computes the signed separation distance between two edges along their cross product axis.
         * A positive return value indicates penetration along the axis; a negative value indicates separation
         * and the axis is a valid separating axis. Parallel edges are skipped by returning
         * `std::numeric_limits<f32>::max()`.
         * @param edgeOneA A point on the first edge, in polyhedronTwo local space.
         * @param edgeTwoA A point on the second edge, in polyhedronTwo local space.
         * @param polyhedronOneCentroid Centroid of polyhedronOne transformed into polyhedronTwo local space.
         * @param polyhedronTwoCentroid Centroid of polyhedronTwo in its own local space.
         * @param edgeOneDirection Direction vector of the first edge.
         * @param edgeTwoDirection Direction vector of the second edge.
         * @param isShapeOneTriangle True if polyhedronOne is a triangle shape; changes how the axis orientation is disambiguated.
         * @param outSeparatingAxis Output: the candidate separating axis (normalized, pointing from polyhedronOne toward polyhedronTwo).
         * @returns Signed penetration depth along the candidate axis, or `std::numeric_limits<f32>::max()` if the edges are parallel. */
        f32 computeDistanceBetweenEdges(const glm::vec3 &edgeOneA,
                                        const glm::vec3 &edgeTwoA,
                                        const glm::vec3 &polyhedronOneCentroid,
                                        const glm::vec3 &polyhedronTwoCentroid,
                                        const glm::vec3 &edgeOneDirection,
                                        const glm::vec3 &edgeTwoDirection,
                                        bool isShapeOneTriangle,
                                        glm::vec3 &outSeparatingAxis) const;

        /** @brief Computes the penetration depth between two polyhedra along the outward normal of a single face
         * of polyhedronOne. A positive return value indicates overlap; zero or negative means this face normal is
         * a valid separating axis.
         * @param polyhedronOne The polyhedron whose face normal is being tested.
         * @param polyhedronTwo The opposing polyhedron.
         * @param polyhedronOneToTwoTransform Transform from polyhedronOne local space to polyhedronTwo local space.
         * @param faceIndex Index of the face on polyhedronOne whose normal is tested.
         * @returns Signed penetration depth along the face normal. A zero or negative value means the shapes are separated on this axis. */
        f32 testSingleFaceDirectionPolyhedronVsPolyhedron(const ConvexPolyhedronShape &polyhedronOne,
                                                          const ConvexPolyhedronShape &polyhedronTwo,
                                                          const TransformComponent &polyhedronOneToTwoTransform,
                                                          size_t faceIndex) const;

        /** @brief Tests all face normals of polyhedronOne as candidate separating axes against polyhedronTwo.
         * Returns early as soon as a separating axis is found.
         * @param polyhedronOne The polyhedron whose face normals are tested.
         * @param polyhedronTwo The opposing polyhedron.
         * @param polyhedronOneToTwoTransform Transform from polyhedronOne local space to polyhedronTwo local space.
         * @param minFaceIndex Output: index of the face with the minimum penetration depth, or the first separating face if one exists.
         * @returns The minimum penetration depth found, or the first non-positive depth encountered if a separating axis exists. */
        f32 testFacesDirectionPolyhedronVsPolyhedron(const ConvexPolyhedronShape &polyhedronOne,
                                                     const ConvexPolyhedronShape &polyhedronTwo,
                                                     const TransformComponent &polyhedronOneToTwoTransform,
                                                     size_t &minFaceIndex) const;

        /** @brief Computes the penetration depth between a sphere and a single polyhedron face along the face normal.
         * @param faceIndex Index of the face on the polyhedron to test.
         * @param polyhedron The convex polyhedron shape.
         * @param sphere The sphere shape.
         * @param sphereCenter Center of the sphere in polyhedron local space.
         * @returns Signed penetration depth along the face normal. A zero or negative value means this face normal separates the shapes. */
        f32 computePolyhedronFaceVsSpherePenetrationDepth(size_t faceIndex,
                                                          const ConvexPolyhedronShape &polyhedron,
                                                          const SphereShape &sphere,
                                                          const glm::vec3 &sphereCenter) const;

        /** @brief Computes the penetration depth between a capsule and a single polyhedron face along the face normal.
         * Uses the farthest capsule support point in the direction opposite to the face normal to measure overlap.
         * @param polyhedronFaceIndex Index of the face on the polyhedron to test.
         * @param polyhedron The convex polyhedron shape.
         * @param capsule The capsule shape.
         * @param polyhedronToCapsuleTransform Transform from polyhedron local space to capsule local space.
         * @param outFaceNormalCapsuleSpace Output: the face normal expressed in capsule local space.
         * @returns Signed penetration depth along the face normal. A zero or negative value means this face normal separates the shapes. */
        f32 computePolyhedronFaceVsCapsulePenetrationDepth(size_t polyhedronFaceIndex,
                                                           const ConvexPolyhedronShape &polyhedron,
                                                           const CapsuleShape &capsule,
                                                           const TransformComponent &polyhedronToCapsuleTransform,
                                                           glm::vec3 &outFaceNormalCapsuleSpace) const;

        /** @brief Computes the penetration depth between a capsule and a polyhedron edge along the axis formed
         * by the cross product of the capsule inner segment and the edge direction. If the edge is parallel to
         * the capsule segment the test is skipped and `std::numeric_limits<f32>::max()` is returned.
         * @param polyhedron The convex polyhedron shape.
         * @param capsule The capsule shape.
         * @param capsuleSegmentAxis Direction vector of the capsule inner segment in capsule local space.
         * @param edgeVertexOne First vertex of the polyhedron edge in polyhedron local space.
         * @param edgeDirectionCapsuleSpace Direction of the polyhedron edge expressed in capsule local space.
         * @param polyhedronToCapsuleTransform Transform from polyhedron local space to capsule local space.
         * @param outAxis Output: the candidate separating axis (normalized, pointing away from the polyhedron centroid).
         * @returns Signed penetration depth along the candidate axis, or `std::numeric_limits<f32>::max()` if the edge is parallel to the capsule segment. */
        f32 computeEdgeVsCapsuleInnerSegmentPenetrationDepth(const ConvexPolyhedronShape &polyhedron,
                                                             const CapsuleShape &capsule,
                                                             const glm::vec3 &capsuleSegmentAxis,
                                                             const glm::vec3 &edgeVertexOne,
                                                             const glm::vec3 &edgeDirectionCapsuleSpace,
                                                             const TransformComponent &polyhedronToCapsuleTransform,
                                                             glm::vec3 &outAxis) const;

        /** @brief Computes contact points between two convex polyhedra when the minimum-penetration axis is a
         * face normal of one of the polyhedra. Identifies the reference face (whose normal is the separating axis)
         * and the most anti-parallel incident face on the other polyhedron, then clips the incident face polygon
         * against the side planes of the reference face using Sutherland-Hodgman clipping. Only clipped vertices
         * that lie below the reference face plane are kept as contact points.
         * @param isMinPenetrationFaceNormalPolyhedronOne True if the reference face belongs to polyhedronOne,
         * false if it belongs to polyhedronTwo.
         * @param polyhedronOne Pointer to the first convex polyhedron.
         * @param polyhedronTwo Pointer to the second convex polyhedron.
         * @param polyhedronOneToTwo Transform from polyhedronOne local space to polyhedronTwo local space.
         * @param polyhedronTwoToOne Transform from polyhedronTwo local space to polyhedronOne local space.
         * @param minFaceIndex Index of the reference face on the reference polyhedron.
         * @param batch Batch of narrow-phase data to write contact points into.
         * @param batchIndex Index of the pair within the batch.
         * @returns True if at least one valid contact point was found and added to the batch. */
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
