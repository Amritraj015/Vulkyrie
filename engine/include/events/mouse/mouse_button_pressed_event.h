#pragma once

#include "events/mouse/mouse_button_event.h"
#include "events/enums/key_modifier.h"

namespace Vulkyrie {
    class MouseButtonPressedEvent : public MouseButtonEvent {
    public:
        MouseButtonPressedEvent(const enum MouseButton mouseButton, const KeyModifier modifiers)
            : MouseButtonEvent(mouseButton)
            , Modifiers(modifiers) {
        }

        const KeyModifier Modifiers;

        [[nodiscard]] inline EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] inline std::string ToString() const override {
            return std::format("MouseButtonPressedEvent: {} (modifiers: {})", std::to_underlying(MouseButton), std::to_underlying(Modifiers));
        }

        /** @brief Gets the static event type for this event class.
         * @returns The static event type.
         */
        [[nodiscard]] static inline EventType GetStaticEventType() {
            return EventType::MouseButtonPressed;
        }
    };
} // namespace Vulkyrie
