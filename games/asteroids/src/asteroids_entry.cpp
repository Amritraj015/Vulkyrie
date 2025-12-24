#include "asteroids_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps;
    windowProps.height = 600;
    windowProps.width = 800;
    windowProps.startX = 100;
    windowProps.startY = 100;
    windowProps.title = "Asteroids (Powered by The Vulkyrie Game Engine)";

    Vulkyrie::Core::ApplicationConfig appConfig;
    appConfig.graphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    appConfig.loggerType = Vulkyrie::Core::LoggerType::Console;

    return new Asteroids::AsteroidsApplication(windowProps, appConfig);
}
