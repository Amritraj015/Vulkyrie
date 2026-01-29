#include "renderer/open_gl/open_gl_frame_buffer.h"
// #include "core/logger.h"
#include <glad/glad.h>

namespace Vulkyrie::Renderer {
    OpenGLFrameBuffer::OpenGLFrameBuffer() {
        glCreateFramebuffers(1, &_fboId);

        // u32 colorTex, width, height;
        // glCreateTextures(GL_TEXTURE_2D, 1, &colorTex);
        //
        // // Allocate immutable storage
        // glTextureStorage2D(colorTex, 1, GL_RGBA8, width, height);
        //
        // // Set filtering (important for post-processing)
        // glTextureParameteri(colorTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        // glTextureParameteri(colorTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        //
        // // Optional but recommended
        // glTextureParameteri(colorTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        // glTextureParameteri(colorTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        //
        // glNamedFramebufferTexture(_fboId, GL_COLOR_ATTACHMENT0, colorTex, 0);
    }

    OpenGLFrameBuffer::~OpenGLFrameBuffer() {
        glDeleteFramebuffers(1, &_fboId);
    }
} // namespace Vulkyrie::Renderer
