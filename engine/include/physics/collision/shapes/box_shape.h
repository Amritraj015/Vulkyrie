#pragma once

#include "physics/collision/shapes/convex_polyhedron_shape.h"

namespace Vulkyrie {

    /** @brief The `BoxShape` class represents a box-shaped collision shape, which is a specific type of convex polyhedral shape defined by its half extents
     * along the x, y, and z axes. This class provides an efficient implementation for box-specific operations such as vertex position retrieval and face normal
     * calculation, taking advantage of the box's regular geometry to avoid unnecessary computations. It is designed to be used in collision detection and
     * physics simulations where a simple and computationally inexpensive convex shape is required. */
    class BoxShape final : public ConvexPolyhedronShape {
    public:
        /** @brief Construct a box shape with the specified half extents and margin.
         * @param halfExtents The half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that axis.
         * All components must be positive.
         * @param margin The margin to be applied to the box shape for collision detection purposes. This is an optional parameter that defaults to 0.0f if
         * not provided. A positive margin can help improve collision detection stability by providing a small buffer around the shape.
         */
        BoxShape(const glm::vec3 &halfExtents, f32 margin = 0.0f);

        /** @brief Destructor for the BoxShape class. */
        ~BoxShape() override = default;

        // Delete the copy constructor and the copy assignment operator.
        BoxShape(const BoxShape &) = delete;
        BoxShape &operator=(const BoxShape &) = delete;

        // Delete the move constructor and the move assignment operator.
        BoxShape(BoxShape &&) = delete;
        BoxShape &operator=(BoxShape &&) = delete;

        /** @brief Get the half extents of the box shape.
         * @returns The half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that axis.
         */
        [[nodiscard]] VE_INLINE const glm::vec3 &GetHalfExtents() const {
            return _halfExtents;
        }

        /** @brief Set the half extents of the box shape.
         * @param halfExtents The new half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that
         * axis. All components must be positive.
         */
        void SetHalfExtents(const glm::vec3 &halfExtents) {
            _halfExtents = halfExtents;

            // Notify colliders that the shape has changed so they can update their internal
            // state accordingly (e.g., recompute AABBs, update broadphase proxies).
            NotifyCollidersOfShapeChange();
        }

        /** @brief Get the number of faces of the box shape.
         * @returns The number of faces of the box shape, which is always 6 for a box.
         */
        [[nodiscard]] VE_INLINE constexpr u32 GetFacesCount() const override {
            return 6;
        }

        /** @brief Get the number of vertices of the box shape.
         * @returns The number of vertices of the box shape, which is always 8 for a box.
         */
        [[nodiscard]] VE_INLINE constexpr u32 GetVerticesCount() const override {
            return 8;
        }

        /** @brief Get the number of half edges of the box shape.
         * @returns The number of half edges of the box shape, which is always 24 for a box (each of the 12 edges has 2 half edges).
         */
        [[nodiscard]] VE_INLINE constexpr u32 GetHafEdgesCount() const override {
            return 24;
        }

        /** @brief Get the position of a specific vertex of the box shape.
         * @param vertexIndex The index of the vertex for which to retrieve the position. The valid range for this index is from 0 to 7, since a box has 8
         * vertices.
         * @returns The position of the specified vertex as a glm::vec3. The position is given in the local coordinate space of the shape, where the origin
         * is typically at the centroid of the shape. The vertices are ordered in a consistent manner, such as starting from one corner and proceeding in a
         * specific order around the box.
         */
        [[nodiscard]] VE_INLINE glm::vec3 GetVertexPosition(u32 vertexIndex) const override {
            VASSERT(vertexIndex < GetVerticesCount(), "Vertex index out of bounds for box shape.");

            switch (vertexIndex) {
                case 0:
                    return glm::vec3(-_halfExtents.x, -_halfExtents.y, _halfExtents.z);
                case 1:
                    return glm::vec3(_halfExtents.x, -_halfExtents.y, _halfExtents.z);
                case 2:
                    return glm::vec3(_halfExtents.x, _halfExtents.y, _halfExtents.z);
                case 3:
                    return glm::vec3(-_halfExtents.x, _halfExtents.y, _halfExtents.z);
                case 4:
                    return glm::vec3(-_halfExtents.x, -_halfExtents.y, -_halfExtents.z);
                case 5:
                    return glm::vec3(_halfExtents.x, -_halfExtents.y, -_halfExtents.z);
                case 6:
                    return glm::vec3(_halfExtents.x, _halfExtents.y, -_halfExtents.z);
                case 7:
                    return glm::vec3(-_halfExtents.x, _halfExtents.y, -_halfExtents.z);
            }

            VASSERT(false, "Invalid vertex index for box shape.");

            return glm::vec3(0.0f);
        }

