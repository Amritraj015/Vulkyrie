#include "sandbox_application.h"

std::unique_ptr<Vulkyrie::Core::Application> CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps = {
        .Height = 800,
        .Width = 1500,
        .Title = "Sandbox (Powered by The Vulkyrie Game Engine)",
        .GraphicsAPI = Vulkyrie::Core::GraphicsAPI::OpenGL,
    };

    return std::make_unique<Sandbox::SandboxApplication>(windowProps);
}
