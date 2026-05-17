#pragma once
#include <string>

namespace VCLI {
    struct VulkyCliArguments {
    public:
        bool showHelp = false;
        bool showVersion = false;
        bool verboseLogging = false;
        std::string configFilePath = "N/A";
    };
} // namespace VCLI
