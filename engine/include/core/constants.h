#pragma once

#include "vlkypch.h"
#include "core/types/static_string.h"

namespace Vulkyrie {

    inline constexpr StaticString VE_K_ENGINE_NAME = "Vulkyrie Engine";

    /**
     * @brief The difference between 1.0f and the next representable f32 value
     * greater than 1.0f.
     *
     * This represents the machine epsilon for f32 and is approximately
     * 1.1920929e-07. It describes the spacing between representable values
     * around 1.0f and is useful when reasoning about floating-point precision.
     */
    inline constexpr f32 VE_K_MACHINE_EPSILON = std::numeric_limits<f32>::epsilon();
    inline constexpr f32 VE_K_DECIMAL_MAX = std::numeric_limits<f32>::max();
    inline constexpr f32 VE_K_DECIMAL_MIN = std::numeric_limits<f32>::min();
    inline constexpr f32 VE_K_DECIMAL_LOWEST = std::numeric_limits<f32>::lowest();

} // namespace Vulkyrie
