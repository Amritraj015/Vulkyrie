#pragma once

#include "defines.h"
#include "log_level.hpp"

class ILogger
{
public:
    virtual b8 initialize_logging() = 0;
    virtual void shutdown_logging() = 0;
    static void log_output(log_level level, const char *message, ...);
};

// Logs a fatal-level message.
#define V_FATAL(message, ...) ILogger::log_output(LOG_LEVEL_FATAL, message, ##__VA_ARGS__);

#ifndef V_ERROR
// Logs an error-level message.
#define V_ERROR(message, ...) ILogger::log_output(LOG_LEVEL_ERROR, message, ##__VA_ARGS__);
#endif

#if LOG_WARN_ENABLED == 1
// Logs a warning-level message.
#define V_WARN(message, ...) ILogger::log_output(LOG_LEVEL_WARN, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_WARN_ENABLED != 1
#define V_WARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
/* Logs a info-level message. */
#define V_INFO(message, ...) ILogger::log_output(LOG_LEVEL_INFO, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_INFO_ENABLED != 1
#define V_INFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug-level message.
#define V_DEBUG(message, ...) ILogger::log_output(LOG_LEVEL_DEBUG, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_DEBUG_ENABLED != 1
#define V_DEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Logs a trace-level message.
#define V_TRACE(message, ...) ILogger::log_output(LOG_LEVEL_TRACE, message, ##__VA_ARGS__);
#else
// Does nothing when LOG_TRACE_ENABLED != 1
#define V_TRACE(message, ...)
#endif
