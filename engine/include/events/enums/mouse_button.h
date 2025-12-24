#pragma once

#include "defines.h"

namespace Vulkyrie::Events {
    enum class MouseButton : u8 {
        Unknown = 0,          // Unknown/Unsupported button
        Left = 1,            // The left mouse button.
        ScrollWheel = 2,     // The middle mouse button.
        Right = 3,           // The right mouse button.
        ScrollWheelUp = 4,   // Scroll wheel up
        ScrollWheelDown = 5, // Scroll wheel down
    };
}
