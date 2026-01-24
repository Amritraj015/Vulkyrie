#include "file_log_sink.h"

namespace Vulkyrie::Core {
    FileLogSink::~FileLogSink() {
        if (nullptr != _logFile) {
            std::fflush(_logFile);
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

    static constexpr std::string_view fileLogPrefixes[] = { "[FATAL]: ", "[ERROR]: ", "[WARN]: ", "[INFO]: ", "[DEBUG]: ", "[TRACE]: " };

    void FileLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        if (nullptr == _logFile) return;

        char buffer[LOG_BUFFER_SIZE];

        const std::string_view logPrefix = fileLogPrefixes[static_cast<size_t>(logLevel)];

        const auto it = std::format_to(buffer, "{}: ", logPrefix);
        const auto result = std::vformat_to(it, fmt, args);

        std::fwrite(buffer, 1, result - buffer, _logFile);
        std::fputc('\n', _logFile);
    }

    void FileLogSink::Dispose() {
        if (nullptr != _logFile) {
            std::fflush(_logFile);
            std::fclose(_logFile);
        }

        delete _logFile;
        _logFile = nullptr;
    }
} // namespace Vulkyrie::Core
