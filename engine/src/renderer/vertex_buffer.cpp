#include "core/logger.h"
#include "renderer/vertex_buffer.h"
#include "renderer/open_gl/open_gl_vertex_buffer.h"

namespace Vulkyrie::Renderer {
    Ref<VertexBuffer> VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI api, size_t size) {
        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(size);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI api, float *vertices, size_t size) {
        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices, size);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }

    Ref<VertexBuffer> VertexBuffer::Create(Vulkyrie::Core::GraphicsAPI api, const std::vector<Vertex> &vertices) {
        switch (api) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexBuffer>(vertices);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API for VertexBuffer creation!");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Renderer
