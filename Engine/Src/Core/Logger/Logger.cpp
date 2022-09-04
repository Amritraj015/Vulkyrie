#include "Logger.h"


namespace Vkr
{
    StatusCode Logger::InitializeLogging()
    {
        // TODO: create log file.
        VCREATE("Logger");
        return StatusCode::Successful;
    }

    StatusCode Logger::ShutdownLogging()
    {
        // TODO: cleanup logging/write queued entries.
        VDESTROY("Logger");
        return StatusCode::Successful;
    }

    void Logger::LogOutput(LogLevel level, const char *message, ...)
    {
        const char *levelStrings[6] = {"[FATAL]: ", "[ERROR]: ", "[WARN]:  ", "[INFO]:  ", "[DEBUG]: ", "[TRACE]: "};
        const char *colorStrings[6] = {"0;41", "1;31", "1;33", "1;32", "1;34", "1;30"};

        // Technically imposes a 32k character limit on a single log entry, but...
        // DON'T DO THAT!
        char outMessage[32000];
        char outMessage2[32000];
        memset(outMessage, 0, sizeof(outMessage));

        // Format original message.
        // NOTE: Oddly enough, MS's headers override the GCC/Clang va_list type with a "typedef char* va_list" in some
        // cases, and as a result throws a strange error here. The workaround for now is to just use __builtin_va_list,
        // which is the type GCC/Clang's va_start expects.
        __builtin_va_list argPtr;
        va_start(argPtr, message);
        vsnprintf(outMessage, 32000, message, argPtr);
        va_end(argPtr);

        const u8 levelIndex = to_underlying(level);
        sprintf(outMessage2, "%s%s\n", levelStrings[levelIndex], outMessage);

        // FATAL,ERROR,WARN,INFO,DEBUG,TRACE

        printf("\033[%sm%s\033[0m", colorStrings[levelIndex], outMessage2);
    }
}