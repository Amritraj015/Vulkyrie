#include "core/application_manager.h"
#include "core/logger.h"
#include "core/generic_window.h"

namespace Vulkyrie::Core {
    using Vulkyrie::Core::StatusCode;

    ApplicationManager *ApplicationManager::_instance = nullptr;

    ApplicationManager::ApplicationManager(const Vulkyrie::Core::Application &application)
        : _application(application), _window(std::make_shared<GenericWindow>(application)) {
        _instance = this;
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
        VINFO("*****************************************************************************************")
        VINFO("Application details")
        VINFO("*****************************************************************************************")
        VINFO("Application name              | {}", _application.windowProps.title)
        VINFO("Window Height requested       | {}", _application.windowProps.height)
        VINFO("Window Width requested        | {}", _application.windowProps.width)
        VINFO("Window Starting X position    | {}", _application.windowProps.startX)
        VINFO("Window Starting Y position    | {}", _application.windowProps.startY)
        VINFO("*****************************************************************************************")
        VINFO("Application configuration details")
        VINFO("*****************************************************************************************")
        VINFO("Graphics API                  | {}", std::to_underlying(_application.config.graphicsApi))
        VINFO("*****************************************************************************************")

        // Create the window for this application.
        StatusCode statusCode = _window->Create();

        // If window creation failed, then return an error.
        if (statusCode != StatusCode::Successful) {
            return statusCode;
        }

        // Else, return a successful response.
        return StatusCode::Successful;
    }

    void ApplicationManager::ToggleWireframeMode(bool enable) {
        _instance->_window->ToggleWireframeMode(enable);
    }

    StatusCode ApplicationManager::TerminateSubSystems() {
        // Close the application window.
        _window->Close();

        // Return a successful status code.
        return StatusCode::Successful;
    }
}; // namespace Vulkyrie::Core
