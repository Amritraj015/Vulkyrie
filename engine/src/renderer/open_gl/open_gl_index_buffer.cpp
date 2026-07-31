#include <glad/glad.h>
#include "renderer/open_gl/open_gl_index_buffer.h"

namespace Vulkyrie {

    IndexBufferHandle OpenGLIndexBuffer::Create(std::span<const u32> indices) {
        return createBuffer(indices.data(), indices.size_bytes(), indices.size(), IndexType::U32);
    }

    IndexBufferHandle OpenGLIndexBuffer::Create(std::span<const u16> indices) {
        return createBuffer(indices.data(), indices.size_bytes(), indices.size(), IndexType::U16);
    }

    IndexBufferHandle OpenGLIndexBuffer::Create(std::span<const u8> indices) {
        return createBuffer(indices.data(), indices.size_bytes(), indices.size(), IndexType::U8);
    }

    IndexBufferHandle OpenGLIndexBuffer::createBuffer(const void *data, size_t sizeInBytes, size_t count, IndexType indexType) {
        // An index buffer with nothing in it cannot be drawn with, and glNamedBufferStorage rejects a zero size
        // outright, so this is reported as a failed creation rather than left to surface as a GL error the caller
        // never sees.
        if (data == nullptr || sizeInBytes == 0) {
            return IndexBufferHandle{};
        }

        u32 index = 0;

        if (!_freeIndices.empty()) {
            index = _freeIndices.back();
            _freeIndices.pop_back();
        } else {
            index = static_cast<u32>(_indexBuffers.size());
            _indexBuffers.emplace_back();
        }

        OpenGLIndexBufferSlot &bufferSlot = _indexBuffers[index];
        bufferSlot.Count = count;
        bufferSlot.Type = indexType;

        glCreateBuffers(1, &bufferSlot.EBO);
        glNamedBufferStorage(bufferSlot.EBO, static_cast<GLsizeiptr>(sizeInBytes), data, GL_NONE);

        return IndexBufferHandle(index, bufferSlot.Generation);
    }

    bool OpenGLIndexBuffer::Destroy(IndexBufferHandle bufferHandle) {
        OpenGLIndexBufferSlot *bufferSlot = resolve(bufferHandle);

        if (bufferSlot == nullptr) {
            return false;
        }

        glDeleteBuffers(1, &bufferSlot->EBO);

        bufferSlot->Reset();
        _freeIndices.push_back(bufferHandle.Index());

        return true;
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer() {
        for (OpenGLIndexBufferSlot &bufferSlot : _indexBuffers) {
            // A released slot holds 0, which glDeleteBuffers ignores, so live and free slots need no distinguishing.
            glDeleteBuffers(1, &bufferSlot.EBO);
        }
    }

} // namespace Vulkyrie
