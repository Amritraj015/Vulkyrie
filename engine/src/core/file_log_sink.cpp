#include "core/file_log_sink.h"

namespace Vulkyrie {
    FileLogSink::~FileLogSink() {
        if (nullptr != _logFile) {
            std::fflush(_logFile);
            std::fclose(_logFile);
        }
    }

    StatusCode FileLogSink::Initialize() {
#if defined(VE_PLATFORM_WINDOWS)
        fopen_s(&_logFile, "vulkyrie_log.txt", "a");
#else
        _logFile = std::fopen("vulkyrie_log.txt", "a");
#endif

        if (nullptr == _logFile) {
            return StatusCode::FailedToInitializeLogger;
        }

        return StatusCode::Successful;
    }

    static constexpr std::string_view fileLogPrefixes[] = { "[FATAL]:", "[ERROR]:", "[WARN]:", "[INFO]:", "[DEBUG]:", "[TRACE]:" };

    void FileLogSink::LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) {
        if (nullptr == _logFile) return;

        char buffer[LOG_BUFFER_SIZE];

        const std::string_view logPrefix = fileLogPrefixes[static_cast<size_t>(logLevel)];

        const auto it = std::format_to(buffer, "{} ", logPrefix);
        const auto result = std::vformat_to(it, fmt, args);

        std::fwrite(buffer, 1, result - buffer, _logFile);
        std::fputc('\n', _logFile);
    }
} // namespace Vulkyrie
