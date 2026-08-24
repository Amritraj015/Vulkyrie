#include "sandbox_application.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    const Vulkyrie::ApplicationInfo appInfo{
        .Name = "Sandbox",
        .Version = { 1, 0, 0 },
    };

    const Vulkyrie::ApplicationSettings settings = {
        .GeneralSettings = appInfo,
        .GraphicsSettings = {
            .ValidationSettings = {},
            .WindowDimensions = { 1500, 800 },
            .API = GraphicsAPI::Vulkan,
            .EnableVSync = false,
        },
    };

    return new Sandbox::SandboxApplication(settings);
}
