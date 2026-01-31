#include "asteroids_application.h"

Vulkyrie::Core::Application *CreateApplication() {
    Vulkyrie::Core::WindowProps windowProps;
    windowProps.Height = 600;
    windowProps.Width = 800;
    windowProps.StartX = 100;
    windowProps.StartY = 100;
    windowProps.Title = "Asteroids (Powered by The Vulkyrie Game Engine)";

    Asteroids::AsteroidsApplication *asteroidsApp = new Asteroids::AsteroidsApplication(windowProps);

    return asteroidsApp;
}
