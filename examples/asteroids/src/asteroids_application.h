#pragma once

#include <vulkyrie.h>

namespace Asteroids {
    using namespace Vulkyrie;

    class AsteroidsApplication : public Application {
    public:
        AsteroidsApplication(const WindowProps &windowProps)
            : Application(windowProps) {
        }

        ~AsteroidsApplication() override = default;
    };
} // namespace Asteroids
