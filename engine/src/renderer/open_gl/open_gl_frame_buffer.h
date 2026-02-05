#pragma once

#include "renderer/frame_buffer.h"

namespace Vulkyrie::Renderer {

    struct GLAttachment {
        public:
            u32 ResourceID = 0;
            AttachmentType Type = AttachmentType::Texture;
    };

    class OpenGLFrameBuffer final : public FrameBuffer {
        public:
            OpenGLFrameBuffer(const FrameBufferSpecification &specification);
            ~OpenGLFrameBuffer() override;

            void Resize(u32 width, u32 height) override;
            void Bind() const override;
            void Unbind() const override;
            u32 GetColorAttachmentResourceID(u32 index = 0) const override;

        private:
            void Create();
            void Destroy();
            u32 _fboId;

            std::vector<GLAttachment> _colorAttachments;
            GLAttachment _depthAttachment;
    };
} // namespace Vulkyrie::Renderer
