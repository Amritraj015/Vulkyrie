#pragma once

#include "defines.h"
#include "core/utilities.h"

namespace Vulkyrie::Events {
    enum class EventCategory : u8 {
        ApplicationEvent = BIT(0), // Application Event.
        Input = BIT(1),            // Input event.
        Keyboard = BIT(2),         // Keyboard event.
        Mouse = BIT(3),            // Mouse event.
        MouseButton = BIT(4),      // Mouse Button event.
    };
}
