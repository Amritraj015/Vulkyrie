#include "asteroids_application.h"

Vulkyrie::Core::VulkyrieApplication *CreateVulkyrieApplication() {
    Vulkyrie::Core::VulkyrieWindowProps windowProps;
    windowProps.height = 600;
    windowProps.width = 800;
    windowProps.startX = 100;
    windowProps.startY = 100;
    windowProps.title = "Asteroids (Powered by The Vulkyrie Game Engine)";

    Vulkyrie::Core::VulkyrieAppConfig appConfig;
    appConfig.graphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    appConfig.loggerType = Vulkyrie::Core::LoggerType::Console;

    return new Asteroids::AsteroidsApplication(windowProps, appConfig);
}
