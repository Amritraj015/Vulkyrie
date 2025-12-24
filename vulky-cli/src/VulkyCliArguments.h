#pragma once

namespace VulkyCli {
    struct VulkyCliArguments {
        bool showHelp = false;
        bool showVersion = false;
        bool verboseLogging = false;
        const char* configFilePath = nullptr;
    };
}