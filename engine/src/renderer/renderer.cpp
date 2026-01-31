#include "renderer/renderer.h"
#include "renderer/graphics_context.h"
#include "core/logger.h"

namespace Vulkyrie::Renderer {
    static Vulkyrie::Core::GraphicsAPI RendererAPI = Vulkyrie::Core::GraphicsAPI::None;
    Ref<GraphicsContext> _graphicsContext = nullptr;

    Vulkyrie::Core::StatusCode Initialize(Vulkyrie::Core::GraphicsAPI api) {
        RendererAPI = api; 
    
        // Create the graphics context.
        _graphicsContext = GraphicsContext::Create();

        // Check if graphics context creation failed.
        if (nullptr == _graphicsContext) {
            VFATAL("Failed to create graphics context for the specified graphics API.");
            return Vulkyrie::Core::StatusCode::FailedToCreateGraphicsContext;
        }
    
        // Initialize the graphics context.
        _graphicsContext->Initialize();

        // Return success.
        return Vulkyrie::Core::StatusCode::Successful;
    }

    Vulkyrie::Core::GraphicsAPI GetCurrentGraphicsAPI() { return RendererAPI; }

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
