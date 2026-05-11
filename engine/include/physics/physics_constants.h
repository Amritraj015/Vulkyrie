#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    // Dynamic AABB tree parameters.
    constexpr i32 AABB_TREE_NULL_NODE = -1;
    constexpr f32 AABB_TREE_DEFAULT_INFLATION_PERCENTAGE = 0.04f;
    constexpr size_t AABB_TREE_DEFAULT_INITIAL_NODE_CAPACITY = 128;

    // Constraints for the narrow-phase collision detection algorithms.
    constexpr size_t MAX_CONTACT_POINTS_PER_PAIR_IN_NARROW_PHASE = 16;

} // namespace Vulkyrie
