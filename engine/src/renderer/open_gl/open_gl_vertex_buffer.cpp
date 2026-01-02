#include <glad/glad.h>
#include "open_gl_vertex_buffer.h"

namespace Vulkyrie::Renderer {
    OpenGLVertexBuffer::OpenGLVertexBuffer(size_t size) {
        glCreateBuffers(1, &_vboId);
        glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(float *vertices, size_t size) {
        glCreateBuffers(1, &_vboId);
        // glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        // glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

        glNamedBufferData(_vboId, size, vertices, GL_STATIC_DRAW);
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer() {
        glDeleteBuffers(1, &_vboId);
    }

    void OpenGLVertexBuffer::Bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, _vboId);
    }

    void OpenGLVertexBuffer::Unbind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void OpenGLVertexBuffer::SetData(const void *data, size_t size) {
        glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }

    const BufferLayout &OpenGLVertexBuffer::GetLayout() const {
        return _layout;
    }

    void OpenGLVertexBuffer::SetLayout(const BufferLayout &layout) {
        _layout = layout;
    }
} // namespace Vulkyrie::Renderer
