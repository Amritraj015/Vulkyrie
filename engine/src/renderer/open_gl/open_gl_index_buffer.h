#pragma once

#include "renderer/index_buffer.h"

namespace Vulkyrie::Renderer {
    class OpenGLIndexBuffer final : public IndexBuffer {
        public:
            OpenGLIndexBuffer(u32 *indices, u32 count);
            OpenGLIndexBuffer(u16 *indices, u32 count);
            OpenGLIndexBuffer(u8 *indices, u32 count);

            ~OpenGLIndexBuffer() override;

            u32 GetCount() const override;

            /** @brief Gets the OpenGL index buffer ID.
             * @returns The index buffer ID.
             */
            u32 GetIndexBufferID() const {
                return _eboID;
            }

        private:
            u32 _eboID;
            u32 _count;
    };
} // namespace Vulkyrie::Renderer
