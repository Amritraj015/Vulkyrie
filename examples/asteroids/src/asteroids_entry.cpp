#include "asteroids_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps = {
        .StartX = 100,
        .StartY = 100,
        .Height = 600,
        .Width = 800,
        .Title = "Asteroids (Powered by The Vulkyrie Game Engine)",
        .GraphicsAPI = Vulkyrie::Core::GraphicsAPI::OpenGL,
    };

    Asteroids::AsteroidsApplication *asteroidsApp = new Asteroids::AsteroidsApplication(windowProps);

    return asteroidsApp;
}
