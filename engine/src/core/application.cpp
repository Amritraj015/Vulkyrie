#include "vlkypch.h"
#include "core/status_codes.h"
#include "core/application.h"
#include "core/vulkyrie_glfw_platform.h"
#include "events/application/window_closed_event.h"
#include "events/event_dispatcher.h"

namespace Vulkyrie {

    Application *Application::sInstance = nullptr;

    Application::Application(const ApplicationSettings &appSettings)
        : mAppSettings(appSettings)
        , mRunning(false) {

        const WindowProps windowProps = {
            .Height = appSettings.GraphicsSettings.WindowHeight,
            .Width = appSettings.GraphicsSettings.WindowWidth,
            .Title = appSettings.Name,
            .EnableVSync = appSettings.GraphicsSettings.EnableVSync,
            .GraphicsAPI = appSettings.GraphicsSettings.API,
        };

        mPlatform = CreateScope<VulkyrieGLFWPlatform>(windowProps, [this](Event &event) { this->OnEvent(event); });

        sInstance = this;
    }

    StatusCode Application::Run() {
        // Try to create the application window, if it fails, return the status code.
        RETURN_ON_FAILURE(mPlatform->CreateWindow());

        VINFO("*****************************************************************************************");
        VINFO("Application details");
        VINFO("*****************************************************************************************");
        VINFO("Name                 | {}", mAppSettings.Name);
        VINFO("Window Dimensions    | {} x {}", mAppSettings.GraphicsSettings.WindowHeight, mAppSettings.GraphicsSettings.WindowWidth);
        VINFO("Enable V-Sync        | {}", mAppSettings.GraphicsSettings.EnableVSync);
        VINFO("*****************************************************************************************");

        // Try to initialzed the renderer.
        // TODO: Create and pass the actual device creation info.
        mRenderer = Renderer::Create(mAppSettings.GraphicsSettings.API, {});

        // Return an error status code if the renderer context failed to initialized.
        if (!mRenderer->ContextCreated()) {
            return StatusCode::FailedToInitializeRendererContext;
        }

        // Mark the application as running.
        mRunning = true;
        f32 lastFrameTime = 0.0F;

        // Raise the window created event.
        WindowCreatedEvent event(mAppSettings.GraphicsSettings.WindowWidth, mAppSettings.GraphicsSettings.WindowHeight);
        OnInit(event);

        // Main application loop.
        while (mRunning) {
            VLKY_PROFILE_SCOPE("MainApplicationLoop");

            {
                VLKY_PROFILE_SCOPE("ApplicationLayerProcessQueuedOperations");

                // Process any pending layer operations.
                mLayers.ProcessQueuedOperations();
            }

            {
                VLKY_PROFILE_SCOPE("ApplicationLayerUpdate");

                // Calculate the time since the last frame.
                const f32 time = mPlatform->GetTime();
                Timestep deltaTime(std::min(time - lastFrameTime, 0.1F)); // clamp MAX delta (100 ms)
                lastFrameTime = time;

                // Update each layer.
                for (const auto &layer : mLayers) {
                    layer->OnUpdate(deltaTime);
                }
            }

            {
                VLKY_PROFILE_SCOPE("ApplicationRender");

                // Render the application.
                mRenderer->Render();
            }

            {
                // TODO: May need to remove this.
                // TODO: Probably not needed for Vulkan.
                VLKY_PROFILE_SCOPE("ApplicationWindowUpdate");

                // Update the application window.
                mPlatform->OnUpdate();
            }
        }

        // Return the status code.
        return StatusCode::Successful;
    }

    void Application::Stop() {
        mRunning = false;
    }

    void Application::OnEvent(Event &event) {
        EventDispatcher dispatcher(event);

        // Propagate the event through the layers in reverse order (from top to bottom).
        for (const auto &layer : std::ranges::reverse_view(mLayers)) {
            // If the event has been handled, stop propagating.
            if (event.handled) {
                break;
            }

            // Else, pass the event to the layer.
            layer->OnEvent(event);
        }

        dispatcher.Dispatch<WindowResizedEvent>([this](auto &e) -> bool { return this->OnWindowResized(e); });

        // If the window closed event is dispatched, stop the application.
        dispatcher.Dispatch<WindowClosedEvent>([this]([[maybe_unused]] auto &e) -> bool {
            Stop();
            return false;
        });
    }

    bool Application::OnWindowResized(const WindowResizedEvent &event) {
        mAppSettings.GraphicsSettings.WindowHeight = event.Height;
        mAppSettings.GraphicsSettings.WindowWidth = event.Width;

        mRenderer->OnWindowResize(event.Width, event.Height);

        return false;
    }

    bool Application::OnInit([[maybe_unused]] WindowCreatedEvent &event) {
        return false;
    }

} // namespace Vulkyrie
