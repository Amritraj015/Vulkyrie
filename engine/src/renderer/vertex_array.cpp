#include "vlkypch.h"
#include "renderer/vertex_array.h"
#include "renderer/open_gl/open_gl_vertex_array.h"
#include "renderer/renderer.h"

namespace Vulkyrie {
    Ref<VertexArray> VertexArray::Create() {
        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexArray>();
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for VertexArray creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie
