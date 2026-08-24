#include "sandbox_application.h"

using namespace Vulkyrie;

Application *CreateApplication() {
    const Vulkyrie::ApplicationInfo appInfo{
        .Name = "Sandbox (Powered by The Vulkyrie Game Engine)",
        .MajorVersion = 1,
        .MinorVersion = 0,
        .PatchVersion = 0,
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
