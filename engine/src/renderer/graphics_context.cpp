#include "renderer/graphics_context.h"
#include "renderer/renderer.h"
#include "renderer/open_gl/open_gl_graphics_context.h"
#include "core/logger.h"
#include "core/application.h"

namespace Vulkyrie::Renderer {

    Scope<GraphicsContext> GraphicsContext::Create() {
        using Vulkyrie::Core::Application;
        using Vulkyrie::Core::GraphicsAPI;

        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateScope<OpenGLGraphicsContext>(Application::GetSingleton().GetWindowHandle());
            default:
                VFATAL("Unsupported graphics API '{}' specified for graphics context creation.", GetCurrentGraphicsAPIName());
                return nullptr;
        }
    }

} // namespace Vulkyrie::Renderer
