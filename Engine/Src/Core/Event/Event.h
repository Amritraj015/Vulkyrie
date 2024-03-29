#pragma once

#include "Core/Event/Enums/EventType.h"

namespace Vkr
{
    /* Base event class that needs to be inherited by every event in the engine. */
    class Event
    {
    public:
        virtual ~Event() = default;
        bool handled = false;

        [[nodiscard]] virtual EventType GetEventType() const = 0;
        [[nodiscard]] virtual i32 GetCategoryFlags() const = 0;
    };
}
