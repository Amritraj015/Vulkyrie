#include "renderer/backends/open_gl/open_gl_swapchain.h"

namespace Vulkyrie {

    OpenGLSwapchain::OpenGLSwapchain(OpenGLContext *context, const WindowHandle &windowHandle, u32 width, u32 height, bool vSync)
        : pContext(context)
        , mFormat(Format::RGBA8Srgb)
        , mWidth(width)
        , mHeight(height)
        , mVSync(vSync) {
        (void)windowHandle;
    }

    OpenGLSwapchain::AcquiredImage OpenGLSwapchain::Acquire(u64 timeoutNs) {
        (void)timeoutNs;

        return {};
    }

    bool OpenGLSwapchain::Present(u32 imageIndex, u32 renderFinished) {
        (void)imageIndex;
        (void)renderFinished;

        return true;
    }

    void OpenGLSwapchain::Recreate(u32 width, u32 height, bool vSync) {
        (void)width;
        (void)height;
        (void)vSync;
    }

} // namespace Vulkyrie
