#pragma once

#include "vlkypch.h"
#include "core/constants.h"
#include "physics/collision/shapes/convex_polyhedron_shape.h"
#include "physics/types/half_edge_mesh.h"

namespace Vulkyrie {

    /** @brief Enumeration for specifying which side of a triangle to consider during raycasting. This is used in ray-triangle intersection tests to determine
     * whether the ray should be tested against the front face, back face, or both faces of the triangle. The options are as follows:
     * - Front: Only consider the front face of the triangle for raycasting. The front face is typically defined by the winding order of the triangle's vertices
     * (e.g., counter-clockwise order).
     * - Back: Only consider the back face of the triangle for raycasting. The back face is typically defined as the opposite of the front face (e.g., clockwise
     * order).
     * - FrontAndBack: Consider both the front and back faces of the triangle for raycasting. This means that the ray will be tested against both sides of the
     * triangle, and an intersection will be reported if the ray intersects either face. This option is useful when you want to detect intersections with the
     * triangle regardless of which side is hit, such as in cases where the triangle is double-sided or when you want to allow for backface hits in your
     * raycasting logic. */
    enum class TriangleRaycastSide : i32 {
        /** @brief Raycast against front triangle. */
        Front,

        /** @brief Raycast against back triangle. */
        Back,

        /** @brief Raycast against both front and back triangles. */
        FrontAndBack
    };

    /** @brief Triangle collision shape used internally by ConcaveMeshShape and HeightFieldShape to represent a single triangle within a concave mesh.
     * Instances are never created directly by the user — they are constructed by the owning concave shape during collision queries.
     *
     * Each triangle stores its three vertex positions, per-vertex smooth normals (for smooth mesh contact resolution), and a reference to the shared
     * HalfEdgeMesh that provides the topological data (faces, edges, vertices) common to all triangles of the parent mesh.
     *
     * TriangleShape implements ConvexPolyhedronShape so that GJK/EPA can operate on it directly. It reports 2 faces (front and back), 3 vertices, and
     * 6 half-edges. Its collision margin is always 0 and its volume is always 0. */
    class TriangleShape : public ConvexPolyhedronShape {
    public:
        /** @brief Constructs a TriangleShape from three vertex positions, per-vertex smooth normals, a shape identifier, and a shared half-edge structure.
         * @param vertices Array of exactly three vertex positions in local space. Must not form a degenerate (zero-area) triangle.
         * @param vertexNormals Array of exactly three pre-computed smooth normals, one per vertex, used for smooth mesh contact resolution.
         * @param shapeID Identifier reserved for temporal coherence in collision detection; currently unused by this engine.
         * @param halfEdgeStructure Reference to the shared HalfEdgeMesh that provides topological data for this triangle. Stored as a reference — the
         * caller must ensure the structure outlives this shape.
         */
        explicit TriangleShape(const glm::vec3 vertices[3], const glm::vec3 vertexNormals[3], u32 shapeID, HalfEdgeMesh &halfEdgeStructure);

        /** @brief Constructs a raycasting-only TriangleShape from three vertex positions without smooth normals.
         * The face normal and vertex normals are initialised to zero, so this constructor must only be used for raycasting — calling any method that
         * requires a valid normal (e.g. GetFaceNormal, computeSmoothLocalContactNormalForTriangle) will trigger an assertion.
         * @param vertices Array of exactly three vertex positions in local space.
         * @param shapeID Identifier reserved for temporal coherence in collision detection; currently unused by this engine.
         * @param halfEdgeStructure Reference to the shared HalfEdgeMesh that provides topological data for this triangle. Stored as a reference — the
         * caller must ensure the structure outlives this shape.
         */
        explicit TriangleShape(const glm::vec3 vertices[3], u32 shapeID, HalfEdgeMesh &halfEdgeStructure);

        // Delete the copy constructor and operator.
        TriangleShape(const TriangleShape &) = delete;
        TriangleShape &operator=(const TriangleShape &) = delete;

        // Delete the move constructor and operator.
        TriangleShape(TriangleShape &&) = delete;
        TriangleShape &operator=(TriangleShape &&) = delete;

        /** @brief Default destructor for TriangleShape. */
        virtual ~TriangleShape() override = default;

