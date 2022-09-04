#pragma once
#include "Core/Event/Event.h"
#include "Core/Input/MouseButton.h"
#include "Core/Event/Enums/EventCategory.h"

namespace Vkr
{
    class MouseButtonEvent : public Event
    {
    public:
        MouseButtonEvent(const MouseButton button, const bool pressed, const i32 mouseX, const i32 mouseY)
            : mButton(button), pressed(pressed), mMouseX(mouseX), mMouseY(mouseY) {}

        [[nodiscard]] inline int GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Mouse) | to_underlying(EventCategory::Input) | to_underlying(EventCategory::MouseButton);
        }

        [[nodiscard]] inline EventType GetEventType() const override { return pressed ? EventType::MouseButtonPressed : EventType::MouseButtonReleased; }
        [[nodiscard]] inline MouseButton GetMouseButton() const { return mButton; }
        [[nodiscard]] inline i32 GetMouseX() const { return mMouseX; }
        [[nodiscard]] inline i32 GetMouseY() const { return mMouseY; }
        [[nodiscard]] inline bool IsButtonPressed() const { return pressed; }

    private:
        const MouseButton mButton;
        const bool pressed;
        const i32 mMouseX, mMouseY;
    };
}