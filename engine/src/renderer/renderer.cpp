#include "renderer/renderer.h"
#include "core/asserts.h"

namespace Vulkyrie {

    extern Scope<Renderer> CreateVulkanRenderer(const DeviceCreationInfo &);
    extern Scope<Renderer> CreateOpenGLRenderer(const DeviceCreationInfo &);

    Scope<Renderer> Renderer::Create(const GraphicsAPI api, const DeviceCreationInfo &info) {
        switch (api) {
            case GraphicsAPI::Vulkan:
                return CreateVulkanRenderer(info);
            case GraphicsAPI::OpenGL:
                return CreateOpenGLRenderer(info);
            default:
                VASSERT(false, "Invalid Renderer Type");
        };

        std::unreachable();
    }

} // namespace Vulkyrie
