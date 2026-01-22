#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Events {
    enum class KeyModifier : u8 {
        None        = 0,
        Shift       = 1 << 0,
        Control     = 1 << 1,
        Alt         = 1 << 2,
        Super       = 1 << 3,
        CapsLock    = 1 << 4,
        NumLock     = 1 << 5
    };
} // namespace Vulkyrie::Events