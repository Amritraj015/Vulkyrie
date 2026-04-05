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

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseScrolledEvent: ({}, {})", OffsetX, OffsetY);
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::MouseScrolled;
            }

        private:
            static constexpr i32 _categoryFlags = std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie
