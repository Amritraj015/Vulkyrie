#pragma once

#include "vlkypch.h"
#include "physics/physics_constants.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    class Collider;

    /** Base class for all collision shapes. */
    class CollisionShape {
    public:
        /** @brief Construct a collision shape with the specified type and name.
         * @param type The type of the collision shape (e.g., Sphere, Capsule, ConvexPolyhedron, Concave).
         * @param name The specific name of the collision shape (e.g., Triangle, Sphere
         * @param id The unique identifier of the collision shape in the overlapping pair.
         */
        explicit CollisionShape(CollisionShapeType type, CollisionShapeName name, u32 id = 0);

        // Delete the copy constructor and operator.
        CollisionShape(const CollisionShape &) = delete;
        CollisionShape &operator=(const CollisionShape &) = delete;

        // Delete the move constructor and operator.
        CollisionShape(CollisionShape &&) = delete;
        CollisionShape &operator=(CollisionShape &&) = delete;

        /** Virtual destructor to allow proper cleanup of derived classes. */
        virtual ~CollisionShape() = default;

        /** @brief Get the type of the collision shape.
         * @returns The type of the collision shape (e.g., Sphere, Capsule, ConvexPolyhedron, Concave).
         */
        [[nodiscard]] VE_INLINE CollisionShapeType GetType() const {
            return _type;
        }

        /** @brief Get the specific name of the collision shape.
         * @returns The specific name of the collision shape (e.g., Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield).
         */
        [[nodiscard]] VE_INLINE CollisionShapeName GetName() const {
            return _name;
        }

        /** @brief Get the unique identifier of the collision shape in the overlapping pair.
         * @returns The unique identifier of the collision shape in the overlapping pair.
         */
        [[nodiscard]] VE_INLINE u32 GetID() const {
            return _id;
        }

        /** @brief Add a collider to the list of colliders using this collision shape.
         * @param collider The collider to be added to the list of colliders using this collision shape. */
        VE_INLINE void AddCollider(Collider &collider) {
            _colliders.push_back(&collider);
        }

        /** @brief Remove a collider from the list of colliders using this collision shape.
         * @param collider The collider to be removed from the list of colliders using this collision shape. */
        VE_INLINE void RemoveCollider(Collider &collider) {
            std::erase(_colliders, &collider);
        }

        /** @brief Notify all colliders that are using this collision shape that the shape has changed. This is important for ensuring that any changes to
         * the collision shape (e.g., resizing, changing properties) are properly communicated to the colliders that rely on it for collision detection and
         * response, allowing them to update their internal state accordingly. */
        void NotifyCollidersOfShapeChange() const;

        /** @brief Check if the collision shape is convex.
         * @returns True if the collision shape is convex, false otherwise.
         */
        virtual bool IsConvex() const = 0;

        /** @brief Check if the collision shape is polyhedral.
         * @returns True if the collision shape is polyhedral, false otherwise.
         */
        virtual bool IsPolyhedral() const = 0;

        /** @brief Get the axis-aligned bounding box (AABB) of the collision shape in local space.
         * @returns The AABB of the collision shape defined in its local coordinate system.
         */
        virtual AABB GetLocalAABB() const = 0;

        /** @brief Get the local inertia tensor of the collision shape for a given mass.
         * @param mass The mass of the object for which to compute the inertia tensor.
         * @returns The local inertia tensor of the collision shape as a 3D vector, where each component represents the inertia around the corresponding
         * axis.
         */
        virtual glm::vec3 GetLocalInertiaTensor(f32 mass) const = 0;

        /** @brief Get the volume of the collision shape.
         * @returns The volume of the collision shape.
         */
        virtual f32 GetVolume() const = 0;

        /** @brief Check if the collision shape contains a given point in local space.
         * @param point The point to check, specified in the local coordinate system of the collision shape.
         * @returns True if the point is inside the collision shape, false otherwise.
         */
        virtual bool ContainsPoint(const glm::vec3 &point) const = 0;

        /** @brief Compute the axis-aligned bounding box (AABB) of the collision shape after applying the given transformation.
         * @param transform The transformation to apply to the collision shape.
         * @returns The computed AABB that encompasses the transformed collision shape.
         */
        virtual AABB ComputeTransformedAABB(const TransformComponent &transform) const;

    protected:
        /** @brief The type of the collision shape (e.g., Sphere, Capsule, ConvexPolyhedron, Concave). */
        const CollisionShapeType _type;

        /** @brief The specific name of the collision shape (e.g., Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield). */
        const CollisionShapeName _name;

        /** @brief The unique identifier of the collision shape in the overlapping pair. */
        const u32 _id;

        /** @brief Colliders that are using this collision shape. This allows the shape to notify its colliders of any changes that may affect collision
         * detection or response. */
        std::vector<Collider *> _colliders;
    };

} // namespace Vulkyrie
