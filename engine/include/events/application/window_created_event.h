#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    /** @brief Event triggered during application bootstrap. */
    class WindowCreatedEvent final : public Event {
        public:
            WindowCreatedEvent() = default;
            ~WindowCreatedEvent() = default;

            [[nodiscard]] inline virtual EventType GetEventType() const {
                return EventType::WindowCreated;
            }

            [[nodiscard]] inline virtual i32 GetCategoryFlags() const {
                return _categoryFlags;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return "WindowCreatedEvent";
            }

        private:
            const static i32 _categoryFlags = std::to_underlying(EventCategory::ApplicationEvent);
    };
} // namespace Vulkyrie::Events