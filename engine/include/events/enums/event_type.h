#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Events {
    enum class EventType : u32 {
        Unknown = 0,         // Unknown event.
        WindowCreated,       // Window created event.
        WindowClosed,        // Window close event.
        WindowResized,       // Window resize event.
        WindowFocused,       // Window focus event.
        WindowLostFocus,     // Window lost focus event.
        WindowMoved,         // Window moved event.
        AppTick,             // Application tick event.
        AppUpdate,           // Application update event.
        AppRender,           // Application render event.
        AppShutDown,         // Application shutdown event.
        KeyPressed,          // Keyboard button pressed event.
        KeyReleased,         // Keyboard button released event.
        KeyChar,             // Keyboard button typed event.
        MouseButtonPressed,  // Mouse button pressed event.
        MouseButtonReleased, // Mouse button released event.
        MouseMoved,          // Mouse moved event.
        MouseScrolled,       // Mouse scrolled event.
    };
}
