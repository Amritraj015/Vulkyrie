#include "renderer/renderer.h"
#include "renderer/graphics_context.h"
#include "renderer/open_gl/open_gl_renderer.h"
#include "core/logger.h"

namespace Vulkyrie::Renderer {
    static Vulkyrie::Core::GraphicsAPI RendererAPI = Vulkyrie::Core::GraphicsAPI::None;
    Scope<GraphicsContext> _graphicsContext = nullptr;
    Scope<Renderer> _renderer = nullptr;

    Scope<Renderer> &GetRenderer() {
        return _renderer;
    }

    Vulkyrie::Core::GraphicsAPI GetCurrentGraphicsAPI() {
        return RendererAPI;
    }

    Vulkyrie::Core::StatusCode Initialize(Vulkyrie::Core::GraphicsAPI api) {
        RendererAPI = api;

        // Create the graphics context.
        _graphicsContext = GraphicsContext::Create();

        // Check if graphics context creation failed.
        if (nullptr == _graphicsContext) {
            VFATAL("Failed to create graphics context for the specified graphics API.");
            return Vulkyrie::Core::StatusCode::UnsupportedGraphicsAPI;
        }

        // Try to initialize the graphics context.
        RETURN_ON_FAILURE(_graphicsContext->Initialize());

        // Create the renderer based on the specified graphics API.
        switch (RendererAPI) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                _renderer = CreateScope<OpenGLRenderer>();
                break;
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported graphics API specified for renderer initialization.");
                return Vulkyrie::Core::StatusCode::UnsupportedGraphicsAPI;
        }

        // Return success.
        return Vulkyrie::Core::StatusCode::Successful;
    }

    std::string_view GetCurrentGraphicsAPIName() {
        switch (GetCurrentGraphicsAPI()) {
            case Vulkyrie::Core::GraphicsAPI::OpenGL:
                return "OpenGL";
            case Vulkyrie::Core::GraphicsAPI::Vulkan:
                return "Vulkan";
            default:
                return "Unknown";
        }
    }

} // namespace Vulkyrie::Renderer
