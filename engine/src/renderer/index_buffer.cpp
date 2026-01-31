#include "core/logger.h"
#include "renderer/index_buffer.h"
#include "renderer/open_gl/open_gl_index_buffer.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    Ref<IndexBuffer> IndexBuffer::Create(u32 *indices, u32 count) {
        switch (Vulkyrie::Renderer::GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLIndexBuffer>(indices, count);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for IndexBuffer creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie::Renderer
