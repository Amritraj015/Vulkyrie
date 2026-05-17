#pragma once

#include "events/keyboard/key_event.h"
#include "events/enums/key_modifier.h"

namespace Vulkyrie {
    class KeyPressedEvent : public KeyEvent {
    public:
        KeyPressedEvent(const enum KeyCode keycode, const KeyModifier modifiers, const bool isRepeat = false)
            : KeyEvent(keycode)
            , IsRepeat(isRepeat)
            , Modifiers(modifiers) {
        }

        /** @brief Indicates whether the key press is a repeat. */
        const bool IsRepeat;

        /** @brief The key modifiers active during the key press. */
        const KeyModifier Modifiers;

        [[nodiscard]] inline EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] inline std::string ToString() const override {
            return std::format(
                "KeyPressedEvent: {} (repeats: {}, modifiers: {})", std::to_underlying(KeyCode), IsRepeat ? "true" : "false", std::to_underlying(Modifiers));
        }

        /** @brief Gets the static event type for this event class.
         * @return The static event type.
         */
        [[nodiscard]] static inline EventType GetStaticEventType() {
            return EventType::KeyPressed;
        }
    };
} // namespace Vulkyrie
