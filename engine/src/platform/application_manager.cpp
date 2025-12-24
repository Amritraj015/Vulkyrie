#include "application_manager.h"
#include "core/logger.h"

namespace Vulkyrie::Platform {
    ApplicationManager::ApplicationManager(PlatformBase *platform, VulkyrieApplication *application)
        : _platform(platform), _application(application) {
    }

    StatusCode ApplicationManager::BootstrapApplication() {
        StatusCode statusCode = InitializeSubSystems();

        // Return if any sub-system failed to initialize.
        if (statusCode != StatusCode::Successful) {
            return statusCode;
        }

        return TerminateSubSystems();
    }

    StatusCode ApplicationManager::InitializeSubSystems() {
        // Initialize the logger sub-system.
        StatusCode statusCode = Vulkyrie::Core::Logger::InitializeLogger(_application->config.loggerType);

        if (statusCode != StatusCode::Successful) {
            return statusCode;
        }

        VINFO("*****************************************************************************************")
        VINFO("Application details")
        VINFO("*****************************************************************************************")
        VINFO("Application name              | {}", _application->windowProps.title)
        VINFO("Window Height requested       | {}", _application->windowProps.height)
        VINFO("Window Width requested        | {}", _application->windowProps.width)
        VINFO("Window Starting X position    | {}", _application->windowProps.startX)
        VINFO("Window Starting Y position    | {}", _application->windowProps.startY)
        VINFO("*****************************************************************************************")
        VINFO("Application configuration details")
        VINFO("*****************************************************************************************")
        VINFO("Logger Type                   | {}", std::to_underlying(_application->config.loggerType))
        VINFO("Graphics API                  | {}", std::to_underlying(_application->config.graphicsApi))
        VINFO("*****************************************************************************************")

        // Create the window for this application.
        statusCode = _platform->CreateNewWindow(_application->windowProps);

        // If window creation failed, then return an error.
        if (statusCode != StatusCode::Successful) {
            return statusCode;
        }

        // Else, return a successful response.
        return StatusCode::Successful;
    }

    StatusCode ApplicationManager::TerminateSubSystems() {
        // Close the application window.
        _platform->CloseWindow();

        // Terminate the logger sub-system.
        return Vulkyrie::Core::Logger::TerminateLogger();
    }
}; // namespace Vulkyrie::Platform
