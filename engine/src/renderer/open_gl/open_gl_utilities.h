#pragma once

#include "glad/glad.h"
#include "renderer/texture_sampler_wrap_mode.h"
#include "renderer/texture_filter_mode.h"

namespace Vulkyrie {
    constexpr GLenum ToOpenGLSamplerWrapMode(TextureSamplerWrapMode mode) {
        switch (mode) {
            case TextureSamplerWrapMode::Repeat:
                return GL_REPEAT;
            case TextureSamplerWrapMode::MirroredRepeat:
                return GL_MIRRORED_REPEAT;
            case TextureSamplerWrapMode::ClampToEdge:
                return GL_CLAMP_TO_EDGE;
            case TextureSamplerWrapMode::ClampToBorder:
                return GL_CLAMP_TO_BORDER;
        }

        return GL_REPEAT;
    }

    constexpr GLenum ToOpenGLTextureFilterMode(TextureFilterMode mode) {
        switch (mode) {
            case TextureFilterMode::Nearest:
                return GL_NEAREST;
            case TextureFilterMode::Linear:
                return GL_LINEAR;
        }

        return GL_LINEAR;
    }
} // namespace Vulkyrie
