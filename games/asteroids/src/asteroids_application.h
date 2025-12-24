#pragma once

#include <vulkyrie.h>

namespace Asteroids {
    class AsteroidsApplication : public Vulkyrie::Core::VulkyrieApplication {
        public:
            AsteroidsApplication(Vulkyrie::Core::VulkyrieWindowProps windowProps, Vulkyrie::Core::VulkyrieAppConfig config)
                : Vulkyrie::Core::VulkyrieApplication(windowProps, config) { }

            ~AsteroidsApplication() override = default;

            // Additional methods and members specific to Asteroids can be added here.
    };
} // namespace Asteroids
