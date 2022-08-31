#pragma once
#include "Core/Event/Event.h"
#include "Core/Event/Enums/EventCategory.h"

namespace Vkr
{
    class MouseScrolledEvent : public Event
    {
    public:
        MouseScrolledEvent(const bool direction, const f32 xOffset, const f32 yOffset)
            : direction(direction), mXOffset(xOffset), mYOffset(yOffset) {}

        inline f32 GetXOffset() const { return mXOffset; }
        inline f32 GetYOffset() const { return mYOffset; }
        inline bool GetDirection() const { return direction; }

        virtual int GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Mouse) | to_underlying(EventCategory::Input);
        }

        virtual EventType GetEventType() const override { return EventType::MouseScrolled; }

    private:
        // `true` -> mouse scrolled up else mouse scrolled down.
        const bool direction;
        const f32 mXOffset, mYOffset;
    };
}