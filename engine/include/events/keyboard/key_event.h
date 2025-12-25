#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"
#include "events/enums/key_code.h"

namespace Vulkyrie::Events {
    // A class to represent a key press or release event.
    class KeyEvent : public Event {
        public:
            KeyEvent(KeyCode keycode) : _keyCode(keycode) {
            }

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline KeyCode GetKeyCode() const {
                return _keyCode;
            }

        private:
            const KeyCode _keyCode;
            const static i32 _categoryFlags = std::to_underlying(EventCategory::Keyboard) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie::Events 