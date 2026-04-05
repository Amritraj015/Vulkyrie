#pragma once

#include "renderer/vertex_array.h"

namespace Vulkyrie {
    class OpenGLVertexArray final : public VertexArray {
        public:
            OpenGLVertexArray();
            virtual ~OpenGLVertexArray();

            void Bind() const override;
            void Unbind() const override;

            void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) override;
            void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) override;

            const std::vector<Ref<VertexBuffer>> &GetVertexBuffers() const override;
            const Ref<IndexBuffer> &GetIndexBuffer() const override;
            VertexBuffer &GetVertexBuffer(size_t index) const override;

        private:
            u32 _vaoID;
            u32 _vertexBufferIndex = 0;
            std::vector<Ref<VertexBuffer>> _vertexBuffers;
            Ref<IndexBuffer> _indexBuffer;
    };
} // namespace Vulkyrie
