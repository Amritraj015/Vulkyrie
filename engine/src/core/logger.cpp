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

    void Logger::Log(const char *message) {
        if (_logSink != nullptr) {
            _logSink->LogMessage(message);
        }
    }

    StatusCode Logger::TerminateLogger() {
        if (_logSink != nullptr) {
            return _logSink->Dispose();
        }

        return StatusCode::Successful;
    }
} // namespace Vulkyrie::Core