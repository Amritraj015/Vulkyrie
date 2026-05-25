#pragma once

#include "physics/collision/shapes/convex_polyhedron_shape.h"
#include "vlkypch.h"

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
        FrontAndBback
    };

    class TriangleShape : public ConvexPolyhedronShape {
    public:
    private:
    };

} // namespace Vulkyrie
