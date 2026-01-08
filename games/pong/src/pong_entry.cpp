#include "pong_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps;
    windowProps.Height = 600;
    windowProps.Width = 800;
    windowProps.StartX = 100;
    windowProps.StartY = 100;
    windowProps.VSync = false;
    windowProps.Title = "Pong (Powered by The Vulkyrie Game Engine)";

    Pong::PongApplication *pongApp = new Pong::PongApplication(windowProps);

    return pongApp;
}
