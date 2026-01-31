#pragma once

#include "vlkypch.h"
#include "renderer/index_buffer.h"
#include "renderer/vertex_buffer.h"

namespace Vulkyrie::Renderer {
    class VertexArray {
        public:
            /** @brief Virtual destructor for the VertexArray class. */
            virtual ~VertexArray() = default;

            /** @brief Binds the vertex array. */
            virtual void Bind() const = 0;

            /** @brief Unbinds the vertex array. */
            virtual void Unbind() const = 0;

            /** @brief Adds a vertex buffer to the vertex array.
             * @param vertexBuffer The vertex buffer to add.
             */
            virtual void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) = 0;

            /** @brief Sets the index buffer for the vertex array.
             * @param indexBuffer The index buffer to set.
             */
            virtual void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) = 0;

            /** @brief Gets the vertex buffers associated with the vertex array.
             * @returns A constant reference to the vector of vertex buffers.
             */
            virtual const std::vector<Ref<VertexBuffer>> &GetVertexBuffers() const = 0;

            /** @brief Gets the index buffer associated with the vertex array.
             * @returns A constant reference to the index buffer.
             */
            virtual const Ref<IndexBuffer> &GetIndexBuffer() const = 0;

            /** @brief Creates a vertex array.
             * @returns A reference to the created VertexArray.
             */
            static Ref<VertexArray> Create();
    };
} // namespace Vulkyrie::Renderer
