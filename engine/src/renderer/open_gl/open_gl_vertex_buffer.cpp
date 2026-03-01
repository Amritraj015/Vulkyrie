#include <glad/glad.h>
#include "renderer/open_gl/open_gl_vertex_buffer.h"

namespace Vulkyrie::Renderer {
    OpenGLVertexBuffer::OpenGLVertexBuffer(size_t size) {
        glCreateBuffers(1, &_vboID);
        // glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        // glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);

        glNamedBufferData(_vboID, size, nullptr, GL_DYNAMIC_DRAW);
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(f32 *vertices, size_t size) {
        glCreateBuffers(1, &_vboID);
        // glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        // glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);

        glNamedBufferData(_vboID, size, vertices, GL_STATIC_DRAW);
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(const std::vector<Vertex> &vertices) {
        glCreateBuffers(1, &_vboID);
        // glBindBuffer(GL_ARRAY_BUFFER, _vboId);
        // glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

        glNamedBufferData(_vboID, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer() {
        glDeleteBuffers(1, &_vboID);
    }

    void OpenGLVertexBuffer::SetData(const void *data, size_t size) {
        glBindBuffer(GL_ARRAY_BUFFER, _vboID);
        glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    }
} // namespace Vulkyrie::Renderer
