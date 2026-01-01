#pragma once

#include "renderer/index_buffer.h"

namespace Vulkyrie::Renderer {
    class OpenGLIndexBuffer final : public IndexBuffer {
        public:
            OpenGLIndexBuffer(uint32_t *indices, uint32_t count);
            virtual ~OpenGLIndexBuffer();

            void Bind() const override;
            void Unbind() const override;
            uint32_t GetCount() const override;

        private:
            uint32_t _eboId;
            uint32_t _count;
    };
} // namespace Vulkyrie::Renderer
