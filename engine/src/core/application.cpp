#include "core/status_codes.h"
#include "vlkypch.h"
#include "core/application.h"
#include "core/vulkyrie_glfw_platform.h"
#include "events/event_dispatcher.h"
#include "renderer/renderer.h"

namespace Vulkyrie::Core {

    Application *Application::_instance = nullptr;

    Application::Application(const WindowProps &windowProps)
        : _platform(CreateRef<VulkyrieGLFWPlatform>(this->_windowProps, [this](Vulkyrie::Events::Event &event) { this->OnEvent(event); }))
        , _windowProps(windowProps)
        , _running(false) {
        _instance = this;
    }

    StatusCode Application::Run() {
        // Create the application window.
        RETURN_ON_FAILURE(_platform->CreateWindow());

        // If window creation failed, return the status code.
        RETURN_ON_FAILURE(Vulkyrie::Renderer::Initialize(_windowProps.GraphicsAPI));

        VINFO("*****************************************************************************************")
        VINFO("Application details")
        VINFO("*****************************************************************************************")
        VINFO("Application name              | {}", _windowProps.Title)
        VINFO("Window Height requested       | {}", _windowProps.Height)
        VINFO("Window Width requested        | {}", _windowProps.Width)
        VINFO("*****************************************************************************************")
        VINFO("Application configuration details")
        VINFO("*****************************************************************************************")
        VINFO("Graphics API                  | {}", Vulkyrie::Renderer::GetCurrentGraphicsAPIName())
        VINFO("*****************************************************************************************")

        // Mark the application as running.
        // This is placed here to prevent the user from altering the layer stack before the application starts.
        _running = true;
        f32 lastFrameTime = 0.0f;

        // Raise the window created event.
        Vulkyrie::Events::WindowCreatedEvent event(_windowProps.Width, _windowProps.Height);
        OnInit(event);

        // Main application loop.
        while (_running) {
            VLKY_PROFILE_SCOPE("MainApplicationLoop");

            {
                VLKY_PROFILE_SCOPE("ApplicationLayerProcessQueuedOperations");

                // Process any pending layer operations.
                _layers.ProcessQueuedOperations();
            }

            {
                VLKY_PROFILE_SCOPE("ApplicationLayerUpdate");

                // Calculate the time since the last frame.
                const f32 time = _platform->GetTime();
                Timestep deltaTime(std::min(time - lastFrameTime, 0.1f)); // clamp MAX delta (100 ms)
                lastFrameTime = time;

                // Update each layer.
                for (const auto &layer : _layers) {
                    layer->OnUpdate(deltaTime);
                }
            }

            {
                VLKY_PROFILE_SCOPE("ApplicationWindowUpdate");

                // Update the application window.
                _platform->OnUpdate();
            }
        }

        // Return the status code.
        return StatusCode::Successful;
    }

    void Application::Stop() {
        _running = false;
    }

    void Application::OnEvent(Vulkyrie::Events::Event &event) {
        Vulkyrie::Events::EventDispatcher dispatcher(event);

        // If the event is a window close or resize event, handle it first.
        dispatcher.Dispatch<Vulkyrie::Events::WindowClosedEvent>([this](auto &e) -> bool { return this->OnWindowClosed(e); });
        dispatcher.Dispatch<Vulkyrie::Events::WindowResizedEvent>([this](auto &e) -> bool { return this->OnWindowResized(e); });

        for (const auto &_layer : std::ranges::reverse_view(_layers)) {
            // If the event has been handled, stop propagating.
            if (event.handled) {
                break;
            }

            // Else, pass the event to the layer.
            _layer->OnEvent(event);
        }
    }

    bool Application::OnWindowResized(const Vulkyrie::Events::WindowResizedEvent &event) {
        _windowProps.Height = event.Height;
        _windowProps.Width = event.Width;

        return false;
    }

    bool Application::OnWindowClosed([[maybe_unused]] Vulkyrie::Events::WindowClosedEvent &event) {
        Stop();

        return false;
    }

    bool Application::OnInit([[maybe_unused]] Vulkyrie::Events::WindowCreatedEvent &event) {
        return false;
    }
} // namespace Vulkyrie::Core
