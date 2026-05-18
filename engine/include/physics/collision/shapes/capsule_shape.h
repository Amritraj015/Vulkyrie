#pragma once

#include "physics/collision/shapes/convex_shape.h"

namespace Vulkyrie {

    /** @brief The `CapsuleShape` class represents a capsule-shaped collision shape, which consists of a cylindrical part with hemispherical end caps. It is
     * defined by its radius (the radius of the hemispherical end caps and the cylindrical part) and its height (the distance between the centers of the two
     * hemispherical end caps). */
    class CapsuleShape final : public ConvexShape {
    public:
        /** @brief Construct a capsule shape with the specified radius and height.
         * @param radius The radius of the capsule shape, which is the radius of the hemispherical end caps and the cylindrical part. Must be a positive
         * value.
         * @param height The height of the capsule shape, which is the distance between the centers of the two hemispherical end caps. Must be a positive
         * value. */
        CapsuleShape(f32 radius, f32 height);

        // Delete the copy constructor and the copy assignment operator to prevent copying of CapsuleShape instances.
        CapsuleShape(const CapsuleShape &) = delete;
        CapsuleShape &operator=(const CapsuleShape &) = delete;

        // Delete the move constructor and the move assignment operator to prevent moving of CapsuleShape instances.
        CapsuleShape(CapsuleShape &&) = delete;
        CapsuleShape &operator=(CapsuleShape &&) = delete;

        /** @brief Destructor for the CapsuleShape class. */
        ~CapsuleShape() override = default;

        /** @brief Get the radius of the capsule shape.
         * @returns The radius of the capsule shape, which is the radius of the hemispherical end caps and the cylindrical part. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetRadius() const {
            return _margin;
        }

        /** @brief Set the radius of the capsule shape.
         * @param radius The new radius of the capsule shape. Must be a positive value. */
        VE_FORCE_INLINE void SetRadius(f32 radius) {
            VASSERT(radius > 0.0f, "Radius must be positive for capsule shape.");

            // Set the new radius for the capsule shape.
            _margin = radius;

            // Notify colliders that the shape has changed so they can update their internal
            // state accordingly (e.g., recompute AABBs, update broadphase proxies).
            NotifyCollidersOfShapeChange();
        }

        /** @brief Get the height of the capsule shape.
         * @returns The height of the capsule shape, which is the distance between the centers of the two hemispherical end caps. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetHeight() const {
            return _halfHeight + _halfHeight;
        }

        /** @brief Set the height of the capsule shape.
         * @param height The new height of the capsule shape, which is the distance between the centers of the two hemispherical end caps. Must be a
         * positive value. */
        VE_FORCE_INLINE void SetHeight(f32 height) {
            VASSERT(height > 0.0f, "Height must be positive for capsule shape.");

            // Set the half-height of the capsule.
            _halfHeight = height * 0.5f;

            // Notify colliders that the shape has changed so they can update their internal
            // state accordingly (e.g., recompute AABBs, update broadphase proxies).
            NotifyCollidersOfShapeChange();
        }

        /** @brief Check if the collision shape is polyhedral.
         * @returns True if the collision shape is polyhedral, false otherwise.
         */
        [[nodiscard]] VE_FORCE_INLINE bool IsPolyhedral() const override {
            return false;
        }

        /** @brief Get the axis-aligned bounding box (AABB) of the collision shape in local space.
         * @returns The AABB of the collision shape defined in its local coordinate system.
         */
        [[nodiscard]] VE_FORCE_INLINE AABB GetLocalAABB() const override {
            return AABB(glm::vec3(-_margin, -_halfHeight - _margin, -_margin), glm::vec3(_margin, _halfHeight + _margin, _margin));
        }

        /** @brief Get the local inertia tensor of the collision shape for a given mass.
         * @param mass The mass of the object for which to compute the inertia tensor.
         * @returns The local inertia tensor of the collision shape as a 3D vector, where each component represents the inertia around the corresponding
         * axis. */
        glm::vec3 GetLocalInertiaTensor(f32 mass) const override;

        /** @brief Get the volume of the collision shape.
         * @returns The volume of the collision shape. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetVolume() const override {
            // The volume of a capsule can be calculated as the volume of the cylindrical part plus the volume of the two hemispherical end caps. The
            // formula is: V = π * r² * h + (4/3) * π * r³, where r is the radius and h is the height of the cylindrical part (which is the total height
            // minus the diameter of the end caps).
            constexpr f32 fourThirds = 4.0f / 3.0f;
            return glm::pi<f32>() * _margin * _margin * (fourThirds * _margin + f32(2.0f) * _halfHeight);
        }

        /** @brief Check if the collision shape contains a given point in local space.
         * @param point The point to check, specified in the local coordinate system of the collision shape.
         * @returns True if the point is inside the collision shape, false otherwise. */
        bool ContainsPoint(const glm::vec3 &point) const override;

        /** @brief Returns the local support point on the capsule's core (without margin) in the given direction.
         * For a capsule aligned with the y-axis, this function returns the endpoint of the central spine (excluding the hemispherical radius)
         * that is farthest in the specified direction. This is used in collision detection algorithms (e.g., GJK) to find the extreme point
         * of the capsule's core along a direction, ignoring the margin/radius.
         * @param direction The direction vector in which to search for the support point (does not need to be normalized).
         * @returns The local-space position of the support point on the capsule's spine, without margin. */
        [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const override {
            // Support point on the top hemisphere.
            const f32 dotProductTop = _halfHeight * direction.y;

            // Support point on the bottom hemisphere.
            const f32 dotProductBottom = -_halfHeight * direction.y;

            // Determine which hemisphere provides the support point in the given direction.
            if (dotProductTop > dotProductBottom) {
                // Support point on the top hemisphere.
                return glm::vec3(0.0f, _halfHeight, 0.0f);
            } else {
                // Support point on the bottom hemisphere.
                return glm::vec3(0.0f, -_halfHeight, 0.0f);
            }
        }

    private:
        f32 _halfHeight;
    };

} // namespace Vulkyrie
