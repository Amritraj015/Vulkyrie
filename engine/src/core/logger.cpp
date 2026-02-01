#include "core/logger.h"
#include "console_log_sink.h"
#include "file_log_sink.h"

namespace Vulkyrie::Core {
    // Define the static member variable
    Scope<LogSink> Logger::_logSink = nullptr;

    StatusCode Logger::InitializeLogger(LoggerType loggerType) {
        switch (loggerType) {
            case LoggerType::Console:
                _logSink = CreateScope<ConsoleLogSink>();
                break;
            case LoggerType::File:
                _logSink = CreateScope<FileLogSink>();
                break;
            default:
                return StatusCode::UnsupportedLoggerType;
        }

        return _logSink->Initialize();
    }

} // namespace Vulkyrie::Core
