#include "pong_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps;
    windowProps.Height = 600;
    windowProps.Width = 800;
    windowProps.StartX = 100;
    windowProps.StartY = 100;
    windowProps.Title = "Pong (Powered by The Vulkyrie Game Engine)";

    Vulkyrie::Core::ApplicationConfig appConfig;
    appConfig.GraphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    // appConfig.loggerType = Vulkyrie::Core::LoggerType::Console;

    Pong::PongApplication *pongApp = new Pong::PongApplication(windowProps, appConfig);

    return pongApp;
}
