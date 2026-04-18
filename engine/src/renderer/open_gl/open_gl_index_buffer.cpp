#include <glad/glad.h>
#include "renderer/open_gl/open_gl_index_buffer.h"

namespace Vulkyrie {

    OpenGLIndexBuffer::OpenGLIndexBuffer(u32 *indices, size_t count) {
        glCreateBuffers(1, &_eboID);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
        glBindBuffer(GL_ARRAY_BUFFER, _eboID);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(u32), indices, GL_STATIC_DRAW);
        _count = count;
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(u16 *indices, size_t count) {
        glCreateBuffers(1, &_eboID);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
        glBindBuffer(GL_ARRAY_BUFFER, _eboID);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(u16), indices, GL_STATIC_DRAW);
        _count = count;
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(u8 *indices, size_t count) {
        glCreateBuffers(1, &_eboID);

        // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
        // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
        glBindBuffer(GL_ARRAY_BUFFER, _eboID);
        glBufferData(GL_ARRAY_BUFFER, count * sizeof(u8), indices, GL_STATIC_DRAW);
        _count = count;
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer() {
        glDeleteBuffers(1, &_eboID);
    }

    size_t OpenGLIndexBuffer::GetCount() const {
        return _count;
    }

} // namespace Vulkyrie
