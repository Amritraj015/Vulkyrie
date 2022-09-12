#include "ApplicationManager.h"
#include "Platform/Platform.h"
#include "Core/Event/Keyboard/KeyEvent.h"
#include "Core/Event/Mouse/MouseMovedEvent.h"
#include "Core/Event/Mouse/MouseButtonEvent.h"
#include "Core/Event/Mouse/MouseScrolledEvent.h"

namespace Vkr
{
#define BIND_CALLBACK_FUNCTION(function) std::bind(&ApplicationManager::function, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)

    ApplicationManager::ApplicationManager(const std::shared_ptr<Platform> &platform)
    {
        mPlatform = platform;
    }

    bool ApplicationManager::OnKeyPress(const SenderType senderType, const ListenerType listenerType, Event *event)
    {
        auto ev = (KeyEvent *)event;
        VINFO("Key %s - KeyCode: '%c', Type: '%i'", ev->IsKeyPressed() ? "pressed": "released", ev->GetKeyCode(), ev->GetEventType())
        return true;
    }

    bool ApplicationManager::OnMouseButtonPress(const SenderType senderType, const ListenerType listenerType, Event *event)
    {
        auto ev = (MouseButtonEvent *)event;
        VINFO("Mouse Button: '%i' %s at (x: %i, y: %i)", ev->GetMouseButton(), ev->IsButtonPressed() ? "pressed" : "released", ev->GetMouseX(), ev->GetMouseY())
        return true;
    }

    bool ApplicationManager::OnMouseScrolled(const SenderType senderType, const ListenerType listenerType, Event *event)
    {
        auto ev = (MouseScrolledEvent *)event;
        VINFO("Mouse scrolled '%s' at: (x: %i, y: %i)", ev->GetDirection() ? "Up" : "Down", ev->GetXOffset(), ev->GetYOffset())
        return true;
    }

    bool ApplicationManager::OnMouseMoved(const SenderType senderType, const ListenerType listenerType, Event *event)
    {
        auto ev = (MouseMovedEvent *)event;
        VINFO("Mouse moved to: (x: %i, y: %i)", ev->GetX(), ev->GetY())
        return true;
    }
    
    bool ApplicationManager::OnWindowClose(const SenderType senderType, const ListenerType listenerType, Event *event)
    {
        mRunning = false;
        return true;
    }

    StatusCode ApplicationManager::InitializeSubsystems()
    {
        StatusCode statusCode = Logger::InitializeLogging();
        ENSURE_SUCCESS(statusCode, "An error occurred while initializing the logging system.")
		
		statusCode = EventSystemManager::RegisterEvent(EventType::KeyPressed, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnKeyPress));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `KeyPressed` event.")

