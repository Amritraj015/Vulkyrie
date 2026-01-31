#include "renderer/graphics_context.h"
#include "renderer/renderer.h"
#include "renderer/open_gl/open_gl_graphics_context.h"
#include "core/logger.h"
#include "core/application.h"


namespace Vulkyrie::Renderer {

    Ref<GraphicsContext> GraphicsContext::Create() {
        using Vulkyrie::Core::Application;
        using Vulkyrie::Core::GraphicsAPI;

        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return CreateRef<OpenGLGraphicsContext>(Application::GetSingleton().GetWindowHandle());
            default:
                VERROR("Unsupported graphics API specified for graphics context creation.");
                return nullptr;
        }
    }

} // namespace Vulkyrie::Renderer