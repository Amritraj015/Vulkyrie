#include "asteroids_application.h"

using namespace Vulkyrie;

std::unique_ptr<Application> CreateApplication() {
    WindowProps windowProps = {
        .Height = 600,
        .Width = 800,
        .Title = "Asteroids (Powered by The Vulkyrie Game Engine)",
        .GraphicsAPI = GraphicsAPI::OpenGL,
    };

    return std::make_unique<Asteroids::AsteroidsApplication>(windowProps);
}
