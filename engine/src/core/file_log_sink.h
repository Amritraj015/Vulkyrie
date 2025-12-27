#pragma once

#include "core/log_sink.h"
#include <cstdio>

namespace Vulkyrie::Core {
    class FileLogSink final : public LogSink {
        public:
            StatusCode Initialize() override;
            void LogMessage(LogLevel logLevel, std::string_view fmt, std::format_args args) override;
            void Dispose() override;
            ~FileLogSink() override;

        private:
            FILE *_logFile;
    };
} // namespace Vulkyrie::Core
