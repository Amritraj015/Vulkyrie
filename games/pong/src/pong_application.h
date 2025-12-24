#pragma once

#include <vulkyrie.h>

namespace Pong {
    class PongApplication : public Vulkyrie::Core::VulkyrieApplication {
        public:
            PongApplication(Vulkyrie::Core::VulkyrieWindowProps windowProps, Vulkyrie::Core::VulkyrieAppConfig config) 
                : Vulkyrie::Core::VulkyrieApplication(windowProps, config) { }

            ~PongApplication() override = default;

            // Additional Pong-specific methods and members can be added here
    };
} // namespace Pong
