#pragma once
#include "Defines.h"

namespace Vkr
{
    enum class EventCategory
    {
        ApplicationEvent = BIT(0), // Application Event]]
        Input = BIT(1),            // Input event.
        Keyboard = BIT(2),         // Keyboard event.
        Mouse = BIT(3),            // Mouse event.
        MouseButton = BIT(4)       // Mouse Button event.
    };
}
