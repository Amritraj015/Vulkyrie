#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    class WindowCloseEvent : public Event {
        public:
            [[nodiscard]] inline EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return "WindowCloseEvent";
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::WindowClose;
            }

        private:
            const static i32 _categoryFlags = std::to_underlying(EventCategory::ApplicationEvent);
    };
} // namespace Vulkyrie::Events 
