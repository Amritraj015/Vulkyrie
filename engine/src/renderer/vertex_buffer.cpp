#include "vlkypch.h"
#include "renderer/vertex_buffer.h"
#include "renderer/open_gl/open_gl_vertex_buffer.h"
#include "renderer/renderer_context.h"

namespace Vulkyrie {

    Ref<VertexBuffer> VertexBuffer::Create(size_t size) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(size);
            case GraphicsAPI::Vulkan:
            default:
                VERROR("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(float *vertices, size_t size) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
            case GraphicsAPI::Vulkan:
            default:
                VERROR("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(const std::vector<Vertex> &vertices) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices);
            case GraphicsAPI::Vulkan:
            default:
                VERROR("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

} // namespace Vulkyrie
