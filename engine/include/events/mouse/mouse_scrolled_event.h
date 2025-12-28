#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    class MouseScrolledEvent : public Event {
        public:
            MouseScrolledEvent(const f64 offsetX, const f64 offsetY)
                : _offsetX(offsetX), _offsetY(offsetY) {
            }

            [[nodiscard]] inline f64 GetXOffset() const {
                return _offsetX;
            }

            [[nodiscard]] inline f64 GetYOffset() const {
                return _offsetY;
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return GetStaticEventType();
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseScrolledEvent: ({}, {})", _offsetX, _offsetY);
            }

            /** @brief Gets the static event type for this event class.
             * @return The static event type.
             */
            [[nodiscard]] static inline EventType GetStaticEventType() {
                return EventType::MouseScrolled;
            }

        private:
            const f64 _offsetX, _offsetY;
            static constexpr i32 _categoryFlags = std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie::Events
