#include "sandbox_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps = {
        .StartX = 100,
        .StartY = 100,
        .Height = 600,
        .Width = 800,
        .Title = "Sandbox (Powered by The Vulkyrie Game Engine)",
        .GraphicsAPI = Vulkyrie::Core::GraphicsAPI::OpenGL,
    };

    Sandbox::SandboxApplication *sandboxApp = new Sandbox::SandboxApplication(windowProps);

    return sandboxApp;
}
