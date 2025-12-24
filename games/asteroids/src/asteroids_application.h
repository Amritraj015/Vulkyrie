#pragma once

#include <vulkyrie.h>

namespace Asteroids {
    class AsteroidsApplication : public Vulkyrie::Core::Application {
        public:
            AsteroidsApplication(Vulkyrie::Core::WindowProps windowProps, Vulkyrie::Core::ApplicationConfig config)
                : Vulkyrie::Core::Application(windowProps, config) {
            }

            ~AsteroidsApplication() override = default;

            // Additional methods and members specific to Asteroids can be added here.
    };
} // namespace Asteroids
