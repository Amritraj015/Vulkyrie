#include <Vulkyrie.h>
#include "Defines.h"
#include "Core/Application/ApplicationManager.h"

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

int main(int argc, char **argv)
{
    std::unique_ptr<Vkr::ApplicationManager> appManager = std::make_unique<Vkr::ApplicationManager>();

    // Initialize the application.
    Vkr::StatusCode statusCode = appManager->InitializeApplication(GetApplication());
    CHECK_APPLICATION_STATUS(statusCode, "Failed to initialize the application!")

    // Run the application.
    statusCode = appManager->RunApplication();
    CHECK_APPLICATION_STATUS(statusCode, "Application did not shutdown gracefully!")

    return to_underlying(Vkr::StatusCode::Successful);
}