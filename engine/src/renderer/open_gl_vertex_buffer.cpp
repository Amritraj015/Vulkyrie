#include "open_gl_vertex_buffer.h"

namespace Vulkyrie::Renderer {

    OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size) {
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(float *vertices, uint32_t size) {
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer() {
    }

    void OpenGLVertexBuffer::Bind() const {
    }

    void OpenGLVertexBuffer::Unbind() const {
    }

    void OpenGLVertexBuffer::SetData(const void *data, size_t size) {
    }

    // const BufferLayout &OpenGLVertexBuffer::GetLayout() const {
    // }

    void OpenGLVertexBuffer::SetLayout(const BufferLayout &layout) {
    }
} // namespace Vulkyrie::Renderer
