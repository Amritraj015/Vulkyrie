#include "renderer/backends/open_gl/open_gl_context.h"

namespace Vulkyrie {

    OpenGLContext::OpenGLContext(const DeviceCreationInfo &info)
        : mCapabilities() {
        // TODO: Finish this.
        (void)info;
    }

    OpenGLContext::~OpenGLContext() = default;

    void OpenGLContext::WaitIdle() const {
        // TODO: glFinish() once a GL context exists here.
    }

    bool OpenGLContext::DeviceLost() const {
        return false;
    }

} // namespace Vulkyrie
