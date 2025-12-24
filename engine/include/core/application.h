#pragma once

#include "window_props.h"
#include "application_config.h"
#include "layer/layer.h"

namespace Vulkyrie::Core {
    class Application {
        public:
            Application(WindowProps windowProps, ApplicationConfig config) : windowProps(windowProps), config(config) {
            }
            virtual ~Application() = default;

            // Window properties for the application.
            WindowProps windowProps;

            // The application configuration.
            ApplicationConfig config;

            // Layers associated with the application.
            std::vector<std::unique_ptr<Vulkyrie::Layer::Layer>> layers;
    };
} // namespace Vulkyrie::Core