        /** @brief Get the normal vector of a specific face of the box shape.
         * @param faceIndex The index of the face for which to retrieve the normal vector. The valid range for this index is from 0 to 5, since a box has 6
         * faces.
         * @returns The normal vector of the specified face as a glm::vec3. The normal vector is a unit vector that is perpendicular to the face and points
         * outward from the surface of the shape. The faces are ordered in a consistent manner, such as starting from one face and proceeding in a specific
         * order around the box.
         */
        [[nodiscard]] VE_INLINE glm::vec3 GetFaceNormal(u32 faceIndex) const override {
            VASSERT(faceIndex < GetFacesCount(), "Face index out of bounds for box shape.");

            switch (faceIndex) {
                case 0:
                    return glm::vec3(0.0f, 0.0f, 1.0f);
                case 1:
                    return glm::vec3(1.0f, 0.0f, 0.0f);
                case 2:
                    return glm::vec3(0.0f, 0.0f, -1.0f);
                case 3:
                    return glm::vec3(-1.0f, 0.0f, 0.0f);
                case 4:
                    return glm::vec3(0.0f, -1.0f, 0.0f);
                case 5:
                    return glm::vec3(0.0f, 1.0f, 0.0f);
            }

            VASSERT(false, "Invalid face index for box shape.");

            return glm::vec3(0.0f);
        }

        /** @brief Get the centroid of the box shape.
         * @returns The centroid of the box shape as a glm::vec3. For a box defined in local space with its center at the origin, the centroid is simply (0,
         * 0, 0).
         */
        [[nodiscard]] VE_INLINE glm::vec3 GetCentroid() const override {
            return glm::vec3(0.0f);
        }

        /** @brief Compute the local-space AABB of the box shape (i.e. before any transform is applied).
         * @returns An AABB centered at the origin with min corner at (-halfExtents) and max corner at (halfExtents), where halfExtents is the half extents
         * of the box shape. This represents the axis-aligned bounding box of the box shape in its local coordinate space.
         */
        [[nodiscard]] VE_INLINE AABB GetLocalAABB() const override {
            return AABB(-_halfExtents, _halfExtents);
        }

        /** @brief Compute the local-space inertia tensor diagonal for a solid box.
         * @param mass The mass of the body this shape belongs to.
         * @returns A vector whose components are given by the formula:
         * (1/3) * mass * (halfExtents.y² + halfExtents.z²) for the x component,
         * (1/3) * mass * (halfExtents.x² + halfExtents.z²) for the y component,
         * (1/3) * mass * (halfExtents.x² + halfExtents.y²) for the z component.
         * This gives a diagonal inertia tensor for a solid box, where halfExtents is the half extents of the box shape.
         */
        [[nodiscard]] VE_INLINE glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
            constexpr f32 oneThird = f32(1.0) / f32(3.0);
            const f32 factor = oneThird * mass;
            const f32 xSquare = _halfExtents.x * _halfExtents.x;
            const f32 ySquare = _halfExtents.y * _halfExtents.y;
            const f32 zSquare = _halfExtents.z * _halfExtents.z;

            return glm::vec3(factor * (ySquare + zSquare), factor * (xSquare + zSquare), factor * (xSquare + ySquare));
        }

        /** @brief Get the volume of the box shape.
         * @returns The volume of the box shape, calculated using the formula V = 8 * halfExtents.x * halfExtents.y * halfExtents.z, where halfExtents is
         * the half extents of the box shape. This formula accounts for the fact that the full extents of the box are twice the half extents along each
         * axis.
         */
        [[nodiscard]] VE_INLINE f32 GetVolume() const override {
            return 8.0f * _halfExtents.x * _halfExtents.y * _halfExtents.z;
        }

        /** @brief Check if a given point is contained within the box shape.
         * @param point The point to check for containment, specified as a glm::vec3 in the local coordinate space of the box shape. The local coordinate
         * space is defined such that the center of the box is at the origin (0, 0, 0).
         * @returns True if the point is contained within the box shape, false otherwise. A point is considered to be contained within the box if its
         * coordinates along each axis are between -halfExtents and halfExtents, where halfExtents is the half extents of the box shape.
         */
        [[nodiscard]] VE_INLINE bool ContainsPoint(const glm::vec3 &point) const override {
            return (point.x < _halfExtents.x && point.x > -_halfExtents.x && point.y < _halfExtents.y && point.y > -_halfExtents.y &&
                    point.z < _halfExtents.z && point.z > -_halfExtents.z);
        }

        /** @brief Compute the local support point of the box shape in a given direction, without considering the margin.
         * @param direction The direction in which to compute the support point, specified as a glm::vec3. This vector does not need to be normalized.
         * @returns The local support point of the box shape in the given direction, calculated by taking the sign of each component of the direction vector
         * and multiplying it by the corresponding half extent of the box shape. This gives the vertex of the box that is furthest in the specified
         * direction, without accounting for any margin that may be applied to the shape for collision detection purposes.
         */
        [[nodiscard]] VE_INLINE glm::vec3 GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const override {
            return glm::vec3(direction.x < f32(0.0) ? -_halfExtents.x : _halfExtents.x,
                             direction.y < f32(0.0) ? -_halfExtents.y : _halfExtents.y,
                             direction.z < f32(0.0) ? -_halfExtents.z : _halfExtents.z);
        }

    private:
        /** @brief The half extents of the box shape as a glm::vec3, where each component represents half the size of the box along that axis. All
         * components must be positive. */
        glm::vec3 _halfExtents;
    };

} // namespace Vulkyrie
