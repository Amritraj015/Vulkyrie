#pragma once

#include "events/mouse/mouse_button_event.h"

namespace Vulkyrie::Events {
    class MouseButtonReleasedEvent : public MouseButtonEvent {
        public:
            explicit MouseButtonReleasedEvent(const MouseButton button) : MouseButtonEvent(button) { }

            [[nodiscard]] inline EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseButtonReleasedEvent: {}", std::to_underlying(GetMouseButton()));
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::MouseButtonReleased;
            }
    };
}