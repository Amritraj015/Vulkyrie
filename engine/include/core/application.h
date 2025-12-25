#pragma once

#include "window_props.h"
#include "application_config.h"
#include "core/layer.h"

namespace Vulkyrie::Core {
    class Application {
        public:
            Application(const WindowProps &windowProps, const ApplicationConfig &config) : windowProps(windowProps), config(config) {
            }
            virtual ~Application() = default;

            // Window properties for the application.
            WindowProps windowProps;

            // The application configuration.
            ApplicationConfig config;

            // Layers associated with the application.
            std::vector<std::unique_ptr<Vulkyrie::Core::Layer>> layers;
    };
} // namespace Vulkyrie::Core
