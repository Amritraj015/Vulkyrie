#include "vlkypch.h"
#include "renderer/vertex_buffer.h"
#include "renderer/open_gl/open_gl_vertex_buffer.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    Ref<VertexBuffer> VertexBuffer::Create(size_t size) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(size);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VERROR("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(float *vertices, size_t size) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VERROR("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(const std::vector<Vertex> &vertices) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VERROR("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
