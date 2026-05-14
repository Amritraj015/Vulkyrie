#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    // General physics constants.
    constexpr f32 VE_MACHINE_EPSILON = std::numeric_limits<f32>::epsilon();

    // Dynamic AABB tree parameters.
    constexpr i32 AABB_TREE_NULL_NODE = -1;
    constexpr f32 AABB_TREE_DEFAULT_INFLATION_PERCENTAGE = 0.04f;
    constexpr size_t AABB_TREE_DEFAULT_INITIAL_NODE_CAPACITY = 128;

    // Constraints for the narrow-phase collision detection algorithms.
    constexpr size_t MAX_CONTACT_POINTS_PER_PAIR_IN_NARROW_PHASE = 16;

    /** The type of the collision shape, used to categorize shapes into broad types for efficient processing. */
    enum class CollisionShapeType : u32 { Sphere, Capsule, ConvexPolyhedron, Concave };
    constexpr i32 SUPPORTED_COLLISION_SHAPE_TYPE_COUNT = 4;

    /** The specific name of the collision shape, used to identify the exact type of shape. */
    enum class CollisionShapeName : u32 { Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, Heightfield };

} // namespace Vulkyrie
