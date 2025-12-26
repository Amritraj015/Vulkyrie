#include "vulkyrie.h"
#include "platform/application_manager.h"

#if defined(VULKYRIE_DEBUG)
// #include "core/logger.h"

// void *operator new(size_t size) {
//     VDEBUG("Allocating: %zu bytes", size);
//     return malloc(size);
// }
#endif

int main(int argc, char **argv) {
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

    // Create an instance of Application Manager.
    Vulkyrie::Platform::ApplicationManager applicationManager(*application);

    // Bootstrap the application.
    // This is a blocking call until the application is closed.
    auto status = applicationManager.BootstrapApplication();

    // Clean up the application instance.
    delete application;

    // Terminate the logger sub-system and return its status code.
    return std::to_underlying(Vulkyrie::Core::Logger::TerminateLogger());
}
