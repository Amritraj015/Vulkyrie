#pragma once

#include "defines.h"

namespace Vulkyrie::Core {
    struct VulkyrieWindowProps {
        // Starting position of the window on x-axis.
        u32 startX = 0;

        // Starting position of the window on x-axis.
        u32 startY = 0;

        // Starting width of the window.
        u32 height = 0;

        // Starting width of the window.
        u32 width = 0;

        // The title for the window.
        const char *title;
    };
}
