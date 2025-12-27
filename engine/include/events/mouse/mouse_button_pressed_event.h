#pragma once

#include "events/mouse/mouse_button_event.h"

namespace Vulkyrie::Events {
    class MouseButtonPressedEvent : public MouseButtonEvent {
        public:
            MouseButtonPressedEvent(const MouseButton button, const KeyModifier modifiers)
                : MouseButtonEvent(button), _modifiers(modifiers) {
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline KeyModifier GetModifiers() const {
                return _modifiers;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseButtonPressedEvent: {} (modifiers: {})",
                                   std::to_underlying(GetMouseButton()),
                                   std::to_underlying(_modifiers));
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::MouseButtonPressed;
            }


        private:
            const KeyModifier _modifiers;
    };
}