#include "core/application.h"
#include "core/generic_window.h"
#include "core/logger.h"
#include "core/graphics_api.h"
#include "events/event_dispatcher.h"
#include "platform/platform.h"

namespace Vulkyrie::Core {
    constexpr static std::string_view GetGraphicsApiName(GraphicsAPI api) {
        switch (api) {
            case GraphicsAPI::OpenGL:
                return "OpenGL";
            case GraphicsAPI::Vulkan:
                return "Vulkan";
            case GraphicsAPI::DirectX11:
                return "DirectX 11";
            case GraphicsAPI::DirectX12:
                return "DirectX 12";
            default:
                return "Unknown";
        }
    }

    Application::Application(const WindowProps &windowProps, const ApplicationConfig &config)
        : _windowProps(windowProps), _config(config),
          _window(std::make_shared<GenericWindow>(this->_windowProps, [this](Vulkyrie::Events::Event &event) { this->OnEvent(event); })),
          _lastFrameTime(0.0f), _running(false) {
    }

    Application::~Application() {
        // for (auto &layer : _layers) {
        //     layer->OnDetach();
        // }
    }

    StatusCode Application::Run() {
        // Create the application window.
        StatusCode statusCode = _window->Create();

        // If window creation failed, return the status code.
        if (StatusCode::Successful != statusCode) {
            return statusCode;
        }

        VINFO("*****************************************************************************************")
        VINFO("Application details")
        VINFO("*****************************************************************************************")
        VINFO("Application name              | {}", _windowProps.title)
        VINFO("Window Height requested       | {}", _windowProps.height)
        VINFO("Window Width requested        | {}", _windowProps.width)
        VINFO("Window Starting X position    | {}", _windowProps.startX)
        VINFO("Window Starting Y position    | {}", _windowProps.startY)
        VINFO("*****************************************************************************************")
        VINFO("Application configuration details")
        VINFO("*****************************************************************************************")
        VINFO("Graphics API                  | {}", GetGraphicsApiName(_config.graphicsApi))
        VINFO("*****************************************************************************************")

        // Mark the application as running.
        // This is placed here to prevent the user from altering the layer stack before the application starts.
        _running = true;

        // Raise the window created event.
        Vulkyrie::Events::WindowCreatedEvent event;
        OnEvent(event);

        // Main application loop.
        while (_running) {
            // Calculate the time since the last frame.
            const f32 time = Vulkyrie::Platform::GetTime();
            const Timestep deltaTime(time - _lastFrameTime);
            _lastFrameTime = time;

            // Update each layer.
            for (const auto &layer : _layers) {
                layer->OnUpdate(deltaTime);
            }

            // Update the application window.
            _window->OnUpdate();
        }

        // Close the application window.
        statusCode = _window->Close();

        // Return the status code.
        return statusCode;
    }

    void Application::Stop() {
        _running = false;
    }

    void Application::OnEvent(Vulkyrie::Events::Event &event) {
        Vulkyrie::Events::EventDispatcher dispatcher(event);

        // If the event is a window close or resize event, handle it first.
        dispatcher.Dispatch<Vulkyrie::Events::WindowCreatedEvent>([this](auto &e) -> bool { return this->OnInit(e); });
        dispatcher.Dispatch<Vulkyrie::Events::WindowClosedEvent>([this](auto &e) -> bool { return this->OnWindowClosed(e); });
        dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](auto &e) -> bool { return this->OnWindowResized(e); });

        for (auto it = _layers.rbegin(); it != _layers.rend(); ++it) {
            // If the event has been handled, stop propagating.
            if (event.handled) {
                break;
            }

            // Else, pass the event to the layer.
            (*it)->OnEvent(event);
        }
    }

    bool Application::OnWindowResized(Vulkyrie::Events::WindowResizedEvent &event) {
        return false;
    }

    bool Application::OnWindowClosed(Vulkyrie::Events::WindowClosedEvent &event) {
        Stop();

        return false;
    }

    bool Application::OnInit(Vulkyrie::Events::WindowCreatedEvent &event) {
        return false;
    }
} // namespace Vulkyrie::Core
