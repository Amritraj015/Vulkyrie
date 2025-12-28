#pragma once

#include "graphics_api.h"

namespace Vulkyrie::Core {
    struct ApplicationConfig {
        public:
            // Vulkyrie::Core::LoggerType loggerType = Vulkyrie::Core::LoggerType::Console;
            Vulkyrie::Core::GraphicsAPI graphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    };
} // namespace Vulkyrie::Core
