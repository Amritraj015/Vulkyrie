#pragma once

#include "renderer/rhi/pipeline_types.h"
#include "renderer/rhi/rhi_types.h"
#include <glad/glad.h>
// #include "renderer/texture_sampler_wrap_mode.h"
// #include "renderer/texture_filter_mode.h"

namespace Vulkyrie {

    [[nodiscard]] VE_INLINE GLenum ToGLInternalFormat(Format format) noexcept;
    [[nodiscard]] VE_INLINE GLenum ToGLBaseFormat(Format format) noexcept;
    [[nodiscard]] VE_INLINE GLenum ToGLType(Format format) noexcept;
    [[nodiscard]] VE_INLINE GLenum ToGLCompareOp(CompareOp compareOp) noexcept;
    [[nodiscard]] VE_INLINE GLenum ToGLBlendFactor(BlendFactor blend) noexcept;
    [[nodiscard]] VE_INLINE GLenum ToGLBlendOp(BlendOp blendOp) noexcept;
    [[nodiscard]] VE_INLINE GLenum ToGLTopology(PrimitiveTopology topology) noexcept;

    // The whole barrier story on this tier: state -> glMemoryBarrier bit.
    [[nodiscard]] VE_INLINE GLbitfield ToGLMemoryBarrierBit(ResourceState state) noexcept;

    // constexpr GLenum ToOpenGLSamplerWrapMode(TextureSamplerWrapMode mode) {
    //     switch (mode) {
    //         case TextureSamplerWrapMode::Repeat:
    //             return GL_REPEAT;
    //         case TextureSamplerWrapMode::MirroredRepeat:
    //             return GL_MIRRORED_REPEAT;
    //         case TextureSamplerWrapMode::ClampToEdge:
    //             return GL_CLAMP_TO_EDGE;
    //         case TextureSamplerWrapMode::ClampToBorder:
    //             return GL_CLAMP_TO_BORDER;
    //     }
    //
    //     return GL_REPEAT;
    // }
    //
    // constexpr GLenum ToOpenGLTextureFilterMode(TextureFilterMode mode) {
    //     switch (mode) {
    //         case TextureFilterMode::Nearest:
    //             return GL_NEAREST;
    //         case TextureFilterMode::Linear:
    //             return GL_LINEAR;
    //     }
    //
    //     return GL_LINEAR;
    // }

} // namespace Vulkyrie
