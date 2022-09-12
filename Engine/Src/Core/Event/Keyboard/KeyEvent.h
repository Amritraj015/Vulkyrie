#pragma once

#include "Core/Event/Event.h"
#include "Core/Event/Enums/EventCategory.h"
#include "Core/Input/Key.h"

namespace Vkr
{
    // A class to represent a Key press or release event.
    class KeyEvent : public Event
    {
    public:
        KeyEvent(Key keycode, bool pressed) : keycode(keycode), pressed(pressed) {}

        [[nodiscard]] inline i32 GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Keyboard) | to_underlying(EventCategory::Input);
        }

        [[nodiscard]] inline EventType GetEventType() const override { return pressed ? EventType::KeyPressed : EventType::KeyReleased; }
        [[nodiscard]] inline Key GetKeyCode() const { return keycode; }
        [[nodiscard]] inline bool IsKeyPressed() const { return pressed; }

    private:
        bool pressed;
        Key keycode;
    };
}
