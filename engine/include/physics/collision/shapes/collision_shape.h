#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    enum class CollisionShapeType : u32 { Sphere, Capsule, ConvexPolyhedron, Concave };
    static constexpr u8 CollisionShapeTypeCount = 4;

    enum class CollisionShapeName : u32 { Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield };

    class CollisionShape {
        public:
            CollisionShape(CollisionShapeType type, CollisionShapeName name)
                : _type(type)
                , _name(name) {
            }

            ~CollisionShape() = default;

            [[nodiscard]] VE_FORCE_INLINE CollisionShapeType GetType() const {
                return _type;
            }

            [[nodiscard]] VE_FORCE_INLINE CollisionShapeName GetName() const {
                return _name;
            }

        protected:
            CollisionShapeType _type;
            CollisionShapeName _name;
    };

} // namespace Vulkyrie
