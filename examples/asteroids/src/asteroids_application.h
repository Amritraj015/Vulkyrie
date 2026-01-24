#pragma once

#include <vulkyrie.h>

namespace Asteroids {
    class AsteroidsApplication : public Vulkyrie::Core::Application {
        public:
            AsteroidsApplication(const Vulkyrie::Core::WindowProps &windowProps) : Vulkyrie::Core::Application(windowProps) {
            }

            ~AsteroidsApplication() override = default;
    };
} // namespace Asteroids