        /** @brief Applies smooth triangle mesh contact resolution to a contact between two shapes, at least one of which must be a TriangleShape.
         * Implements the technique from "Game Physics Pearls" (van den Bergen & Gregorius, ch. 5) to eliminate the internal-edge artefact that occurs
         * when a convex shape slides across adjacent triangles of a concave mesh: the raw triangle edge normal is replaced by the barycentric-interpolated
         * smooth normal at the contact point, and the contact point on the other shape is re-projected to stay aligned with the new normal.
         * @param shapeOne First collision shape in the contact pair. Must be a TriangleShape if shapeTwo is not.
         * @param shapeTwo Second collision shape in the contact pair. Must be a TriangleShape if shapeOne is not.
         * @param localContactPointShapeOne Local-space contact point on shapeOne. Updated in-place when shapeTwo is the triangle.
         * @param localContactPointShapeTwo Local-space contact point on shapeTwo. Updated in-place when shapeOne is the triangle.
         * @param shapeOneToWorld World-space transform of shapeOne.
         * @param shapeTwoToWorld World-space transform of shapeTwo.
         * @param penetrationDepth Penetration depth, used to re-project the contact point on the non-triangle shape.
         * @param outSmoothVertexNormal Input: raw world-space contact normal. Output: smoothed world-space contact normal directed from shape 1 to shape 2.
         */
        VE_INLINE static void ComputeSmoothTriangleMeshContact(const CollisionShape *shapeOne,
                                                               const CollisionShape *shapeTwo,
                                                               glm::vec3 &localContactPointShapeOne,
                                                               glm::vec3 &localContactPointShapeTwo,
                                                               const TransformComponent &shapeOneToWorld,
                                                               const TransformComponent &shapeTwoToWorld,
                                                               const f32 penetrationDepth,
                                                               glm::vec3 &outSmoothVertexNormal) {
            VASSERT(shapeOne->GetName() == CollisionShapeName::Triangle || shapeTwo->GetName() == CollisionShapeName::Triangle,
                    "At least one of the shapes must be a triangle shape to compute smooth triangle mesh contact.");

            const bool isShape1Triangle = shapeOne->GetName() == CollisionShapeName::Triangle;

            if (isShape1Triangle || shapeTwo->GetName() == CollisionShapeName::Triangle) {
                const TriangleShape *triangleShape =
                    isShape1Triangle ? static_cast<const TriangleShape *>(shapeOne) : static_cast<const TriangleShape *>(shapeTwo);

                // Compute the world-to-local inverse of the other shape's transform.
                const TransformComponent &otherToWorld = isShape1Triangle ? shapeTwoToWorld : shapeOneToWorld;
                const TransformComponent worldToOther = otherToWorld.Inverse();

                // Compute the smooth triangle mesh contact normal and recompute the local contact point on the other shape.
                triangleShape->computeSmoothMeshContact(isShape1Triangle ? localContactPointShapeOne : localContactPointShapeTwo,
                                                        isShape1Triangle ? shapeOneToWorld : shapeTwoToWorld,
                                                        worldToOther,
                                                        penetrationDepth,
                                                        isShape1Triangle,
                                                        isShape1Triangle ? localContactPointShapeTwo : localContactPointShapeOne,
                                                        outSmoothVertexNormal);
            }
        }

        /** @brief Returns the local-space AABB of the triangle, expanded by the collision margin on each side.
         * @returns The axis-aligned bounding box in local space.
         */
        [[nodiscard]] VE_INLINE virtual AABB GetLocalAABB() const override {
            const glm::vec3 xAxis = glm::vec3(_vertices[0].x, _vertices[1].x, _vertices[2].x);
            const glm::vec3 yAxis = glm::vec3(_vertices[0].y, _vertices[1].y, _vertices[2].y);
            const glm::vec3 zAxis = glm::vec3(_vertices[0].z, _vertices[1].z, _vertices[2].z);

            glm::vec3 min = glm::vec3(
                glm::min(xAxis.x, glm::min(xAxis.y, xAxis.z)), glm::min(yAxis.x, glm::min(yAxis.y, yAxis.z)), glm::min(zAxis.x, glm::min(zAxis.y, zAxis.z)));
            glm::vec3 max = glm::vec3(
                glm::max(xAxis.x, glm::max(xAxis.y, xAxis.z)), glm::max(yAxis.x, glm::max(yAxis.y, yAxis.z)), glm::max(zAxis.x, glm::max(zAxis.y, zAxis.z)));

            min -= glm::vec3(GetMargin());
            max += glm::vec3(GetMargin());

            return AABB(min, max);
        }

