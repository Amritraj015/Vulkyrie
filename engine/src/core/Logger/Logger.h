#pragma once

#include "Defines.h"
#include "LogLevel.h"

namespace Vkr {

#if defined(_DEBUG)
#define LOG_WARN_ENABLED 1
#define LOG_INFO_ENABLED 1
#define LOG_DEBUG_ENABLED 1
#define LOG_TRACE_ENABLED 1
#else
	// Disable debug and trace logging for release builds.
#define LOG_DEBUG_ENABLED 0
#define LOG_TRACE_ENABLED 0
#endif

	StatusCode InitializeLogging();

	StatusCode ShutdownLogging();

	VAPI void LogOutput(LogLevel level, const char *message, ...);

// Logs a fatal-level message.
#define VFATAL(message, ...) LogOutput(Vkr::LogLevel::Fatal, message, ##__VA_ARGS__);

// Logs an error-level message.
#define VERROR(message, ...) LogOutput(Vkr::LogLevel::Error, message, ##__VA_ARGS__);

#if LOG_WARN_ENABLED == 1
// Logs a warning-level message.
#define VWARN(message, ...) LogOutput(Vkr::LogLevel::Warn, message, ##__VA_ARGS__);
#else
	// Does nothing when LOG_WARN_ENABLED != 1
#define VWARN(message, ...)
#endif

#if LOG_INFO_ENABLED == 1
/* Logs a info-level message. */
#define VINFO(message, ...) LogOutput(Vkr::LogLevel::Info, message, ##__VA_ARGS__);
#else
	// Does nothing when LOG_INFO_ENABLED != 1
#define VINFO(message, ...)
#endif

#if LOG_DEBUG_ENABLED == 1
// Logs a debug-level message.
#define VDEBUG(message, ...) LogOutput(Vkr::LogLevel::Debug, message, ##__VA_ARGS__);
#else
	// Does nothing when LOG_DEBUG_ENABLED != 1
#define VDEBUG(message, ...)
#endif

#if LOG_TRACE_ENABLED == 1
// Logs a trace-level message.
#define VTRACE(message, ...) LogOutput(Vkr::LogLevel::Trace, message, ##__VA_ARGS__);

#define VCREATE(instance) VDEBUG("%s - Creating instance!", instance);
#define VDESTROY(instance) VDEBUG("%s - Terminating instance!", instance);

#define CONSTRUCTOR_LOG(instance) \
    instance()                    \
    {                             \
        VCREATE(#instance)        \
    }

#define DESTRUCTOR_LOG(instance) \
    ~instance()                  \
    {                            \
        VDESTROY(#instance)      \
    }
#else
	// Does nothing when LOG_TRACE_ENABLED != 1
#define VTRACE(message, ...)
#define VCREATE(instance)
#define VDESTROY(instance)

#define CONSTRUCTOR_LOG(instance) \
	instance() {}

#define DESTRUCTOR_LOG(instance) \
	~instance() {}
#endif
}
