#include "vlkypch.h"
#include "renderer/index_buffer.h"
#include "renderer/open_gl/open_gl_index_buffer.h"
#include "renderer/renderer.h"

namespace Vulkyrie {
    Ref<IndexBuffer> IndexBuffer::Create(u32 *indices, u32 count) {
        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLIndexBuffer>(indices, count);
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for IndexBuffer creation!");
                break;
        }

        return nullptr;
    }
} // namespace Vulkyrie
