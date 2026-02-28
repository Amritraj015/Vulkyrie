#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    /** @brief Texture filtering modes for framebuffer attachments. */
    enum TextureFilterMode : u32 {
        /** @brief Nearest neighbor filtering. */
        Nearest,

        /** @brief Linear filtering. */
        Linear,
    };
} // namespace Vulkyrie::Renderer
