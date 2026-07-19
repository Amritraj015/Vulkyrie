#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    // Dynamic AABB tree parameters.
    constexpr u32 AABB_TREE_NULL_NODE = std::numeric_limits<u32>::max();
    constexpr f32 AABB_TREE_DEFAULT_INFLATION_PERCENTAGE = 0.04f;
    constexpr size_t AABB_TREE_DEFAULT_INITIAL_NODE_CAPACITY = 128;

    // Constraints for the narrow-phase collision detection algorithms.
    constexpr size_t MAX_CONTACT_POINTS_PER_PAIR_IN_NARROW_PHASE = 16;
    constexpr size_t MAX_CONTACT_POINTS_IN_POTENTIAL_MANIFOLD = 255;
    constexpr i32 MAX_CONTACT_POINTS_IN_MANIFOLD = 4;
    constexpr i32 MAX_CONTACT_MANIFOLDS = 4;
    constexpr u8 MAX_POTENTIAL_CONTACT_MANIFOLDS = 64;
    constexpr f32 SAME_CONTACT_POINT_DISTANCE_THRESHOLD = 0.01f;

    /** The type of the collision shape, used to categorize shapes into broad types for efficient processing. */
    enum class CollisionShapeType : u32 { Sphere, Capsule, ConvexPolyhedron, Concave };
    constexpr i32 SUPPORTED_COLLISION_SHAPE_TYPE_COUNT = 4;

    /** The specific name of the collision shape, used to identify the exact type of shape. */
    enum class CollisionShapeName : u32 { Triangle, Sphere, Capsule, Box, ConvexMesh, TriangleMesh, HeightField };

} // namespace Vulkyrie
