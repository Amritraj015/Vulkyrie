#include "service_provider.hpp"

Platform ServiceProvider::platform_instance;
Logger ServiceProvider::logger_instance;

IPlatform &ServiceProvider::get_platform()
{
    return platform_instance;
}

ILogger &ServiceProvider::get_logger()
{
    return logger_instance;
}