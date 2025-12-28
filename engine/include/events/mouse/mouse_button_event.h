#pragma once

#include "events/event.h"
#include "events/enums/mouse_button.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    class MouseButtonEvent : public Event {
        public:
            explicit MouseButtonEvent(const MouseButton button) : _button(button) { }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline MouseButton GetMouseButton() const {
                return _button;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseButtonEvent: {}", std::to_underlying(GetMouseButton()));
            }

        private:
            const MouseButton _button;
            const static i32 _categoryFlags =
                std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input) | std::to_underlying(EventCategory::MouseButton);
    };
} // namespace Vulkyrie::Events 
