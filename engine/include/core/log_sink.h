#pragma once

#include "status_codes.h"
#include "core/log_level.h"
#include "defines.h"

namespace Vulkyrie::Core {
    class LogSink {
        public:
            virtual ~LogSink() = default;

            /** Initializes the logger. */
            virtual StatusCode Initialize() {
                return StatusCode::Successful;
            }

            virtual void LogMessage(LogLevel logLevel, const char *fmt, va_list args);

            /** Terminates the logger. */
            virtual StatusCode Dispose() {
                return StatusCode::Successful;
            }

        protected:
            static constexpr u16 LOG_BUFFER_SIZE = 4096;
    };
} // namespace Vulkyrie::Core
