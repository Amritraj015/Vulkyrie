#include "sandbox_application.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    Vulkyrie::ApplicationSettings settings = {
        .Name = "Sandbox (Powered by The Vulkyrie Game Engine)",
        .GraphicsSettings = {
            .API = GraphicsAPI::OpenGL,
            .WindowHeight = 800,
            .WindowWidth = 1500,
            .EnableVSync = false,
        },
    };

    return new Sandbox::SandboxApplication(settings);
}
