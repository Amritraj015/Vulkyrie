#include "vulkyrie.h"

#if defined(VULKYRIE_DEBUG)
// void *operator new(size_t size) {
//     VDEBUG("Allocating: {} bytes", size);
//     return std::malloc(size);
// }
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char **argv) {
    // Initialize the logger sub-system.
    auto statusCode = Vulkyrie::Core::Logger::InitializeLogger(Vulkyrie::Core::LoggerType::Console);

    // If logger initialization failed, return the error code.
    if (statusCode != Vulkyrie::Core::StatusCode::Successful) {
        return std::to_underlying(statusCode);
    }

    // Initialize the Vulkyrie Application.
    auto application = CreateApplication();

    // If the application is invalid, return an invalid application error.
    if (nullptr == application) {
        return std::to_underlying(Vulkyrie::Core::StatusCode::InvalidApplication);
    }

    // Run the application and get its status code.
    statusCode = application->Run();

    // Clean up the application instance.
    delete application;

    // Terminate the logger sub-system and return its status code.
    return std::to_underlying(statusCode);
}
