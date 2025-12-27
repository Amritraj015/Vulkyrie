#include "file_log_sink.h"

namespace Vulkyrie::Core {
    FileLogSink::~FileLogSink() {
        if (nullptr != _logFile) {
            std::fclose(_logFile);
        }

        delete _logFile;
        _logFile = nullptr;
    };

    StatusCode FileLogSink::Initialize() {
        _logFile = std::fopen("vulkyrie_log.txt", "a");

        if (nullptr == _logFile) {
            return StatusCode::FailedToInitializeLogger;
        }

        return StatusCode::Successful;
    }

    void FileLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        if (nullptr == _logFile) return;

        char buffer[LOG_BUFFER_SIZE];

        std::string_view logPrefix;
        switch (logLevel) {
        case LogLevel::Fatal:
            logPrefix = "FATAL";
            break;
        case LogLevel::Error:
            logPrefix = "ERROR";
            break;
        case LogLevel::Warn:
            logPrefix = "WARN";
            break;
        case LogLevel::Info:
            logPrefix = "INFO";
            break;
        case LogLevel::Debug:
            logPrefix = "DEBUG";
            break;
        case LogLevel::Trace:
            logPrefix = "TRACE";
            break;
        default:
            logPrefix = "UNKNOWN";
            break;
        }

        auto it = std::format_to(buffer, "[{}]: ", logPrefix);
        auto result = std::vformat_to(it, fmt, args);

        std::fwrite(buffer, 1, result - buffer, _logFile);
        std::fputc('\n', _logFile);
        std::fflush(_logFile);
    }

    void FileLogSink::Dispose() {
        if (nullptr != _logFile) {
            std::fclose(_logFile);
        }

        delete _logFile;
        _logFile = nullptr;
    }
} // namespace Vulkyrie::Core
