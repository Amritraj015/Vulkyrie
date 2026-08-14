#include "vlkypch.h"
#include "renderer/frame_buffer.h"
#include "renderer/open_gl/open_gl_frame_buffer.h"
#include "renderer/renderer_context.h"

namespace Vulkyrie {

    Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification &specification) {
        switch (RendererContext::GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLFrameBuffer>(specification);
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for FrameBuffer creation!");
                break;
        }

        return nullptr;
    }

} // namespace Vulkyrie
