#include "pong_application.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

Vulkyrie::Core::Application *CreateApplication() {
    glm::vec4 point(1.0f, 0.0f, 0.0f, 1.0f);
    glm::mat4 trans(1.0f);
    trans = glm::translate(trans, glm::vec3(1.0f, 1.0f, 0.0f));
    glm::vec4 result = trans * point;
    std::cout << "Resulting point: (" << result.x << ", " << result.y << ", " << result.z << ", " << result.w << ")" << std::endl;

    Vulkyrie::Core::WindowProps windowProps;
    windowProps.Height = 600;
    windowProps.Width = 800;
    windowProps.StartX = 100;
    windowProps.StartY = 100;
    windowProps.VSync = true;
    windowProps.Title = "Pong (Powered by The Vulkyrie Game Engine)";

    Vulkyrie::Core::ApplicationConfig appConfig;
    appConfig.GraphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    // appConfig.loggerType = Vulkyrie::Core::LoggerType::Console;

    Pong::PongApplication *pongApp = new Pong::PongApplication(windowProps, appConfig);

    return pongApp;
}
