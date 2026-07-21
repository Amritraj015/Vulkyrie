#include "sandbox_application.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    WindowProps windowProps = {
        .Height = 800,
        .Width = 1500,
        .Title = "Sandbox (Powered by The Vulkyrie Game Engine)",
        .EnableVSync = false,
        .GraphicsAPI = GraphicsAPI::OpenGL,
    };

    return new Sandbox::SandboxApplication(windowProps);
}
