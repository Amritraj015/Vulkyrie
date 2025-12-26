#pragma once

#include "events/mouse/mouse_button_event.h"

namespace Vulkyrie::Events {
    class MouseButtonReleasedEvent : public MouseButtonEvent {
        public:
            MouseButtonReleasedEvent(const MouseButton button) : MouseButtonEvent(button) { }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::MouseButtonReleased;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseButtonReleasedEvent: {}", std::to_underlying(GetMouseButton()));
            }
    };
}