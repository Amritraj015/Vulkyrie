#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/convex_shape.h"
#include "physics/types/half_edge_mesh.h"

namespace Vulkyrie {

    /** @brief The `ConvexPolyhedronShape` class represents a convex polyhedral collision shape, which is a type of convex shape defined by flat faces, straight
     * edges, and sharp vertices. This class provides an interface for accessing the geometric properties of the convex polyhedron, such as its faces, vertices,
     * and half edges. It is designed to be used in collision detection and physics simulations where accurate representation of complex convex shapes is
     * required. */
    class ConvexPolyhedronShape : public ConvexShape {
    public:
        /** @brief Construct a convex polyhedron shape with the specified name and margin.
         * @param name The specific name of the convex polyhedron shape. This is used to identify the exact type of convex polyhedron shape.
         * @param margin The margin to be applied to the convex polyhedron shape for collision detection purposes. This is an optional parameter that
         * defaults to 0.0f if not provided. A positive margin can help improve collision detection stability by providing a small buffer around the shape.
         * @param id The unique identifier of the convex polyhedron shape in the overlapping pair.
         */
        explicit ConvexPolyhedronShape(CollisionShapeName name, f32 margin = 0.0f, u32 id = 0);

        VE_DELETE_MOVE_AND_COPY(ConvexPolyhedronShape);

        /** @brief Destructor for the ConvexPolyhedronShape class. */
        virtual ~ConvexPolyhedronShape() override = default;

        /** @brief Get the number of faces of the convex polyhedron shape.
         * @returns The number of faces of the convex polyhedron shape. This is typically equal to the number of flat surfaces that make up the shape.
         */
        virtual size_t GetFacesCount() const = 0;

        /** @brief Get the number of vertices of the convex polyhedron shape.
         * @returns The number of vertices of the convex polyhedron shape. This is typically equal to the number of corners or points where edges meet on the
         * shape.
         */
        virtual size_t GetVerticesCount() const = 0;

        /** @brief Get the number of half edges of the convex polyhedron shape.
         * @returns The number of half edges of the convex polyhedron shape. This is typically equal to twice the number of edges, since each edge is
         * represented by two half edges in a half-edge data structure.
         */
        virtual size_t GetHalfEdgesCount() const = 0;

        /** @brief Get the position of a specific vertex of the convex polyhedron shape.
         * @param vertexIndex The index of the vertex for which to retrieve the position. The valid range for this index is from 0 to GetVerticesCount()
         * - 1.
         * @returns The position of the specified vertex as a glm::vec3. The position is given in the local coordinate space of the shape, where the origin
         * is typically at the centroid of the shape.
         */
        virtual glm::vec3 GetVertexPosition(size_t vertexIndex) const = 0;

        /** @brief Get the normal vector of a specific face of the convex polyhedron shape.
         * @param faceIndex The index of the face for which to retrieve the normal vector. The valid range for this index is from 0 to GetFacesCount() - 1.
         * @returns The normal vector of the specified face as a glm::vec3. The normal vector is a unit vector that is perpendicular to the face and points
         * outward from the surface of the shape.
         */
        virtual glm::vec3 GetFaceNormal(size_t faceIndex) const = 0;

        /** @brief Get the centroid of the convex polyhedron shape.
         * @returns The centroid of the convex polyhedron shape as a glm::vec3. The centroid is the geometric center of the shape, calculated as the average
         * position of all its vertices.
         */
        virtual glm::vec3 GetCentroid() const = 0;

        /** @brief Get a face of the convex polyhedron shape by index.
         * @param faceIndex The index of the face to retrieve. The valid range is from 0 to GetFacesCount() - 1.
         * @returns A const reference to the `HalfEdgeMesh::Face` at the given index.
         */
        virtual const HalfEdgeMesh::Face &GetFace(size_t faceIndex) const = 0;

        /** @brief Get a half-edge of the convex polyhedron shape by index.
         * @param edgeIndex The index of the half-edge to retrieve. The valid range is from 0 to GetHafEdgesCount() - 1.
         * @returns A const reference to the `HalfEdgeMesh::Edge` at the given index.
         */
        virtual const HalfEdgeMesh::Edge &GetHalfEdge(size_t edgeIndex) const = 0;

        /** @brief Get all half-edges of the convex polyhedron shape.
         * @returns A const reference to a vector containing all `HalfEdgeMesh::Edge` instances of the shape.
         */
        virtual const std::vector<HalfEdgeMesh::Edge> &GetHalfEdges() const = 0;

        /** @brief Get a vertex of the convex polyhedron shape by index.
         * @param vertexIndex The index of the vertex to retrieve. The valid range is from 0 to GetVerticesCount() - 1.
         * @returns A const reference to the `HalfEdgeMesh::Vertex` at the given index.
         */
        virtual const HalfEdgeMesh::Vertex &GetVertex(size_t vertexIndex) const = 0;

        /** @brief Find the index of the face whose outward normal is most anti-parallel to the given direction. This is used in collision detection algorithms
         * (e.g., SAT, GJK/EPA) to quickly identify the face most likely to be the reference face for a contact manifold — i.e., the face pointing most
         * directly against the query direction.
         * @param direction The direction vector to test against each face normal. Does not need to be normalized.
         * @returns The index of the face with the smallest dot product between its normal and `direction`.
         */
        [[nodiscard]] VE_INLINE size_t FindMostAntiParallelFaceIndex(const glm::vec3 &direction) const {
            f32 minDotProduct = std::numeric_limits<f32>::max();
            size_t mostAntiParallelFaceIndex = 0;

            for (size_t i = 0; i < GetFacesCount(); ++i) {
                const glm::vec3 faceNormal = GetFaceNormal(i);
                const f32 dotProduct = glm::dot(faceNormal, direction);

                if (dotProduct < minDotProduct) {
                    minDotProduct = dotProduct;
                    mostAntiParallelFaceIndex = i;
                }
            }

            return mostAntiParallelFaceIndex;
        }

        /** @brief Check if the collision shape is polyhedral.
         * @returns True if the collision shape is polyhedral, false otherwise.
         */
        VE_INLINE constexpr bool IsPolyhedral() const override {
            return true;
        }
    };

} // namespace Vulkyrie
