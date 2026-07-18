#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /** @brief The smallest positive normal number such that `1.0f + VE_MACHINE_EPSILON != 1.0f`. This value is used as a threshold for floating-point
     * comparisons to determine if two values are effectively equal, accounting for the limitations of floating-point precision. It is defined as the difference
     * between 1.0f and the next representable float value greater than 1.0f, which is typically around 1.1920929e-07 for single-precision floats (f32). Using
     * this constant helps prevent issues with floating-point arithmetic where very small differences can lead to unexpected results due to rounding errors or
     * precision limitations. */
    constexpr f32 VE_MACHINE_EPSILON = std::numeric_limits<f32>::epsilon();

    constexpr f32 VE_DECIMAL_MAX = std::numeric_limits<f32>::max();
    constexpr f32 VE_DECIMAL_MIN = std::numeric_limits<f32>::min();

} // namespace Vulkyrie
