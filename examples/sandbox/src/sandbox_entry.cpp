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
            .API = GraphicsAPI::Vulkan,
            .WindowHeight = 800,
            .WindowWidth = 1500,
            .EnableVSync = false,
        },
    };

    return new Sandbox::SandboxApplication(settings);
}
