#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie {
    class MouseScrolledEvent : public Event {
    public:
        MouseScrolledEvent(const f32 offsetX, const f32 offsetY)
            : OffsetX(offsetX)
            , OffsetY(offsetY) {
        }

        /** The horizontal scroll offset of the mouse wheel. */
        const f32 OffsetX;

        /** The vertical scroll offset of the mouse wheel. */
        const f32 OffsetY;

        [[nodiscard]] VE_INLINE i32 GetCategoryFlags() const override {
            return _categoryFlags;
        }

        [[nodiscard]] VE_INLINE EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] VE_INLINE std::string ToString() const override {
            return std::format("MouseScrolledEvent: ({}, {})", OffsetX, OffsetY);
        }

        /** @brief Gets the static event type for this event class.
         * @returns The static event type.
         */
        [[nodiscard]] static VE_INLINE EventType GetStaticEventType() {
            return EventType::MouseScrolled;
        }

    private:
        static constexpr i32 _categoryFlags = std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie
