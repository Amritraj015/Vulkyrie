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

        virtual int GetCategoryFlags() const override
        {
            return to_underlying(EventCategory::Keyboard) | to_underlying(EventCategory::Input);
        }

        virtual EventType GetEventType() const override { return pressed ? EventType::KeyPressed : EventType::KeyReleased; }
        inline Key GetKeyCode() const { return keycode; }
        inline bool IsKeyPressed() const { return pressed; }

    private:
        bool pressed;
        Key keycode;
    };
}
