#include "graphics_context.h"
#include "core/logger.h"
#include "core/open_gl/open_gl_graphics_context.h"

namespace Vulkyrie::Core {

    Scope<GraphicsContext> GraphicsContext::Create(Vulkyrie::Core::GraphicsAPI api, void *windowHandle) {
        switch (api) {
            case GraphicsAPI::OpenGL:
                return CreateScope<Vulkyrie::Core::OpenGLGraphicsContext>(windowHandle);
            default:
                VERROR("Unsupported graphics API specified for graphics context creation.");
                return nullptr;
        }
    }
} // namespace Vulkyrie::Core
