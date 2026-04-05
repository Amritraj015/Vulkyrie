#pragma once

#include "vlkypch.h"

namespace Vulkyrie {
    /** @brief Texture wrapping modes for framebuffer attachments. */
    enum TextureSamplerWrapMode : u32 {
        /** @brief Repeat the texture when sampling outside the [0, 1] range. */
        Repeat,

        /** @brief Clamp the texture coordinates to the edge of the texture. */
        ClampToEdge,

        /** @brief Clamp the texture coordinates to the border color. */
        ClampToBorder,

        /** @brief Mirror the texture when sampling outside the [0, 1] range. */
        MirroredRepeat,
    };
} // namespace Vulkyrie
