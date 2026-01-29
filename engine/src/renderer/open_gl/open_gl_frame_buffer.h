#pragma once

#include "renderer/frame_buffer.h"

namespace Vulkyrie::Renderer {
    class OpenGLFrameBuffer : public FrameBuffer {
        public:
            OpenGLFrameBuffer();
            ~OpenGLFrameBuffer() override;

        private:
            u32 _fboId;
    };
} // namespace Vulkyrie::Renderer
