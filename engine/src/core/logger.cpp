#include "core/logger.h"
#include "console_log_sink.h"
#include "file_log_sink.h"

namespace Vulkyrie::Core {
    // Define the static member variable
    std::unique_ptr<LogSink> Logger::_logSink = nullptr;

    StatusCode Logger::InitializeLogger(LoggerType loggerType) {
        switch (loggerType) {
        case LoggerType::Console:
            _logSink = std::make_unique<ConsoleLogSink>();
            break;
        case LoggerType::File:
            _logSink = std::make_unique<FileLogSink>();
            break;
        default:
            return StatusCode::UnsupportedLoggerType;
        }

        return _logSink->Initialize();
    }

    void Logger::Log(LogLevel logLevel, const char *fmt, ...) {
        if (nullptr == _logSink) return;

        va_list args;
        va_start(args, fmt);
        _logSink->LogMessage(logLevel, fmt, args);
        va_end(args);
    }

    StatusCode Logger::TerminateLogger() {
        if (_logSink != nullptr) {
            return _logSink->Dispose();
        }

        return StatusCode::Successful;
    }
} // namespace Vulkyrie::Core
