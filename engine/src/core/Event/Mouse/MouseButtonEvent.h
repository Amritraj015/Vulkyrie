#pragma once
#include "Core/Event/Event.h"
#include "Core/Input/MouseButton.h"
#include "Core/Event/Enums/EventCategory.h"

namespace Vkr
{
    class MouseButtonEvent : public Event
    {
    public:
        MouseButtonEvent(const MouseButton button, const bool pressed, const f32 mouseX, const f32 mouseY)
            : mButton(button), pressed(pressed), mMouseX(mouseX), mMouseY(mouseY) {}

        virtual int GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Mouse) | to_underlying(EventCategory::Input) | to_underlying(EventCategory::MouseButton);
        }

        virtual EventType GetEventType() const override { return pressed ? EventType::MouseButtonPressed : EventType::MouseButtonReleased; }
        inline MouseButton GetMouseButton() const { return mButton; }
        inline f32 GetMouseX() const { return mMouseX; }
        inline f32 GetMouseY() const { return mMouseY; }

    private:
        const MouseButton mButton;
        const bool pressed;
        const f32 mMouseX, mMouseY;
    };
}