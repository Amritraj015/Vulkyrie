#pragma once

#include "core/log_sink.h"
#include <cstdio>

namespace Vulkyrie::Core {
    class FileLogSink final : public LogSink {
        public:
            StatusCode Initialize() override;
            void LogMessage(const char *message) override;
            StatusCode Dispose() override;
            ~FileLogSink() override;

        private:
            FILE *_logFile;
    };
}