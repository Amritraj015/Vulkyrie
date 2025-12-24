#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    class MouseMovedEvent : public Event {
        public:
            MouseMovedEvent(const i32 x, const i32 y) : _mouseX(x), _mouseY(y) {
            }

            [[nodiscard]] inline i32 GetX() const {
                return _mouseX;
            }

            [[nodiscard]] inline i32 GetY() const {
                return _mouseY;
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return EventType::MouseMoved;
            }

        private:
            const i32 _mouseX, _mouseY;
            const static i32 _categoryFlags = std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie::Events
