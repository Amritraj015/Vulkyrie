#pragma once
#include "Core/Event/Event.h"
#include "Core/Event/Enums/EventCategory.h"

namespace Vkr
{
    class MouseMovedEvent : public Event
    {
    public:
        MouseMovedEvent(const f32 x, const f32 y)
            : mMouseX(x), mMouseY(y) {}

        f32 GetX() const { return mMouseX; }
        f32 GetY() const { return mMouseY; }

        virtual int GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Mouse) | to_underlying(EventCategory::Input);
        }

        virtual EventType GetEventType() const override { return EventType::MouseMoved; }

    private:
        const f32 mMouseX, mMouseY;
    };
}