#pragma once

#include "renderer/index_buffer.h"

namespace Vulkyrie {

    class OpenGLIndexBuffer final : public IndexBuffer {
        public:
            OpenGLIndexBuffer(u32 *indices, size_t count);
            OpenGLIndexBuffer(u16 *indices, size_t count);
            OpenGLIndexBuffer(u8 *indices, size_t count);

            ~OpenGLIndexBuffer() override;

            size_t GetCount() const override;

            /** @brief Gets the OpenGL index buffer ID.
             * @returns The index buffer ID.
             */
            u32 GetIndexBufferID() const {
                return _eboID;
            }

        private:
            u32 _eboID;
            size_t _count;
    };

} // namespace Vulkyrie
