#pragma once

#include "vulkyrie_window_props.h"
#include "vulkyrie_app_config.h"

namespace Vulkyrie::Core {
    class VulkyrieApplication {
        public:
            VulkyrieApplication(VulkyrieWindowProps windowProps, VulkyrieAppConfig config) : windowProps(windowProps), config(config) {}
            virtual ~VulkyrieApplication() = default;

            // Window properties for the application.
            VulkyrieWindowProps windowProps;

            // The application configuration.
            VulkyrieAppConfig config;
    };
} // namespace Vulkyrie::Core
