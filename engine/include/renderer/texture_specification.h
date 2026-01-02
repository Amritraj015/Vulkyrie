#pragma once

#include "renderer/texture_image_format.h"

namespace Vulkyrie::Renderer {
    /** @brief Specification for creating a texture. */
    struct TextureSpecification {
        public:
            /** @brief Height of the texture in pixels. Default value is 1px. */
            u32 Height = 1;

            /** @brief Width of the texture in pixels. Default value is 1px. */
            u32 Width = 1;

            /** @brief Format of the texture image. Default is RGBA8. */
            TextureImageFormat Format = TextureImageFormat::RGBA8;

            /** @brief Whether to generate mipmaps for the texture. Default is true. */
            bool GenerateMips = true;
    };
} // namespace Vulkyrie::Renderer
