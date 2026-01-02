#pragma once

#include "buffer_layout.h"
#include "core/graphics_api.h"

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

            /** @brief Creates a vertex buffer based on the specified graphics API and size.
             * @param api The graphics API to use.
             * @param size Size of the vertex buffer in bytes.
             * @returns A reference to the created VertexBuffer.
             */
            static Ref<VertexBuffer> Create(Vulkyrie::Core::GraphicsAPI api, size_t size);

            /** @brief Creates a vertex buffer based on the specified graphics API, vertices, and size.
             * @param api The graphics API to use.
             * @param vertices Pointer to the vertex data.
             * @param size Size of the vertex data in bytes.
             * @returns A reference to the created VertexBuffer.
             */
            static Ref<VertexBuffer> Create(Vulkyrie::Core::GraphicsAPI api, float *vertices, size_t size);
    };
} // namespace Vulkyrie::Renderer