        /** @brief Returns a zero inertia tensor. A triangle is an infinitely thin 2D surface with no volume, so a meaningful inertia tensor is undefined.
         * @param mass Unused.
         * @returns A zero vector.
         */
        VE_INLINE constexpr virtual glm::vec3 GetLocalInertiaTensor([[maybe_unused]] f32 mass) const override {
            // A triangle is a 2D shape in 3D space, so it has no volume and thus an inertia tensor of zero.
            return glm::vec3(0.0f);
        }

        /** @brief Computes the world-space AABB enclosing the triangle by transforming all three vertices and taking the component-wise min/max.
         * @param transform The world-space transform to apply to the triangle's vertices.
         * @returns The axis-aligned bounding box enclosing the transformed triangle.
         */
        virtual AABB ComputeTransformedAABB(const TransformComponent &transform) const override;

        /** @brief Returns the local-space position of a triangle vertex.
         * @param vertexIndex Index of the vertex (0, 1, or 2).
         * @returns The position of the specified vertex in local space.
         */
        [[nodiscard]] VE_INLINE virtual glm::vec3 GetVertexPosition(size_t vertexIndex) const override {
            VASSERT(vertexIndex < 3, "Vertex index out of bounds for triangle shape. A triangle shape only has 3 vertices.");

            return _vertices[vertexIndex];
        }

        /** @brief Returns the outward-pointing unit normal of a triangle face. Face 0 is the front face; face 1 is the back face (negated normal).
         * @param faceIndex Index of the face (0 for front, 1 for back).
         * @returns The unit normal of the specified face in local space.
         */
        [[nodiscard]] VE_INLINE virtual glm::vec3 GetFaceNormal(size_t faceIndex) const override {
            VASSERT(faceIndex < 2, "Face index out of bounds for triangle shape. A triangle shape only has 2 faces (front and back).");
            VASSERT(glm::length2(_normal) > 0.0f, "Normal vector of the triangle shape should not be a zero vector.");

            return faceIndex == 0 ? _normal : -_normal;
        }

        /** @brief Returns 2. A triangle exposes both a front and a back face to the collision system.
         * @returns 2.
         */
        [[nodiscard]] VE_INLINE constexpr virtual size_t GetFacesCount() const override {
            return 2;
        }

        /** @brief Returns 3. A triangle has exactly three vertices.
         * @returns 3.
         */
        [[nodiscard]] VE_INLINE constexpr virtual size_t GetVerticesCount() const override {
            return 3;
        }

        /** @brief Returns 6. A triangle has 3 edges, each represented as two directed half-edges in the half-edge structure.
         * @returns 6.
         */
        [[nodiscard]] VE_INLINE constexpr virtual size_t GetHalfEdgesCount() const override {
            // A triangle has 3 edges, and each edge is represented by 2 half-edges
            // in a half-edge data structure, so the total number of half-edges is 6.
            return 6;
        }

        /** @brief Returns the centroid of the triangle, computed as the average of its three vertex positions.
         * @returns The centroid in local space.
         */
        [[nodiscard]] VE_INLINE virtual glm::vec3 GetCentroid() const override {
            return (_vertices[0] + _vertices[1] + _vertices[2]) / f32(3.0f);
        }

        /** @brief Returns 0. A triangle is an infinitely thin 2D surface and has no volume.
         * @returns 0.0f.
         */
        [[nodiscard]] VE_INLINE constexpr virtual f32 GetVolume() const override {
            return f32(0.0f);
        }

        /** @brief Returns the half-edge mesh face at the given index from the shared half-edge structure.
         * @param faceIndex Index of the face (0 or 1).
         * @returns A const reference to the requested face.
         */
        [[nodiscard]] VE_INLINE virtual const HalfEdgeMesh::Face &GetFace(size_t faceIndex) const override {
            VASSERT(faceIndex < GetFacesCount(), "Face index out of bounds for triangle shape. A triangle shape only has 2 faces (front and back).");

            return _halfEdgeStructure.GetFace(faceIndex);
        }

