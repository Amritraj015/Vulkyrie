#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/shapes/triangle_shape.h"

namespace Vulkyrie {

    /** @brief The `ConcaveShape` class serves as the base class for all concave collision shapes, such as triangle meshes and heightfields. Unlike convex
     * shapes, concave shapes can represent complex geometry with inward-facing surfaces and are not closed under linear interpolation. They are typically used
     * for static geometry (e.g., terrain, walls) since concave shapes do not support dynamic rigid body simulation directly. This class extends
     * `CollisionShape` and adds support for non-uniform scaling and configurable raycast test sides for triangle-based queries. */
    class ConcaveShape : public CollisionShape {
    public:
        /** @brief Construct a concave shape with the specified name and scale.
         * @param name The specific name of the collision shape (e.g., TriangleMesh, Heightfield).
         * @param scale The non-uniform scale applied to the concave shape in local space.
         */
        ConcaveShape(CollisionShapeName name, const glm::vec3 &scale);

        VE_DELETE_MOVE_AND_COPY(ConcaveShape);

        /** @brief Default destructor. */
        virtual ~ConcaveShape() override = default;

        /** @brief Get the non-uniform scale of the concave shape.
         * @returns The scale vector applied to the shape in local space.
         */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetScale() const {
            return _scale;
        }

        /** @brief Set the non-uniform scale of the concave shape and notify all attached colliders of the change.
         * @param scale The new scale vector to apply to the shape in local space.
         */
        VE_INLINE void SetScale(const glm::vec3 &scale) {
            _scale = scale;

            NotifyCollidersOfShapeChange();
        }

        /** @brief Get which side(s) of triangles are tested during raycasting.
         * @returns The `TriangleRaycastSide` value indicating whether raycasts are tested against the front face, back face, or both faces of triangles.
         */
        [[nodiscard]] VE_INLINE TriangleRaycastSide GetTriangleRaycastSide() const {
            return _triangleRaycastSide;
        }

        /** @brief Set which side(s) of triangles are tested during raycasting.
         * @param triangleRaycastSide The side(s) of triangles to test against during raycasting. Use `TriangleRaycastSide::Front` for one-sided geometry,
         * or `TriangleRaycastSide::FrontAndBack` for double-sided geometry.
         */
        VE_INLINE void SetTriangleRaycastSide(TriangleRaycastSide triangleRaycastSide) {
            _triangleRaycastSide = triangleRaycastSide;
        }

        /** @brief Check if the collision shape is convex.
         * @returns Always false, since concave shapes are non-convex by definition.
         */
        [[nodiscard]] VE_INLINE virtual constexpr bool IsConvex() const override {
            return false;
        }

        /** @brief Check if the collision shape is polyhedral.
         * @returns Always true, since concave shapes are represented as a collection of polygons (triangles).
         */
        [[nodiscard]] VE_INLINE virtual constexpr bool IsPolyhedral() const override {
            return true;
        }

        /** @brief Get the local inertia tensor of the concave shape for a given mass.
         * @param mass The mass of the object.
         * @returns A uniform inertia tensor approximation using the given mass. Concave shapes do not have an analytically exact inertia tensor, so this
         * returns a conservative approximation.
         */
        [[nodiscard]] VE_INLINE virtual glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
            return glm::vec3(mass);
        }

        /** @brief Check if the concave shape contains a given point in local space.
         * @param point The point to test, in local space.
         * @returns Always false. Point-in-shape containment is not supported for concave shapes.
         */
        [[nodiscard]] VE_INLINE virtual constexpr bool ContainsPoint([[maybe_unused]] const glm::vec3 &point) const override {
            return false;
        }

        /** @brief Get the volume of the concave shape.
         * @returns The volume of the concave shape.
         */
        [[nodiscard]] VE_INLINE virtual f32 GetVolume() const override;

        /** @brief Compute all triangles of the concave shape that overlap the given local-space AABB. This is the primary mechanism for broadphase and
         * narrowphase collision queries against concave shapes — only the subset of triangles intersecting the query AABB need to be tested.
         * @param localAABB The axis-aligned bounding box in the shape's local space used to cull triangles.
         * @param triangleVertices Output list of triangle vertex positions (3 vertices per triangle, in order).
         * @param triangleVerticesNormals Output list of per-vertex normals corresponding to each vertex in `triangleVertices`.
         * @param shapeIds Output list of shape identifiers, one per triangle, used to identify individual sub-shapes or triangle indices.
         */
        virtual void ComputeOverlappingTriangles(const AABB &localAABB,
                                                 std::vector<glm::vec3> &triangleVertices,
                                                 std::vector<glm::vec3> &triangleVerticesNormals,
                                                 std::vector<u32> &shapeIds) const = 0;

    private:
        /** @brief The non-uniform scale applied to the concave shape in local space. */
        glm::vec3 _scale;

        /** @brief The side(s) of triangles to consider during ray-triangle intersection tests. */
        TriangleRaycastSide _triangleRaycastSide;
    };

} // namespace Vulkyrie
