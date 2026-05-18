#pragma once

#include "physics/collision/shapes/convex_shape.h"

namespace Vulkyrie {

    /** @brief The `SphereShape` class represents a spherical collision shape, which is a type of convex shape defined by a single radius centered at the local
     * origin. This class provides an efficient implementation for sphere-specific operations such as AABB computation and inertia tensor calculation, taking
     * advantage of the sphere's rotational symmetry to avoid unnecessary matrix operations. It is designed to be used in collision detection and physics
     * simulations where a simple and computationally inexpensive convex shape is required. */
    class SphereShape final : public ConvexShape {
    public:
        /** @brief Construct a sphere collision shape.
         * @param radius The radius of the sphere. Must be positive.
         * @param margin Optional collision margin that expands the effective surface of the shape for broadphase and GJK/EPA stability. Defaults to 0. */
        SphereShape(f32 radius, f32 margin = 0.0f);

        // Delete the copy constructor and the copy assignment operator.
        SphereShape(const SphereShape &) = delete;
        SphereShape &operator=(const SphereShape &) = delete;

        // Delete the move constructor and the move assignment operator.
        SphereShape(SphereShape &&) = delete;
        SphereShape &operator=(SphereShape &&) = delete;

        ~SphereShape() override = default;

        /** @brief Get the radius of the sphere.
         * @returns The radius of the sphere. */
        [[nodiscard]] VE_FORCE_INLINE f32 GetRadius() const {
            return _margin;
        }

        /** @brief Set the radius of the sphere.
         * @param radius The new radius. Must be positive. */
        void SetRadius(f32 radius) {
            VASSERT(radius > 0.0f, "Radius must be positive for sphere shape.");

            // Set the new radius for the sphere shape.
            _margin = radius;

            // Notify colliders that the shape has changed so they can update their
            // internal state accordingly (e.g., recompute AABBs, update broadphase proxies).
            NotifyCollidersOfShapeChange();
        }

        /** @brief A sphere is not a polyhedral shape.
         * @returns Always false. */
        [[nodiscard]] VE_FORCE_INLINE constexpr bool IsPolyhedral() const override {
            return false;
        }

        /** @brief Compute the local-space AABB of the sphere (i.e. before any transform is applied).
         * @returns An AABB centered at the origin with half-extents equal to the radius along every axis. */
        [[nodiscard]] VE_FORCE_INLINE AABB GetLocalAABB() const override {
            return AABB(glm::vec3(-_margin), glm::vec3(_margin));
        }

        /** @brief Compute the local-space inertia tensor diagonal for a solid sphere.
         * @param mass The mass of the body this shape belongs to.
         * @returns A vector whose three equal components are `(2/5) * mass * radius²`, giving a uniform diagonal inertia tensor. */
        [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLocalInertiaTensor(f32 mass) const override {
            f32 diag = f32(0.4) * mass * _margin * _margin;

            return glm::vec3(diag, diag, diag);
        }

        /** @brief Get the volume of the sphere shape.
         * @returns The volume of the sphere shape, calculated using the formula V = (4/3) * π * r^3, where r is the radius of the sphere.
         */
        [[nodiscard]] VE_FORCE_INLINE f32 GetVolume() const override {
            return f32(4.0) / f32(3.0) * static_cast<f32>(std::numbers::pi) * _margin * _margin * _margin;
        }

        /** @brief Check if a given point is contained within the sphere shape.
         * @param point The point to check for containment, specified as a glm::vec3 in the local coordinate space of the sphere shape. The local coordinate
         * space is defined such that the center of the sphere is at the origin (0, 0, 0).
         * @returns True if the point is contained within the sphere shape, false otherwise. A point is considered to be contained within the sphere if
         * its distance from the center of the sphere (the origin) is less than the radius of the sphere.
         */
        [[nodiscard]] VE_FORCE_INLINE bool ContainsPoint(const glm::vec3 &point) const override {
            return (glm::length2(point) < _margin * _margin);
        }

        /** @brief Compute the axis-aligned bounding box (AABB) of the sphere shape after applying the given transformation.
         * @param transform The transformation to apply to the sphere shape, which includes translation, rotation, and scaling. The AABB will be computed
         * based on the transformed position and size of the sphere.
         * @returns The AABB of the transformed sphere shape, defined by its minimum and maximum coordinates in world space. Since a sphere is rotationally
         * symmetric, the AABB will always be a cube that encompasses the entire sphere regardless of its orientation. The effective radius used for the
         * AABB calculation includes the collision margin to ensure that the broadphase collision detection does not miss contacts when objects are close to
         * each other.
         */
        [[nodiscard]] VE_FORCE_INLINE AABB ComputeTransformedAABB(const TransformComponent &transform) const override {
            // A sphere is rotationally symmetric, so its AABB is always a cube regardless of orientation.
            // The effective radius includes the collision margin so the broadphase doesn't miss contacts.
            const glm::vec3 halfExtents(_margin + _margin);

            // The local center is at the origin, so rotation has no effect — the world center is just the translation.
            return AABB(transform.Position - halfExtents, transform.Position + halfExtents);
        }

        /** @brief Get the local support point on the sphere shape in the given direction, without applying the margin. The support point is the point on
         * the shape that is farthest in the specified direction, and it is used in collision detection algorithms like GJK to determine if two shapes are
         * intersecting. For a sphere, the support point in any direction is simply the center of the sphere (the origin) plus the radius in the direction
         * of the input vector. Since this function is supposed to return the support point without margin, we will return the center of the sphere, which
         * is (0, 0, 0) in local space.
         * @param direction The direction in which to calculate the support point, represented as a glm::vec3. The direction vector does not need to be
         * normalized.
         * @returns The local support point on the sphere shape in the given direction, without applying the margin. This is the point on the shape that is
         * farthest in the specified direction, and it is used for collision detection purposes. For a sphere, this will always be (0, 0, 0) in local space,
         * since we are not applying any margin and the sphere's center is at the origin. */
        [[nodiscard]] VE_FORCE_INLINE glm::vec3 GetLocalSupportPointWithoutMargin([[maybe_unused]] const glm::vec3 &direction) const override {
            // For a sphere, the support point in any direction is simply the center of the sphere (the origin) plus the radius in the direction of the
            // input vector. Since this function is supposed to return the support point without margin, we will return the center of the sphere, which is
            // (0, 0, 0) in local space.
            return glm::vec3(0.0f);
        }
    };

} // namespace Vulkyrie