        /** @brief Returns the half-edge mesh vertex at the given index from the shared half-edge structure.
         * @param vertexIndex Index of the vertex (0, 1, or 2).
         * @returns A const reference to the requested vertex.
         */
        [[nodiscard]] VE_INLINE virtual const HalfEdgeMesh::Vertex &GetVertex(size_t vertexIndex) const override {
            VASSERT(vertexIndex < GetVerticesCount(), "Vertex index out of bounds for triangle shape. A triangle shape only has 3 vertices.");

            return _halfEdgeStructure.GetVertex(vertexIndex);
        }

        /** @brief Returns the half-edge at the given index from the shared half-edge structure.
         * @param edgeIndex Index of the half-edge (0 through 5).
         * @returns A const reference to the requested half-edge.
         */
        [[nodiscard]] VE_INLINE virtual const HalfEdgeMesh::Edge &GetHalfEdge(size_t edgeIndex) const override {
            VASSERT(edgeIndex < GetHalfEdgesCount(), "Half-edge index out of bounds for triangle shape. A triangle shape only has 6 half-edges.");

            return _halfEdgeStructure.GetHalfEdge(edgeIndex);
        }

        /** @brief Get all half-edges of the triangle shape.
         * @returns A const reference to a vector containing all `HalfEdgeMesh::Edge` instances of the shape.
         */
        [[nodiscard]] VE_INLINE const std::vector<HalfEdgeMesh::Edge> &GetHalfEdges() const override {
            return _halfEdgeStructure.GetHalfEdges();
        }

        /** @brief Returns the local-space support point in the given direction, ignoring the collision margin.
         * The support point is the vertex with the largest projection onto the direction vector.
         * @param direction The query direction in local space. Does not need to be normalized.
         * @returns The vertex position furthest along the given direction.
         */
        [[nodiscard]] VE_INLINE virtual glm::vec3 GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const override {
            glm::vec3 dotProducts = glm::vec3(glm::dot(direction, _vertices[0]), glm::dot(direction, _vertices[1]), glm::dot(direction, _vertices[2]));

            // The support point in a given direction is the vertex that has the largest dot product with that direction vector.
            // This is because the dot product measures how much one vector extends in the direction of another vector,
            // so the vertex with the largest dot product will be the furthest point in that direction.
            if (dotProducts.x > dotProducts.y) {
                if (dotProducts.x > dotProducts.z) {
                    return _vertices[0];
                } else {
                    return _vertices[2];
                }
            } else {
                if (dotProducts.y > dotProducts.z) {
                    return _vertices[1];
                } else {
                    return _vertices[2];
                }
            }
        }

        /** @brief Always returns false. A triangle is an infinitely thin 2D surface and contains no interior volume.
         * @param point Unused.
         * @returns false.
         */
        [[nodiscard]] VE_INLINE constexpr virtual bool ContainsPoint([[maybe_unused]] const glm::vec3 &point) const override {
            // A triangle is a 2D shape in 3D space, so it does not contain any points in its local space.
            // It only contains points that lie on the plane of the triangle and within the triangle's boundaries,
            // but since the triangle has no thickness, it cannot contain any points in its local space.
            return false;
        }

        /** @brief Returns which face(s) of this triangle are tested during ray intersection queries.
         * @returns The current TriangleRaycastSide setting (Front, Back, or FrontAndBack).
         */
        [[nodiscard]] VE_INLINE TriangleRaycastSide GetRaycastSide() const {
            return _raycastSide;
        }

        /** @brief Sets which face(s) of this triangle are tested during ray intersection queries.
         * @param raycastSide The desired side to test (Front, Back, or FrontAndBack).
         */
        VE_INLINE void SetRaycastSide(TriangleRaycastSide raycastSide) {
            _raycastSide = raycastSide;
        }

    private:
        /** @brief The three vertex positions of the triangle in local space. */
        glm::vec3 _vertices[3];

        /** @brief Per-vertex smooth normals in local space, pre-computed from the parent mesh topology. Used by computeSmoothLocalContactNormalForTriangle
         * to interpolate a smooth contact normal at edge and vertex contacts, avoiding the internal-edge artefact. Zero-initialised when the triangle is
         * constructed for raycasting only. */
        glm::vec3 _vertexNormals[3];

        /** @brief The unit face normal of the triangle in local space, computed from the cross product of the two edge vectors during construction.
         * Zero-initialised when the triangle is constructed for raycasting only (the raycasting constructor does not require a valid normal). */
        glm::vec3 _normal;

        /** @brief Specifies which face(s) are tested during ray intersection queries (front, back, or both). Defaults to Front. */
        TriangleRaycastSide _raycastSide;

