#pragma once

namespace Vulkyrie::Physics {

    enum class CollisionShapeType : u32 {
        Sphere,
        Capsule,
        ConvexPolyhedron,
        Concave,
    };

    enum class CollisionShapeName : u32 {
        Triangle,
        Sphere,
        Capsule,
        Box,
        ConvexMesh,
        TriangleMesh,
        Heightfield,
    };

    class CollisionShape {
        public:
            CollisionShape(CollisionShapeType type, CollisionShapeName name)
                : _type(type)
                , _name(name) {
            }

            [[nodiscard]] inline CollisionShapeType GetType() const {
                return _type;
            }

            [[nodiscard]] inline CollisionShapeName GetName() const {
                return _name;
            }

        private:
            CollisionShapeType _type;
            CollisionShapeName _name;
    };

} // namespace Vulkyrie::Physics
