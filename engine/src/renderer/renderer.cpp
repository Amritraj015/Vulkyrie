#include "renderer/renderer.h"

namespace Vulkyrie::Renderer {
    static Vulkyrie::Core::GraphicsAPI RendererAPI = Vulkyrie::Core::GraphicsAPI::None;

    void Initialize(Vulkyrie::Core::GraphicsAPI api) { RendererAPI = api; }

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
