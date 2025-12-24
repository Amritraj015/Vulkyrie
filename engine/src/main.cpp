#include "vulkyrie.h"
#include "platform/platform_factory.h"
#include "platform/application_manager.h"

#if defined(VULKYRIE_DEBUG)
// #include "core/logger.h"
//
// void *operator new(size_t size) {
//     VDEBUG("Allocating: %zu bytes", size);
//     return malloc(size);
// }
#endif

int main(int argc, char **argv) {
    // Detect the current platform.
    auto platform = Vulkyrie::Platform::DetectPlatform();

    // If the platform is not supported, return an unsupported platform error.
    if (nullptr == platform) {
        return std::to_underlying(Vulkyrie::Core::StatusCode::UnsupportedPlatform);
    }

    // Initialize the Vulkyrie Application.
    auto application = CreateApplication();

    // If the application is invalid, return an invalid application error.
    if (nullptr == application) {
        return std::to_underlying(Vulkyrie::Core::StatusCode::InvalidApplication);
    }

    // Create an instance of Application Manager.
    Vulkyrie::Platform::ApplicationManager applicationManager(platform, application);

    // Bootstrap the application.
    // This is a blocking call until the application is closed.
    auto status = applicationManager.BootstrapApplication();

    // Clean up the application instance.
    delete application;

    // Return success status code.
    return std::to_underlying(status);
}
