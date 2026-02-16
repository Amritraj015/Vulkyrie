#include "asteroids_application.h"

std::unique_ptr<Vulkyrie::Core::Application> CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps = {
        .Height = 600,
        .Width = 800,
        .Title = "Asteroids (Powered by The Vulkyrie Game Engine)",
        .GraphicsAPI = Vulkyrie::Core::GraphicsAPI::OpenGL,
    };

    return std::make_unique<Asteroids::AsteroidsApplication>(windowProps);
}
