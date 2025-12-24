#pragma once

#include "defines.h"

namespace Vulkyrie::Events {
    enum class EventType : u8 {
        None = 0,                 // No event.
        WindowClose = 1,          // Window close event.
        WindowResize = 2,         // Window resize event.
        WindowFocus = 3,          // Window focus event.
        WindowLostFocus = 4,      // Window lost focus event.
        WindowMoved = 5,          // Window moved event.
        AppTick = 6,              // Application tick event.
        AppUpdate = 7,            // Application update event.
        AppRender = 8,            // Application render event.
        AppShutDown = 9,          // Application shutdown event.
        KeyPressed = 10,          // Keyboard button pressed event.
        KeyReleased = 11,         // Keyboard button released event.
        MouseButtonPressed = 12,  // Mouse button pressed event.
        MouseButtonReleased = 13, // Mouse button released event.
        MouseMoved = 14,          // Mouse moved event.
        MouseScrolled = 15,       // Mouse scrolled event.
    };
}