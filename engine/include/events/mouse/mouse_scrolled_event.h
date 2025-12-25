#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    class MouseScrolledEvent : public Event {
        public:
            MouseScrolledEvent(const i32 offsetX, const i32 offsetY)
                : _offsetX(offsetX), _offsetY(offsetY) {
            }

            [[nodiscard]] inline i32 GetXOffset() const {
                return _offsetX;
            }

            [[nodiscard]] inline i32 GetYOffset() const {
                return _offsetY;
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::MouseScrolled;
            }

        private:
            const i32 _offsetX, _offsetY;
            const static i32 _categoryFlags = std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie::Events
