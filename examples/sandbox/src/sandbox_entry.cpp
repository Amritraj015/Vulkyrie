#include "sandbox_application.h"

using namespace Vulkyrie;

std::unique_ptr<Application> CreateApplication() {
    WindowProps windowProps = {
        .Height = 800,
        .Width = 1500,
        .Title = "Sandbox (Powered by The Vulkyrie Game Engine)",
        .EnableVSync = false,
        .GraphicsAPI = GraphicsAPI::OpenGL,
    };

    return std::make_unique<Sandbox::SandboxApplication>(windowProps);
}
