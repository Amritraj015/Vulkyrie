#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    /** The type of the collision shape, used to categorize shapes into broad types for efficient processing. */
    enum class CollisionShapeType : u32 { Sphere, Capsule, ConvexPolyhedron, Concave };

    /** The total number of collision shape types defined in the `CollisionShapeType` enum. */
    static constexpr u8 COLLISION_SHAPE_TYPE_COUNT = 4;

    /** The specific name of the collision shape, used to identify the exact type of shape. */
    enum class CollisionShapeName : u32 { Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield };

    /** Base class for all collision shapes. */
    class CollisionShape {
        public:
            /** @brief Construct a collision shape with the specified type and name.
             * @param type The type of the collision shape (e.g., Sphere, Capsule, ConvexPolyhedron, Concave).
             * @param name The specific name of the collision shape (e.g., Triangle, Sphere
             */
            CollisionShape(CollisionShapeType type, CollisionShapeName name);

            /** Virtual destructor to allow proper cleanup of derived classes. */
            virtual ~CollisionShape() = default;

            // Delete the copy constructor and operator.
            CollisionShape(const CollisionShape &) = delete;
            CollisionShape &operator=(const CollisionShape &) = delete;

            // Delete the move constructor and operator.
            CollisionShape(CollisionShape &&) = delete;
            CollisionShape &operator=(CollisionShape &&) = delete;

            /** @brief Get the type of the collision shape.
             * @return The type of the collision shape (e.g., Sphere, Capsule, ConvexPolyhedron, Concave).
             */
            [[nodiscard]] VE_FORCE_INLINE CollisionShapeType GetType() const {
                return _type;
            }

            /** @brief Get the specific name of the collision shape.
             * @return The specific name of the collision shape (e.g., Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield).
             */
            [[nodiscard]] VE_FORCE_INLINE CollisionShapeName GetName() const {
                return _name;
            }

            /** @brief Check if the collision shape is convex.
             * @return True if the collision shape is convex, false otherwise.
             */
            virtual bool IsConvex() const = 0;

            /** @brief Check if the collision shape is polyhedral.
             * @return True if the collision shape is polyhedral, false otherwise.
             */
            virtual bool IsPolyhedral() const = 0;

            /** @brief Get the axis-aligned bounding box (AABB) of the collision shape in local space.
             * @return The AABB of the collision shape defined in its local coordinate system.
             */
            virtual AABB GetLocalAABB() const = 0;

            /** @brief Get the local inertia tensor of the collision shape for a given mass.
             * @param mass The mass of the object for which to compute the inertia tensor.
             * @return The local inertia tensor of the collision shape as a 3D vector, where each component represents the inertia around the corresponding
             * axis.
             */
            virtual glm::vec3 GetLocalInertiaTensor(f32 mass) const = 0;

            /** @brief Get the volume of the collision shape.
             * @return The volume of the collision shape.
             */
            virtual f32 GetVolume() const = 0;

            /** @brief Check if the collision shape contains a given point in local space.
             * @param point The point to check, specified in the local coordinate system of the collision shape.
             * @return True if the point is inside the collision shape, false otherwise.
             */
            virtual bool ContainsPoint(const glm::vec3 &point) const = 0;

            /** @brief Compute the axis-aligned bounding box (AABB) of the collision shape after applying the given transformation.
             * @param transform The transformation to apply to the collision shape.
             * @return The computed AABB that encompasses the transformed collision shape.
             */
            virtual AABB ComputeTransformedAABB(const TransformComponent &transform) const;

        protected:
            /** The type of the collision shape (e.g., Sphere, Capsule, ConvexPolyhedron, Concave). */
            const CollisionShapeType _type;

            /** The specific name of the collision shape (e.g., Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield). */
            const CollisionShapeName _name;
    };

} // namespace Vulkyrie
