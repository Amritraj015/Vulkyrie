#include <glad/glad.h>
#include "core/logger.h"
#include "open_gl_vertex_array.h"

namespace Vulkyrie::Renderer {
    static constexpr GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
        switch (type) {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::Mat3:
            case ShaderDataType::Mat4:
                return GL_FLOAT;
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                return GL_INT;
            case ShaderDataType::Bool:
                return GL_BOOL;
            case ShaderDataType::None:
                break;
        }

        return 0;
    }

    OpenGLVertexArray::OpenGLVertexArray() {
        glCreateVertexArrays(1, &_vaoId);
    }

    OpenGLVertexArray::~OpenGLVertexArray() {
        glDeleteVertexArrays(1, &_vaoId);
    }

    void OpenGLVertexArray::Bind() const {
        glBindVertexArray(_vaoId);
    }

    void OpenGLVertexArray::Unbind() const {
        glBindVertexArray(0);
    }

    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) {
        glBindVertexArray(_vaoId);
        vertexBuffer->Bind();

        const auto &layout = vertexBuffer->GetLayout();

        for (const auto &element : layout) {
            switch (element.Type) {
                case ShaderDataType::Float:
                case ShaderDataType::Float2:
                case ShaderDataType::Float3:
                case ShaderDataType::Float4: {
                    glEnableVertexAttribArray(_vertexBufferIndex);
                    glVertexAttribPointer(_vertexBufferIndex,
                                          element.GetComponentCount(),
                                          ShaderDataTypeToOpenGLBaseType(element.Type),
                                          element.Normalized ? GL_TRUE : GL_FALSE,
                                          layout.GetStride(),
                                          (const void *)element.Offset);
                    _vertexBufferIndex++;
                    break;
                }
                case ShaderDataType::Int:
                case ShaderDataType::Int2:
                case ShaderDataType::Int3:
                case ShaderDataType::Int4:
                case ShaderDataType::Bool: {
                    glEnableVertexAttribArray(_vertexBufferIndex);
                    glVertexAttribIPointer(_vertexBufferIndex,
                                           element.GetComponentCount(),
                                           ShaderDataTypeToOpenGLBaseType(element.Type),
                                           layout.GetStride(),
                                           (const void *)element.Offset);
                    _vertexBufferIndex++;
                    break;
                }
                case ShaderDataType::Mat3:
                case ShaderDataType::Mat4: {
                    uint8_t count = element.GetComponentCount();
                    for (uint8_t i = 0; i < count; i++) {
                        glEnableVertexAttribArray(_vertexBufferIndex);
                        glVertexAttribPointer(_vertexBufferIndex,
                                              count,
                                              ShaderDataTypeToOpenGLBaseType(element.Type),
                                              element.Normalized ? GL_TRUE : GL_FALSE,
                                              layout.GetStride(),
                                              (const void *)(element.Offset + sizeof(float) * count * i));
                        glVertexAttribDivisor(_vertexBufferIndex, 1);
                        _vertexBufferIndex++;
                    }
                    break;
                }
                default:
                    VERROR("Unknown ShaderDataType!");
            }
        }

        _vertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) {
        glBindVertexArray(_vaoId);
        indexBuffer->Bind();

        _indexBuffer = indexBuffer;
    }

    const std::vector<Ref<VertexBuffer>> &OpenGLVertexArray::GetVertexBuffers() const {
        return _vertexBuffers;
    }

    const Ref<IndexBuffer> &OpenGLVertexArray::GetIndexBuffer() const {
        return _indexBuffer;
    }
} // namespace Vulkyrie::Renderer
