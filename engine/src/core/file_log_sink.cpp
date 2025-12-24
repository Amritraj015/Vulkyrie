#include <print>
#include "file_log_sink.h"

namespace Vulkyrie::Core {
    FileLogSink::~FileLogSink() {
        if (nullptr != _logFile) {
            std::fclose(_logFile);
        }

        delete _logFile;
    };

    StatusCode FileLogSink::Initialize() {
        _logFile = std::fopen("vulkyrie_log.txt", "a");

        if (nullptr == _logFile) {
            return StatusCode::FailedToInitializeLogger;
        }

        return StatusCode::Successful;
    }

    void FileLogSink::LogMessage(const char *message) {
        std::println(_logFile, "{}", message);
    }

    StatusCode FileLogSink::Dispose() {
        if (nullptr != _logFile) {
            std::fclose(_logFile);
        }

        return StatusCode::Successful;
    }
} // namespace Vulkyrie::Core
