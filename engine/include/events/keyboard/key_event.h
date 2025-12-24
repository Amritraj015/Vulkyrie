#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"
#include "events/enums/key.h"

namespace Vulkyrie::Events {
    // A class to represent a key press or release event.
    class KeyEvent : public Event {
        public:
            KeyEvent(Key keycode, bool pressed) : _keyCode(keycode), _pressed(pressed) {
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline EventType GetEventType() const override {
                return _pressed ? EventType::KeyPressed : EventType::KeyReleased;
            }

            [[nodiscard]] inline Key GetKeyCode() const {
                return _keyCode;
            }

            [[nodiscard]] inline bool IsKeyPressed() const {
                return _pressed;
            }

        private:
            const bool _pressed;
            const Key _keyCode;
            const static i32 _categoryFlags = std::to_underlying(EventCategory::Keyboard) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie::Events 