#include "vlkypch.h"
#include "renderer/frame_buffer.h"
#include "renderer/open_gl/open_gl_frame_buffer.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {

    Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferSpecification &specification) {
        switch (GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return CreateRef<OpenGLFrameBuffer>(specification);
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported Graphics API specified for FrameBuffer creation!");
                break;
        }

        return nullptr;
    }

} // namespace Vulkyrie::Renderer
