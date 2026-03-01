#pragma once

#include "buffer_layout.h"
#include "renderer/vertex.h"

namespace Vulkyrie::Renderer {
    class VertexBuffer {
        public:
            virtual ~VertexBuffer() = default;

            /** @brief Gets the layout of the vertex buffer.
             * @returns The buffer layout.
             */
            [[nodiscard]] inline const BufferLayout &GetLayout() const {
                return _layout;
            }

            /** @brief Sets the layout of the vertex buffer.
             * @param layout The buffer layout to set.
             */
            inline void SetLayout(const BufferLayout &layout) {
                _layout = layout;
            }

            /** @brief Sets the data of the vertex buffer.
             * @param data Pointer to the data to set.
             * @param size Size of the data in bytes.
             */
            virtual void SetData(const void *data, size_t size) = 0;

            /** @brief Creates a vertex buffer based on the specified size.
             * @param size Size of the vertex buffer in bytes.
             * @returns A reference to the created VertexBuffer.
             */
            static Ref<VertexBuffer> Create(size_t size);

            /** @brief Creates a vertex buffer based on the specified vertices and size.
             * @param vertices Pointer to the vertex data.
             * @param size Size of the vertex data in bytes.
             * @returns A reference to the created VertexBuffer.
             */
            static Ref<VertexBuffer> Create(f32 *vertices, size_t size);

            /** @brief Creates a vertex buffer based on a vector of vertices.
             * @param vertices The vector of vertices.
             * @returns A reference to the created VertexBuffer.
             */
            static Ref<VertexBuffer> Create(const std::vector<Vertex> &vertices);

        private:
            BufferLayout _layout;
    };
} // namespace Vulkyrie::Renderer
