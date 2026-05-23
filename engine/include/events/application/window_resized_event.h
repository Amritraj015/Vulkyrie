#pragma once

#include "vlkypch.h"
#include "events/event.h"
#include "events/enums/event_category.h"
#include "events/enums/event_type.h"

namespace Vulkyrie {
    class WindowResizedEvent : public Event {
    public:
        WindowResizedEvent(const u32 width, const u32 height)
            : Width(width)
            , Height(height) {
        }

        /** @brief The new width of the window. */
        const u32 Width;

        /** @brief The new height of the window. */
        const u32 Height;

        [[nodiscard]] VE_INLINE i32 GetCategoryFlags() const override {
            return _categoryFlags;
        }

        [[nodiscard]] VE_INLINE EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] VE_INLINE std::string ToString() const override {
            return std::format("WindowResizeEvent: {}x{}", Width, Height);
        }

        /** @brief Gets the static event type for this event class.
         * @returns The static event type.
         */
        [[nodiscard]] static VE_INLINE EventType GetStaticEventType() {
            return EventType::WindowResized;
        }

    private:
        const static i32 _categoryFlags = std::to_underlying(EventCategory::ApplicationEvent);
    };
} // namespace Vulkyrie
