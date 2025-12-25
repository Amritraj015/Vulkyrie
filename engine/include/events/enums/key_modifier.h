#pragma once

#include "defines.h"
#include "core/utilities.h"

namespace Vulkyrie::Events {
    enum class KeyModifier : i32 {
        None        = 0,
        Shift       = BIT(0),
        Control     = BIT(1),
        Alt         = BIT(2),
        Super       = BIT(3),
        CapsLock    = BIT(4),
        NumLock     = BIT(5)
    };
} // namespace Vulkyrie::Events