        /** @brief Reference to the shared HalfEdgeMesh that provides topological data (faces, half-edges, vertices) for this triangle. Shared across all
         * triangles of the parent concave mesh, so stored as a reference rather than a copy to avoid redundant allocations. */
        HalfEdgeMesh &_halfEdgeStructure;

        /** @brief Computes the smooth contact normal at the given local contact point using barycentric interpolation of the stored vertex normals.
         * Returns the flat face normal directly when the contact point lies strictly inside the triangle. If the interpolated result is shorter than
         * sqrt(MACHINE_EPSILON) (vertex normals cancel), the flat face normal is returned as a fallback to avoid normalising a near-zero vector.
         * @param localContactPoint The contact point in the triangle's local space.
         * @returns The smooth contact normal in local space (unit length).
         */
        [[nodiscard]] VE_INLINE glm::vec3 computeSmoothLocalContactNormalForTriangle(const glm::vec3 &localContactPoint) const {
            VASSERT(glm::length2(_normal) > 0.0f, "Normal vector of the triangle shape should not be a zero vector.");

            // Compute the barycentric coordinates of the local contact point with respect to the triangle vertices.
            const glm::vec3 v0 = _vertices[1] - _vertices[0];
            const glm::vec3 v1 = _vertices[2] - _vertices[0];
            const glm::vec3 v2 = localContactPoint - _vertices[0];

            const f32 d00 = glm::dot(v0, v0);
            const f32 d01 = glm::dot(v0, v1);
            const f32 d11 = glm::dot(v1, v1);
            const f32 d20 = glm::dot(v2, v0);
            const f32 d21 = glm::dot(v2, v1);

            const f32 denom = d00 * d11 - d01 * d01;
            VASSERT(denom != 0.0f, "Degenerate triangle shape with zero area should not be used for collision detection.");

            const f32 v = (d11 * d20 - d01 * d21) / denom;
            const f32 w = (d00 * d21 - d01 * d20) / denom;
            const f32 u = 1.0f - v - w;

            if (u > VE_MACHINE_EPSILON && v > VE_MACHINE_EPSILON && w > VE_MACHINE_EPSILON) {
                // If the contact point is strictly inside the triangle, we can directly use the triangle's normal for the contact normal.
                return _normal;
            }

            // Interpolate the vertex normals using the barycentric coordinates to get a smooth normal at the contact point.
            const glm::vec3 smoothNormal = u * _vertexNormals[0] + v * _vertexNormals[1] + w * _vertexNormals[2];

            // Guard against a degenerate interpolated normal (e.g. vertex normals cancel). Comparing the squared
            // length against MACHINE_EPSILON (not MACHINE_EPSILON²) means we reject any vector shorter than
            // sqrt(MACHINE_EPSILON) ≈ 3.4e-4 — the threshold below which normalization amplifies floating-point
            // noise enough to produce a meaningless direction. Fall back to the flat triangle normal.
            if (glm::length2(smoothNormal) < VE_MACHINE_EPSILON) {
                return _normal;
            }

            return glm::normalize(smoothNormal);
        }

        /** @brief Re-computes the smooth world contact normal and the local contact point on the other shape for a triangle mesh contact.
         * @param localContactPointTriangle The contact point in this triangle's local space.
         * @param triangleShapeToWorldTransform The world-space transform of this triangle.
         * @param worldToOtherShapeTransform The inverse world-space transform of the other shape (world-to-local).
         * @param penetrationDepth The penetration depth of the contact.
         * @param isTriangleShape1 True if this triangle is shape 1 in the contact pair; false if it is shape 2.
         * @param outNewLocalContactPointOtherShape Output: the re-projected contact point in the other shape's local space.
         * @param outSmoothWorldContactTriangleNormal Input/output: world-space contact normal, updated to the smooth value on output.
         */
        void computeSmoothMeshContact(glm::vec3 localContactPointTriangle,
                                      const TransformComponent &triangleShapeToWorldTransform,
                                      const TransformComponent &worldToOtherShapeTransform,
                                      const f32 penetrationDepth,
                                      const bool isTriangleShape1,
                                      glm::vec3 &outNewLocalContactPointOtherShape,
                                      glm::vec3 &outSmoothWorldContactTriangleNormal) const;
    };

} // namespace Vulkyrie
