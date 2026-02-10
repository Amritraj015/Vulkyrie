#pragma once

#include "renderer/texture_image_format.h"
#include "renderer/texture_sampler_wrap_mode.h"
#include "renderer/texture_filter_mode.h"

namespace Vulkyrie::Renderer {
    /** @brief Specification for creating a texture. */
    struct TextureSpecification {
        public:
            /** @brief Height of the texture in pixels. Default value is 1px. */
            u32 Height = 1;

            /** @brief Width of the texture in pixels. Default value is 1px. */
            u32 Width = 1;

            /** @brief Wrapping mode for the S (horizontal) texture coordinate. Default is ClampToEdge. */
            TextureSamplerWrapMode WrapS = TextureSamplerWrapMode::ClampToEdge;

            /** @brief Wrapping mode for the T (vertical) texture coordinate. Default is ClampToEdge. */
            TextureSamplerWrapMode WrapT = TextureSamplerWrapMode::ClampToEdge;

            /** @brief Minification filter for the texture. Default is Linear. */
            TextureFilterMode MinFilter = TextureFilterMode::Linear;

            /** @brief Magnification filter for the texture. Default is Linear. */
            TextureFilterMode MagFilter = TextureFilterMode::Linear;

            /** @brief Number of samples for multi-sampling (default is 1, meaning no multi-sampling). */
            u32 Samples = 1;

            /** @brief Format of the texture image. Default is RGBA8. */
            TextureImageFormat Format = TextureImageFormat::RGBA8;

            /** @brief Whether to generate mipmaps for the texture. Default is true. */
            bool GenerateMips = true;
    };
} // namespace Vulkyrie::Renderer
