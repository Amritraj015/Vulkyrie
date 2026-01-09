#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Events {
    enum class MouseButton : u8 {
        MouseButton1 = 0,   // Mouse button 1 (usually left button)
        MouseButton2,       // Mouse button 2 (usually right button)
        MouseButton3,       // Mouse button 3 (usually middle button)
        MouseButton4,       // Mouse button 4
        MouseButton5,       // Mouse button 5
        MouseButton6,       // Mouse button 6
        MouseButton7,       // Mouse button 7
        MouseButton8,       // Mouse button 8
        Unknown             // Unknown/Unsupported button
    };
}
