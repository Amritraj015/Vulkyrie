#pragma once

#include "core/status_codes.h"
#include "core/logger_type.h"
#include "core/log_level.h"
#include "core/log_sink.h"

#if not defined(VULKYRIE_LOG_LEVEL)
#define VULKYRIE_LOG_LEVEL VULKYRIE_ERROR_LEVEL_LOG
#endif

namespace Vulkyrie::Core {
    class Logger {
        public:
            // Deleted copy constructor and assignment operator to prevent copies.
            Logger(const Logger &) = delete;
            Logger &operator=(const Logger &) = delete;

            static StatusCode InitializeLogger(LoggerType loggerType);

            template <typename... Args> static void Log(LogLevel logLevel, std::string_view fmt, Args &&...args) {
                if (nullptr == _logSink) return;
                _logSink->LogMessage(logLevel, fmt, std::make_format_args(args...));
            }

        private:
            static Scope<LogSink> _logSink;
    };
} // namespace Vulkyrie::Core

// clang-format off

// Logs a fatal message.
#define VFATAL(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Fatal, fmt __VA_OPT__(,) __VA_ARGS__);

// Logs an error message.
#define VERROR(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Error, fmt __VA_OPT__(,) __VA_ARGS__);

// Logs a warning message if warning level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_WARN_LEVEL_LOG
#define VWARN(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Warn, fmt __VA_OPT__(,) __VA_ARGS__);
#else
#define VWARN(fmt, ...)
#endif

// Logs an info message if info level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_INFO_LEVEL_LOG
#define VINFO(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Info, fmt __VA_OPT__(,) __VA_ARGS__);
#else
#define VINFO(fmt, ...)
#endif

// Logs a debug message if debug level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_DEBUG_LEVEL_LOG
#define VDEBUG(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Debug, fmt __VA_OPT__(,) __VA_ARGS__);
#else
#define VDEBUG(fmt, ...)
#endif

// Logs a trace message if trace level logs are enabled else noop.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_TRACE_LEVEL_LOG
#define VTRACE(fmt, ...) Vulkyrie::Core::Logger::Log(Vulkyrie::Core::LogLevel::Trace, fmt __VA_OPT__(,) __VA_ARGS__);
#else
#define VTRACE(fmt, ...)
#endif

// clang-format on
