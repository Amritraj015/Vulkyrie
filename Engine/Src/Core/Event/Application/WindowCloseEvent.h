#pragma once

#include "Core/Event/Event.h"

namespace Vkr {
    class WindowCloseEvent : public Event {
    public:
        [[nodiscard]] inline EventType GetEventType() const override { return EventType::WindowClose; }

        [[nodiscard]] inline int GetCategoryFlags() const override {
            return to_underlying(EventCategory::ApplicationEvent);
        }
    };
}