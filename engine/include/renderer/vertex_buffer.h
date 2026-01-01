#pragma once

#include "buffer_layout.h"

namespace Vulkyrie::Renderer {
    class VertexBuffer {
        public:
            virtual ~VertexBuffer() = default;

            /** @brief Binds the vertex buffer. */
            virtual void Bind() const = 0;

            /** @brief Unbinds the vertex buffer. */
            virtual void Unbind() const = 0;

            /** @brief Sets the data of the vertex buffer.
             * @param data Pointer to the data to set.
             * @param size Size of the data in bytes.
             */
            virtual void SetData(const void *data, size_t size) = 0;

            /** @brief Gets the layout of the vertex buffer.
             * @returns The buffer layout.
             */
            virtual const BufferLayout &GetLayout() const = 0;

            /** @brief Sets the layout of the vertex buffer.
             * @param layout The buffer layout to set.
             */
            virtual void SetLayout(const BufferLayout &layout) = 0;
    };
} // namespace Vulkyrie::Renderer
