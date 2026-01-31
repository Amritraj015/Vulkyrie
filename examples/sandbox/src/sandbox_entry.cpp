#include "sandbox_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps;
    windowProps.Height = 600;
    windowProps.Width = 800;
    windowProps.StartX = 100;
    windowProps.StartY = 100;
    windowProps.Title = "Sandbox (Powered by The Vulkyrie Game Engine)";

    Sandbox::SandboxApplication *sandboxApp = new Sandbox::SandboxApplication(windowProps);

    return sandboxApp;
}
