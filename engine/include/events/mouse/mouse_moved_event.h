#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie {
    class MouseMovedEvent : public Event {
    public:
        MouseMovedEvent(const f32 x, const f32 y)
            : MouseX(x)
            , MouseY(y) {
        }

        /** The X coordinate of the mouse cursor. */
        const f32 MouseX;

        /** The Y coordinate of the mouse cursor. */
        const f32 MouseY;

        [[nodiscard]] inline i32 GetCategoryFlags() const override {
            return _categoryFlags;
        }

        [[nodiscard]] inline EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] inline std::string ToString() const override {
            return std::format("MouseMovedEvent: ({}, {})", MouseX, MouseY);
        }

        /** @brief Gets the static event type for this event class.
         * @returns The static event type.
         */
        [[nodiscard]] static inline EventType GetStaticEventType() {
            return EventType::MouseMoved;
        }

    private:
        const static i32 _categoryFlags = std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie
