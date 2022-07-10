#pragma once

#include "defines.h"

#include "platform/platform.hpp"
#include "core/logger/logger.hpp"

class ServiceProvider
{
private:
    static Platform platform_instance;
    static Logger logger_instance;

public:
    /* Gets the appropriate platform on which the engine is running on. */
    static IPlatform &get_platform();

    /* Gets the register logger instance. */
    static ILogger &get_logger();
};