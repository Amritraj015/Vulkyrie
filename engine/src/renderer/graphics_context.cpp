#include "vlkypch.h"
#include "renderer/renderer_context.h"
#include "renderer/renderer.h"
#include "renderer/open_gl/open_gl_renderer_context.h"
#include "core/application.h"

namespace Vulkyrie {

    Scope<RendererContext> RendererContext::Create() {
        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateScope<OpenGLRendererContext>(Application::GetSingleton().GetWindowHandle());
            default:
                VFATAL("Unsupported graphics API '{}' specified for graphics context creation.", GetCurrentGraphicsAPIName());
                return nullptr;
        }
    }

} // namespace Vulkyrie
