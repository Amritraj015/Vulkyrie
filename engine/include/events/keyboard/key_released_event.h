#pragma once

#include "events/keyboard/key_event.h"

namespace Vulkyrie::Events {
    class KeyReleasedEvent : public KeyEvent {
        public:
            KeyReleasedEvent(const KeyCode keycode) : KeyEvent(keycode) {
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::KeyReleased;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("KeyReleasedEvent: {}", std::to_underlying(GetKeyCode()));
            }
    };
}