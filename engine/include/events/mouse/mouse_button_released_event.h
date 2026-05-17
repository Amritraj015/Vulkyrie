#pragma once

#include "events/mouse/mouse_button_event.h"

namespace Vulkyrie {
    class MouseButtonReleasedEvent : public MouseButtonEvent {
    public:
        explicit MouseButtonReleasedEvent(const enum MouseButton mouseButton)
            : MouseButtonEvent(mouseButton) {
        }

        [[nodiscard]] inline EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] inline std::string ToString() const override {
            return std::format("MouseButtonReleasedEvent: {}", std::to_underlying(MouseButton));
        }

        /** @brief Gets the static event type for this event class.
         * @return The static event type.
         */
        [[nodiscard]] static inline EventType GetStaticEventType() {
            return EventType::MouseButtonReleased;
        }
    };
} // namespace Vulkyrie
