#pragma once

#include "events/keyboard/key_event.h"
#include "events/enums/key_modifier.h"

namespace Vulkyrie::Events {
    class KeyPressedEvent : public KeyEvent {
        public:
            KeyPressedEvent(const KeyCode keycode, const KeyModifier modifiers, const bool isRepeat = false)
                : KeyEvent(keycode), _isRepeat(isRepeat), _modifiers(modifiers) {
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::KeyPressed;
            }

            [[nodiscard]] inline bool IsRepeat() const {
                return _isRepeat;
            }

            [[nodiscard]] inline KeyModifier GetModifiers() const {
                return _modifiers;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("KeyPressedEvent: {} (repeats: {}, modifiers: {})",
                                   std::to_underlying(GetKeyCode()),
                                   _isRepeat ? "true" : "false",
                                   std::to_underlying(_modifiers));
            }

        private:
            const bool _isRepeat;
            const KeyModifier _modifiers;
    };
}