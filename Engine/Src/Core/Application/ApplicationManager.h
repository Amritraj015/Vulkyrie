#pragma once

#include <Application/Application.h>

#include "Defines.h"
#include "Core/Event/Event.h"
#include "Core/Event/Registrar/EventSystemManager.h"
#include "Core/Clock/Clock.h"
#include "Renderers/RendererClient.h"

namespace Vkr
{
    class ApplicationManager
    {
    private:
        // The underlying platform.
        std::shared_ptr<Platform> mPlatform;

        // Represents if the Application is initialized or not.
        bool mInitialized = false;

        // Represents if the Application is suspended or not.
        bool mSuspended = true;

        // Represents if the Application is running or not.
        bool mRunning = false;

        // Width of the window.
        u16 width{};

        // Height of the window.
        u16 mHeight{};

        // Last time.
        f64 mLastTime{};

        // Application configuration.
        Application *mpApp{};

        // Renderer Client pointer.
        std::unique_ptr<RendererClient> mpRendererClient;

        // Clock instance.
        std::unique_ptr<Clock> mpCLock;

        // Initializes core subsystems for the engine.
        StatusCode InitializeSubsystems();

        // Terminates core subsystems for the engine.
        StatusCode TerminateSubsystems();

        // Event handler to handle key press events.
        static bool OnKeyPress(EventType eventType, SenderType senderType, ListenerType listenerType, Event *event);

        // Event handler to handle key release events.
        static bool OnKeyRelease(EventType eventType, SenderType senderType, ListenerType listenerType, Event *event);

        // Event handler to handle mouse button press events.
        static bool OnMouseButtonPress(EventType eventType, SenderType senderType, ListenerType listenerType, Event *event);

        // Event handler to handle mouse button release events.
        static bool OnMouseButtonRelease(EventType eventType, SenderType senderType, ListenerType listenerType, Event *event);

        // Event handler to handle mouse scroll events.
        static bool OnMouseScrolled(EventType eventType, SenderType senderType, ListenerType listenerType, Event *event);

        // Event handler to handle mouse move events.
        static bool OnMouseMoved(EventType eventType, SenderType senderType, ListenerType listenerType, Event *event);

    public:
        explicit ApplicationManager(const std::shared_ptr<Platform> &platform);
        DESTRUCTOR_LOG(ApplicationManager)

        // Initializes the application.
        StatusCode InitializeApplication(Application *pApp);

        // Runs the created application.
        StatusCode RunApplication();
    };
}
