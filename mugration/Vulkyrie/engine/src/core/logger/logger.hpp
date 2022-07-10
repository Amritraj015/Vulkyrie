#pragma once

#include "ilogger.hpp"

class Logger : public ILogger
{
    virtual b8 initialize_logging() override;
    virtual void shutdown_logging() override;
};
