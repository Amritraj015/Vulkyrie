#pragma once

#include "renderer/vertex_buffer.h"

namespace Vulkyrie::Renderer {
    class OpenGLVertexBuffer final : public VertexBuffer {
        public:
            /** @brief Constructs an OpenGL vertex buffer with the given size.
             * @param size Size of the vertex buffer in bytes.
             */
            OpenGLVertexBuffer(uint32_t size);

            /** @brief Constructs an OpenGL vertex buffer with the given vertices and size.
             * @param vertices Pointer to the vertex data.
             * @param size Size of the vertex data in bytes.
             */
            OpenGLVertexBuffer(float *vertices, uint32_t size);

            /** @brief Destructor to clean up the OpenGL vertex buffer. */
            ~OpenGLVertexBuffer();

            /** @brief Binds the vertex buffer. */
            void Bind() const override;

            /** @brief Unbinds the vertex buffer. */
            void Unbind() const override;

            /** @brief Sets the data of the vertex buffer.
             * @param data Pointer to the data to set.
             * @param size Size of the data in bytes.
             */
            void SetData(const void *data, size_t size) override;

            /** @brief Gets the layout of the vertex buffer.
             * @returns The buffer layout.
             */
            const BufferLayout &GetLayout() const override;

            /** @brief Sets the layout of the vertex buffer.
             * @param layout The buffer layout to set.
             */
            void SetLayout(const BufferLayout &layout) override;
    };
} // namespace Vulkyrie::Renderer