		statusCode = EventSystemManager::RegisterEvent(EventType::KeyReleased, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnKeyPress));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `KeyReleased` event.")

		statusCode = EventSystemManager::RegisterEvent(EventType::MouseButtonPressed, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnMouseButtonPress));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `MouseButtonPressed` event.")

		statusCode = EventSystemManager::RegisterEvent(EventType::MouseButtonReleased, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnMouseButtonPress));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `MouseButtonReleased` event.")

		statusCode = EventSystemManager::RegisterEvent(EventType::MouseScrolled, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnMouseScrolled));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `MouseScrolled` event.")

		statusCode = EventSystemManager::RegisterEvent(EventType::MouseMoved, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnMouseMoved));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `MouseMoved` event.")

		statusCode = EventSystemManager::RegisterEvent(EventType::WindowClose, ListenerType::Application, BIND_CALLBACK_FUNCTION(OnWindowClose));
        ENSURE_SUCCESS(statusCode, "An error occurred while registering `WindowClose` event.")

		statusCode = mPlatform->CreateNewWindow(mpApp->name, mpApp->startX, mpApp->startY, mpApp->width, mpApp->height);
        ENSURE_SUCCESS(statusCode, "Error occurred while initializing platform.")

        // Renderer startup
        mpRendererClient = std::make_unique<RendererClient>();
        return mpRendererClient->Initialize(mPlatform, mpApp->rendererType, mpApp->name);
    }

    StatusCode ApplicationManager::TerminateSubsystems()
    {
        StatusCode statusCode = EventSystemManager::UnregisterAllEvents();
        ENSURE_SUCCESS(statusCode, "An error occurred while unregistering events.")

        statusCode = mpRendererClient->Terminate();
        ENSURE_SUCCESS(statusCode, "An error occurred while terminating the renderer.")

        statusCode = mPlatform->CloseWindow();
        ENSURE_SUCCESS(statusCode, "An error occurred while shutting down platform.")

        statusCode = Logger::ShutdownLogging();
        ENSURE_SUCCESS(statusCode, "An error occurred while shutting down the logging system.")

        return statusCode;
    }

    StatusCode ApplicationManager::InitializeApplication(Application *pApp)
    {
        if (mInitialized)
        {
            VERROR("Application has already been initialized")
            return StatusCode::AppAlreadyInitialized;
        }

        if (pApp == nullptr)
        {
            VERROR("A valid application instance is required for initialization")
            return StatusCode::InvalidApplicationInstance;
        }

        mpApp = pApp;
        mRunning = true;
        mSuspended = false;

        StatusCode statusCode = InitializeSubsystems();
		RETURN_ON_FAIL(statusCode)

        // Initialize client application.
        if (!mpApp->Initialize())
        {
            VFATAL("Game failed to initialize")
            return StatusCode::ClientAppInitializationFailed;
        }

        mpApp->OnResize(mWidth, mHeight);

        mInitialized = true;

        return statusCode;
    }

    StatusCode ApplicationManager::RunApplication()
    {
        if (!mInitialized)
        {
            VERROR("Please initialize the application before running.")
            return StatusCode::AppNotInitialized;
        }

        // Initialize Clock.
        mpCLock = std::make_unique<Clock>(mPlatform);
        mpCLock->Start();
        mpCLock->Update();

        mLastTime = mpCLock->GetStartTime();
        f64 runningTime = 0;
        u8 frameCount = 0;
        const f64 targetFrameSeconds = 1.0f / 60;

        while (mRunning)
        {
            if (!mPlatform->PollForEvents())
            {
                mRunning = false;
            }

            if (!mSuspended)
            {
                // Update clock and get delta time.
                mpCLock->Update();

                const f64 currentTime = mpCLock->GetElapsedTime();
                const f64 delta = currentTime - mLastTime;
                const f64 frameStartTime = mPlatform->GetAbsoluteTime();

                if (!mpApp->Update((f32)delta))
                {
                    VFATAL("Game update failed.")
                    mRunning = false;
                    break;
                }

                if (!mpApp->Render((f32)delta))
                {
                    VFATAL("Game render failed.")
                    mRunning = false;
                    break;
                }

                // TODO: refactor packet creation
                // render_packet packet;
                // packet.delta_time = delta;
                // renderer_draw_frame(&packet);

                // Figure out how long the frame took and, if below
                f64 frameEndTime = mPlatform->GetAbsoluteTime();
                f64 frameElapsedTime = frameEndTime - frameStartTime;
                runningTime += frameElapsedTime;
                f64 remainingSeconds = targetFrameSeconds - frameElapsedTime;

                if (remainingSeconds > 0)
                {
                    u64 remainingMilliSecs = (remainingSeconds * 1000);

                    // If there is time left, give it back to the OS.
                    // char limitFrames = false;

                    // if (remainingMilliSecs > 0 && limitFrames) {
                    if (remainingMilliSecs > 0)
                    {
                        mPlatform->SleepForDuration(remainingMilliSecs - 1);
                    }

                    frameCount++;
                }

                // Update last time
                mLastTime = currentTime;
            }
        }

        mRunning = false;

        return TerminateSubsystems();
    }
}