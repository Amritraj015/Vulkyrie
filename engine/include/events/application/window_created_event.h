#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    /** @brief Event triggered during application bootstrap. */
    class WindowCreatedEvent final : public Event {
        public:
            WindowCreatedEvent(const u32 width, const u32 height) : Width(width), Height(height) {}

            /** @brief The width of the created window. */
            const u32 Width;

            /** @brief The height of the created window. */
            const u32 Height;

            [[nodiscard]] inline virtual EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline virtual i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("WindowCreatedEvent: {}x{}", Width, Height);
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
