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
                return GetStaticEventType();
            }

            [[nodiscard]] inline virtual i32 GetCategoryFlags() const {
                return _categoryFlags;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return "WindowCreatedEvent";
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::WindowCreated;
            }

        private:
            const static i32 _categoryFlags = std::to_underlying(EventCategory::ApplicationEvent);
    };
} // namespace Vulkyrie::Events