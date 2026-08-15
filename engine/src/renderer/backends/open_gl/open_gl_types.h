#pragma once

#include "vlkypch.h"
#include <glad/glad.h>

namespace Vulkyrie {

    constexpr u32 OPEN_GL_INVALID_INDEX = std::numeric_limits<u32>::max();

    struct OpenGLImage final {
        GLuint Texture = 0;
        GLenum Target = 0;
        GLenum InternalFormat = 0;
        u32 Width = 0;
        u32 Height = 0;
        u32 Depth = 0;
        u32 BindlessIndex = OPEN_GL_INVALID_INDEX;
        u16 Mips = 0;
        u16 Layers = 0;

        [[nodiscard]] VE_INLINE bool Valid() const noexcept {
            return 0 != Texture;
        }
    };

    struct OpenGLBuffer final {
        GLuint Buffer = 0;
        GLenum Target = 0;
        u64 Size = 0;
        void *Mapped = nullptr;
        u32 BindlessIndex = OPEN_GL_INVALID_INDEX;

        [[nodiscard]] VE_INLINE bool Valid() const noexcept {
            return 0 != Buffer;
        }
    };

    struct OpenGLSampler final {
        GLuint Sampler = 0;

        [[nodiscard]] VE_INLINE bool Valid() const noexcept {
            return 0 != Sampler;
        }
    };

    struct OpenGLPipeline final {
        // GLuint Program = 0;
        // GLuint VAO = 0;
        // bool IsComplete = false;
        //
        // [[nodiscard]] VE_INLINE bool Valid() const noexcept {
        //     return 0 != Program;
        // }
    };

    struct OpenGLShaderModule final {
        GLuint Shader = 0;

        [[nodiscard]] VE_INLINE bool Valid() const noexcept {
            return 0 != Shader;
        }
    };

    static_assert(std::is_trivially_copyable_v<OpenGLImage>);
    static_assert(std::is_trivially_copyable_v<OpenGLBuffer>);
    static_assert(std::is_trivially_copyable_v<OpenGLSampler>);
    static_assert(std::is_trivially_copyable_v<OpenGLPipeline>);
    static_assert(std::is_trivially_copyable_v<OpenGLShaderModule>);

} // namespace Vulkyrie
