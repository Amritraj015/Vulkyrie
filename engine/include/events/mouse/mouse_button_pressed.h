#pragma once

#include "events/mouse/mouse_button_event.h"

namespace Vulkyrie::Events {
    class MouseButtonPressedEvent : public MouseButtonEvent {
        public:
            MouseButtonPressedEvent(const MouseButton button, const KeyModifier modifiers)
                : MouseButtonEvent(button), _modifiers(modifiers) {
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::MouseButtonPressed;
            }

            [[nodiscard]] inline KeyModifier GetModifiers() const {
                return _modifiers;
            }

        private:
            const KeyModifier _modifiers;
    };
}