#include <iostream>
#include "vulky_cli_arguments.h"

int main(int argc, char **argv) {
    VCLI::VulkyCliArguments cliArgs;

    std::cout << "Vulky CLI Arguments:" << std::endl;
    std::cout << "\tShow Help: " << (cliArgs.showHelp ? "true" : "false") << std::endl;
    std::cout << "\tShow Version: " << (cliArgs.showVersion ? "true" : "false") << std::endl;
    std::cout << "\tVerbose Logging: " << (cliArgs.verboseLogging ? "true" : "false") << std::endl;
    std::cout << "\tConfig File Path: " << (cliArgs.configFilePath.c_str() ? cliArgs.configFilePath : "nullptr") << std::endl;

    return 0;
}
