#pragma once
#include "Core/Event/Event.h"
#include "Core/Event/Enums/EventCategory.h"

namespace Vkr
{
    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(const bool direction, const i32 xOffset, const i32 yOffset)
            : direction(direction), mXOffset(xOffset), mYOffset(yOffset) {}

        [[nodiscard]] inline i32 GetXOffset() const { return mXOffset; }
        [[nodiscard]] inline i32 GetYOffset() const { return mYOffset; }
        [[nodiscard]] inline bool GetDirection() const { return direction; }

        [[nodiscard]] inline int GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Mouse) | to_underlying(EventCategory::Input);
        }

        [[nodiscard]] inline EventType GetEventType() const override { return EventType::MouseScrolled; }

    private:
        // `true` -> mouse scrolled up else mouse scrolled down.
        const bool direction;
        const i32 mXOffset, mYOffset;
    };
}