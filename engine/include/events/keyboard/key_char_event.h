#pragma once

#include "events/keyboard/key_event.h"

namespace Vulkyrie::Events {
    class KeyCharEvent : public KeyEvent {
        public:
            KeyCharEvent(const Vulkyrie::Events::KeyCode keycode) : KeyEvent(keycode) {
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("KeyCharEvent: {}", std::to_underlying(KeyCode));
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::KeyChar;
            }
    };
}