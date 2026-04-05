#include "vlkypch.h"
#include "renderer/renderer.h"
#include "renderer/renderer_context.h"

namespace Vulkyrie {
    GraphicsAPI RendererAPI = GraphicsAPI::None;
    Scope<RendererContext> _graphicsContext = nullptr;

    GraphicsAPI GetCurrentGraphicsAPI() {
        return RendererAPI;
    }

    StatusCode Initialize(GraphicsAPI api) {
        RendererAPI = api;

        // Create the graphics context.
        _graphicsContext = RendererContext::Create();

        // Check if graphics context creation failed.
        if (nullptr == _graphicsContext) {
            VFATAL("Failed to create graphics context for the specified graphics API: {}.", GetCurrentGraphicsAPIName());
            return StatusCode::UnsupportedGraphicsAPI;
        }

        // Try to initialize the graphics context.
        RETURN_ON_FAILURE(_graphicsContext->Initialize());

        // Create the renderer based on the specified graphics API.
        switch (RendererAPI) {
            case GraphicsAPI::OpenGL:
                break;
            case GraphicsAPI::Vulkan:
            default:
                VFATAL("Unsupported graphics API specified for renderer initialization.");
                return StatusCode::UnsupportedGraphicsAPI;
        }

        // Return success.
        return StatusCode::Successful;
    }

    std::string_view GetCurrentGraphicsAPIName() {
        switch (GetCurrentGraphicsAPI()) {
            case GraphicsAPI::OpenGL:
                return "OpenGL";
            case GraphicsAPI::Vulkan:
                return "Vulkan";
            default:
                return "Unknown";
        }
    }

} // namespace Vulkyrie
