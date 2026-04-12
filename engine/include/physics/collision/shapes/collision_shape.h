#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/components/transform_component_store.h"

namespace Vulkyrie {

    enum class CollisionShapeType : u32 { Sphere, Capsule, ConvexPolyhedron, Concave };
    static constexpr u8 COLLISION_SHAPE_TYPE_COUNT = 4;

    enum class CollisionShapeName : u32 { Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield };

    class CollisionShape {
        public:
            CollisionShape(CollisionShapeType type, CollisionShapeName name);
            virtual ~CollisionShape() = default;

            CollisionShape(const CollisionShape &) = delete;
            CollisionShape &operator=(const CollisionShape &) = delete;

            CollisionShape(CollisionShape &&) = delete;
            CollisionShape &operator=(CollisionShape &&) = delete;

            [[nodiscard]] VE_FORCE_INLINE CollisionShapeType GetType() const {
                return _type;
            }

            [[nodiscard]] VE_FORCE_INLINE CollisionShapeName GetName() const {
                return _name;
            }

            virtual bool IsConvex() const = 0;
            virtual bool IsPolyhedral() const = 0;
            virtual AABB GetLocalAABB() const = 0;
            virtual glm::vec3 GetLocalInertiaTensor(f32 mass) const = 0;
            virtual f32 GetVolume() const = 0;
            virtual AABB ComputeTransformedAABB(const TransformComponent &transform) const = 0;
            virtual bool ContainsPoint(const glm::vec3 &point) const = 0;

        protected:
            CollisionShapeType _type;
            CollisionShapeName _name;
    };

} // namespace Vulkyrie
