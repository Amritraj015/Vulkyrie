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

    VLKY_PROFILE_BEGIN_SESSION("Game Loop", "profile_results.json");

    // Run the application and get its status code.
    statusCode = application->Run();

    VLKY_PROFILE_END_SESSION();

    // Terminate the logger sub-system and return its status code.
    return std::to_underlying(statusCode);
}
