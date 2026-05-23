#pragma once

#include "physics/collision/shapes/convex_shape.h"

namespace Vulkyrie {

    class Face {};

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
         */
        ConvexPolyhedronShape(CollisionShapeName name, f32 margin = 0.0f);

        /** @brief Destructor for the ConvexPolyhedronShape class. */
        virtual ~ConvexPolyhedronShape() override = default;

        // Delete the copy constructor and operator.
        ConvexPolyhedronShape(const ConvexPolyhedronShape &) = delete;
        ConvexPolyhedronShape &operator=(const ConvexPolyhedronShape &) = delete;

        // Delete the move constructor and operator.
        ConvexPolyhedronShape(ConvexPolyhedronShape &&) = delete;
        ConvexPolyhedronShape &operator=(ConvexPolyhedronShape &&) = delete;

        /** @brief Get the number of faces of the convex polyhedron shape.
         * @returns The number of faces of the convex polyhedron shape. This is typically equal to the number of flat surfaces that make up the shape.
         */
        virtual u32 GetFacesCount() const = 0;

        /** @brief Get the number of vertices of the convex polyhedron shape.
         * @returns The number of vertices of the convex polyhedron shape. This is typically equal to the number of corners or points where edges meet on the
         * shape.
         */
        virtual u32 GetVerticesCount() const = 0;

        /** @brief Get the number of half edges of the convex polyhedron shape.
         * @returns The number of half edges of the convex polyhedron shape. This is typically equal to twice the number of edges, since each edge is
         * represented by two half edges in a half-edge data structure.
         */
        virtual u32 GetHafEdgesCount() const = 0;

        /** @brief Get the position of a specific vertex of the convex polyhedron shape.
         * @param vertexIndex The index of the vertex for which to retrieve the position. The valid range for this index is from 0 to GetVerticesCount()
         * - 1.
         * @returns The position of the specified vertex as a glm::vec3. The position is given in the local coordinate space of the shape, where the origin
         * is typically at the centroid of the shape.
         */
        virtual glm::vec3 GetVertexPosition(u32 vertexIndex) const = 0;

        /** @brief Get the normal vector of a specific face of the convex polyhedron shape.
         * @param faceIndex The index of the face for which to retrieve the normal vector. The valid range for this index is from 0 to GetFacesCount() - 1.
         * @returns The normal vector of the specified face as a glm::vec3. The normal vector is a unit vector that is perpendicular to the face and points
         * outward from the surface of the shape.
         */
        virtual glm::vec3 GetFaceNormal(u32 faceIndex) const = 0;

        /** @brief Get the centroid of the convex polyhedron shape.
         * @returns The centroid of the convex polyhedron shape as a glm::vec3. The centroid is the geometric center of the shape, calculated as the average
         * position of all its vertices.
         */
        virtual glm::vec3 GetCentroid() const = 0;

        /** @brief Check if the collision shape is polyhedral.
         * @returns True if the collision shape is polyhedral, false otherwise.
         */
        VE_INLINE constexpr bool IsPolyhedral() const override {
            return true;
        }
    };

} // namespace Vulkyrie
