#pragma once

#include "logger_type.h"
#include "grapthics_api.h"

namespace Vulkyrie::Core {
    struct VulkyrieAppConfig {
        Vulkyrie::Core::LoggerType loggerType = Vulkyrie::Core::LoggerType::Console;
        Vulkyrie::Core::GraphicsAPI graphicsApi = Vulkyrie::Core::GraphicsAPI::OpenGL;
    };
}