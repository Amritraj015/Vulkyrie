#pragma once

#include "status_codes.h"

namespace Vulkyrie::Core {
    class LogSink {
        public:
            virtual ~LogSink() = default;

            /** Initializes the logger. */
            virtual StatusCode Initialize() {
                return StatusCode::Successful;
            }

            virtual void LogMessage(const char *message);

            /** Terminates the logger. */
            virtual StatusCode Dispose() {
                return StatusCode::Successful;
            }
    };
}