#pragma once
#include "Core/Event/Event.h"
#include "Core/Event/Enums/EventCategory.h"

namespace Vkr
{
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(const i32 x, const i32 y)
            : mMouseX(x), mMouseY(y) {}

        [[nodiscard]] inline i32 GetX() const { return mMouseX; }
        [[nodiscard]] inline i32 GetY() const { return mMouseY; }

        [[nodiscard]] inline i32 GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Mouse) | to_underlying(EventCategory::Input);
        }

        [[nodiscard]] inline EventType GetEventType() const override { return EventType::MouseMoved; }

    private:
        const i32 mMouseX, mMouseY;
    };
}