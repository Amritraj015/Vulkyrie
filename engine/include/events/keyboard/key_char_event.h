#pragma once

#include "events/keyboard/key_event.h"

namespace Vulkyrie::Events {
    class KeyCharEvent : public KeyEvent {
        public:
            KeyCharEvent(const KeyCode keycode) : KeyEvent(keycode) {
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::KeyTyped;
            }
    };
}