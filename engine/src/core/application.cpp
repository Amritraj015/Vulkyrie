#include "vlkypch.h"
#include "core/application.h"
#include "core/vulkyrie_glfw_platform.h"
#include "events/event_dispatcher.h"

namespace Vulkyrie::Core {

    Application *Application::_instance = nullptr;

    Application::Application(const WindowProps &windowProps)
        : _platform(CreateRef<VulkyrieGLFWPlatform>(this->_windowProps, [this](Vulkyrie::Events::Event &event) { this->OnEvent(event); }))
        , _windowProps(windowProps)
        , _running(false)
        , _lastFrameTime(0.0f) {
        _instance = this;
    }

    Application::~Application() {
        // for (auto &layer : _layers) {
        //     layer->OnDetach();
        // }
    }

    StatusCode Application::Run() {
        // Create the application window.
        StatusCode statusCode = _platform->CreateWindow();

        // If window creation failed, return the status code.
        if (StatusCode::Successful != statusCode) {
            return statusCode;
        }

        Vulkyrie::Renderer::Initialize(_windowProps.GraphicsApi);

        VINFO("*****************************************************************************************")
        VINFO("Application details")
        VINFO("*****************************************************************************************")
        VINFO("Application name              | {}", _windowProps.Title)
        VINFO("Window Height requested       | {}", _windowProps.Height)
        VINFO("Window Width requested        | {}", _windowProps.Width)
        VINFO("Window Starting X position    | {}", _windowProps.StartX)
        VINFO("Window Starting Y position    | {}", _windowProps.StartY)
        VINFO("*****************************************************************************************")
        VINFO("Application configuration details")
        VINFO("*****************************************************************************************")
        VINFO("Graphics API                  | {}", Vulkyrie::Renderer::GetCurrentGraphicsAPIName())
        VINFO("*****************************************************************************************")

        // Mark the application as running.
        // This is placed here to prevent the user from altering the layer stack before the application starts.
        _running = true;

        // Raise the window created event.
        Vulkyrie::Events::WindowCreatedEvent event(_windowProps.Width, _windowProps.Height);
        OnInit(event);

        // Main application loop.
        while (_running) {
            // Process any pending layer operations.
            _layers.ProcessQueuedOperations();

            // Calculate the time since the last frame.
            const f32 time = _platform->GetTime();
            Timestep deltaTime(std::min(time - _lastFrameTime, 0.1f)); // clamp MAX delta (100 ms)
            _lastFrameTime = time;

            // Update each layer.
            for (const auto &layer : _layers) {
                layer->OnUpdate(deltaTime);
            }

            // Update the application window.
            _platform->OnUpdate();
        }

        // TODO: The following causes a segfault in OpenGLVertexArray class's destructor,
        // TODO: This needs to happen after all other openGL resources have been cleaned up.
        // Close the application window.
        statusCode = _platform->Close();

        // Return the status code.
        return statusCode;
    }

    void Application::Stop() { _running = false; }

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
        const auto resizeEvent = static_cast<Vulkyrie::Events::WindowResizedEvent>(event);

        _windowProps.Height = resizeEvent.Height;
        _windowProps.Width = resizeEvent.Width;

        return false;
    }

    bool Application::OnWindowClosed([[maybe_unused]] Vulkyrie::Events::WindowClosedEvent &event) {
        Stop();

        return false;
    }

    bool Application::OnInit([[maybe_unused]] Vulkyrie::Events::WindowCreatedEvent &event) { return false; }
} // namespace Vulkyrie::Core
