#pragma once

#include "events/event.h"
#include "events/enums/mouse_button.h"
#include "events/enums/event_category.h"

namespace Vulkyrie::Events {
    class MouseButtonEvent : public Event {
        public:
            MouseButtonEvent(const MouseButton button, const bool pressed, const i32 mouseX, const i32 mouseY)
                : _button(button), _pressed(pressed), _mouseX(mouseX), _mouseY(mouseY) {
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return _pressed ? EventType::MouseButtonPressed : EventType::MouseButtonReleased;
            }

            [[nodiscard]] inline MouseButton GetMouseButton() const {
                return _button;
            }

            [[nodiscard]] inline i32 GetMouseX() const {
                return _mouseX;
            }

            [[nodiscard]] inline i32 GetMouseY() const {
                return _mouseY;
            }

            [[nodiscard]] inline bool IsButtonPressed() const {
                return _pressed;
            }

        private:
            const MouseButton _button;
            const bool _pressed;
            const i32 _mouseX, _mouseY;
            const static i32 _categoryFlags =
                std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input) | std::to_underlying(EventCategory::MouseButton);
    };
} // namespace Vulkyrie::Events 
