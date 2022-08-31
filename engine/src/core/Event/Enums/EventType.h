#pragma once

namespace Vkr
{
    enum class EventType
    {
        WindowClose,         // Window close event.
        WindowResize,        // Window resize event.
        WindowFocus,         // Window focus event.
        WindowLostFocus,     // Window lost focus event.
        WindowMoved,         // Window moved event.
        AppTick,             // Application tick event.
        AppUpdate,           // Application update event.
        AppRender,           // Application render event.
        AppShutDown,         // Application shutdown event.
        KeyPressed,          // Keyboard button pressed event.
        KeyReleased,         // Keyboard button released event.
        MouseButtonPressed,  // Mouse button pressed event.
        MouseButtonReleased, // Mouse button released event.
        MouseMoved,          // Mouse moved event.
        MouseScrolled,       // Mouse scrolled event.
    };
}