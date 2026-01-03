#pragma once

#include "renderer/index_buffer.h"

namespace Vulkyrie::Renderer {
    class OpenGLIndexBuffer final : public IndexBuffer {
        public:
            OpenGLIndexBuffer(u32 *indices, u32 count);
            OpenGLIndexBuffer(u16 *indices, u32 count);
            OpenGLIndexBuffer(u8 *indices, u32 count);

            virtual ~OpenGLIndexBuffer();

            void Bind() const override;
            void Unbind() const override;
            u32 GetCount() const override;

        private:
            u32 _eboId;
            u32 _count;
    };
} // namespace Vulkyrie::Renderer
