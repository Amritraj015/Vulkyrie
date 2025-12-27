#pragma once

#include "status_codes.h"
#include "logger_type.h"
#include "log_level.h"
#include "log_sink.h"

// Enable log levels based on build configuration.
// FATAL and ERROR level logs are always enabled.
#if defined(VULKYRIE_DEBUG)
#define V_LOG_WARN_ENABLED
#define V_LOG_INFO_ENABLED
#define V_LOG_DEBUG_ENABLED
#define V_LOG_TRACE_ENABLED
#endif

namespace Vulkyrie::Core {
    class Logger {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            static StatusCode InitializeLogger(LoggerType loggerType);

            template<typename... Args>
            static void Log(LogLevel logLevel, std::string_view fmt, Args&&... args) {
                if (nullptr == _logSink) return;
                _logSink->LogMessage(logLevel, fmt, std::make_format_args(args...));
            }

            static void TerminateLogger();

        private:
            static std::unique_ptr<LogSink> _logSink;
    };
} // namespace Vulkyrie::Core

// Logs a fatal message.
#define VFATAL(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Fatal, fmt, ##__VA_ARGS__);

// Logs an error message.
#define VERROR(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Error, fmt, ##__VA_ARGS__);

// Logs a warning message if warning level logs are enabled else noop.
#if defined(V_LOG_WARN_ENABLED)
#define VWARN(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Warn, fmt, ##__VA_ARGS__);
#else
#define VWARN(fmt, ...)
#endif

// Logs an info message if info level logs are enabled else noop.
#if defined(V_LOG_INFO_ENABLED)
#define VINFO(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Info, fmt, ##__VA_ARGS__);
#else
#define VINFO(fmt, ...)
#endif

// Logs a debug message if debug level logs are enabled else noop.
#if defined(V_LOG_DEBUG_ENABLED)
#define VDEBUG(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Debug, fmt, ##__VA_ARGS__);
#else
#define VDEBUG(fmt, ...)
#endif

// Logs a trace message if trace level logs are enabled else noop.
#if defined(V_LOG_TRACE_ENABLED)
#define VTRACE(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Trace, fmt, ##__VA_ARGS__);
#else
#define VTRACE(fmt, ...)
#endif
