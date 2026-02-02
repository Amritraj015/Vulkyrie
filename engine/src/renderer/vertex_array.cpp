#include "vlkypch.h"
#include "renderer/vertex_array.h"
#include "renderer/open_gl/open_gl_vertex_array.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    Ref<VertexArray> VertexArray::Create() {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLVertexArray>();
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for VertexArray creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie::Renderer
