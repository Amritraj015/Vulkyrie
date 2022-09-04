#include <Vulkyrie.h>
#include "Defines.h"
#include "Core/Application/ApplicationManager.h"
#include "Platform/LinuxPlatform.h"
#include "Platform/PlatformWindows.h"

#if defined(_DEBUG)
// void *operator new(size_t size)
// {
//     VTRACE("Allocating: %llu bytes", size)
//     return malloc(size);
// }
#endif

#define CHECK_APPLICATION_STATUS(statusCode, message)         \
    if (statusCode != Vkr::StatusCode::Successful)            \
    {                                                         \
        const char *spErrorTemplate = "Status Code: %i - %s"; \
        VFATAL(spErrorTemplate, statusCode, message);         \
        return to_underlying(statusCode);                     \
    }

std::shared_ptr<Vkr::Platform> GetPlatform()
{
#if defined(VPLATFORM_LINUX)
    return std::make_shared<Vkr::LinuxPlatform>();
#elif defined(VPLATFORM_WINDOWS)
    return std::make_shared<Vkr::PlatformWindows>();
#endif
}

int main(int argc, char **argv)
{
    auto appManager = std::make_unique<Vkr::ApplicationManager>(GetPlatform());

    // Initialize the application.
    Vkr::StatusCode statusCode = appManager->InitializeApplication(GetApplication());
    CHECK_APPLICATION_STATUS(statusCode, "Failed to initialize the application!")

    // Run the application.
    statusCode = appManager->RunApplication();
    CHECK_APPLICATION_STATUS(statusCode, "Application did not shutdown gracefully!")

    return to_underlying(Vkr::StatusCode::Successful);
}
