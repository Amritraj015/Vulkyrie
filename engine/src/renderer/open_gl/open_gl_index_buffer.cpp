#include <glad/glad.h>
#include "open_gl_index_buffer.h"

namespace Vulkyrie::Renderer {
    OpenGLIndexBuffer::OpenGLIndexBuffer(u32 *indices, u32 count) {
        glCreateBuffers(1, &_eboId);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
        glBindBuffer(GL_ARRAY_BUFFER, _eboId);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(u32), indices, GL_STATIC_DRAW);
        _count = count;
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(u16 *indices, u32 count) {
        glCreateBuffers(1, &_eboId);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
        glBindBuffer(GL_ARRAY_BUFFER, _eboId);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(u16), indices, GL_STATIC_DRAW);
        _count = count;
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(u8 *indices, u32 count) {
        glCreateBuffers(1, &_eboId);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
        glBindBuffer(GL_ARRAY_BUFFER, _eboId);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(u8), indices, GL_STATIC_DRAW);
        _count = count;
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer() {
        glDeleteBuffers(1, &_eboId);
    }

    void OpenGLIndexBuffer::Bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _eboId);
    }

    void OpenGLIndexBuffer::Unbind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    u32 OpenGLIndexBuffer::GetCount() const {
        return _count;
    }
} // namespace Vulkyrie::Renderer
