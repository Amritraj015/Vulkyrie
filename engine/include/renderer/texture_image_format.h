#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {
    /** @brief Supported image formats for textures. */
    enum class TextureImageFormat : u8 {
        None = 0,
        R8,
        RGB8,
        RGBA8,
        RGBA32F,
    };
} // namespace Vulkyrie::Renderer
