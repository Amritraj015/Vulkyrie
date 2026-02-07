#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Events {
    enum class EventCategory : u32 {
        ApplicationEvent = 1 << 0, // Application Event.
        Input = 1 << 1,            // Input event.
        Keyboard = 1 << 2,         // Keyboard event.
        Mouse = 1 << 3,            // Mouse event.
        MouseButton = 1 << 4,      // Mouse Button event.
    };
}
