#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongApplication : public Vulkyrie::Core::Application {
        public:
            PongApplication(Vulkyrie::Core::WindowProps windowProps, Vulkyrie::Core::ApplicationConfig config)
                : Vulkyrie::Core::Application(windowProps, config) {
            }

            ~PongApplication() override = default;

            // Additional Pong-specific methods and members can be added here
    };
} // namespace Pong
