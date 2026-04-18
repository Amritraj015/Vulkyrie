#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"
#include "events/enums/key_code.h"

namespace Vulkyrie {
    // A class to represent a key press or release event.
    class KeyEvent : public Event {
        public:
            KeyEvent(Vulkyrie::KeyCode keycode) : KeyCode(keycode) {
            }

            /** @brief The key code associated with the event. */
            const Vulkyrie::KeyCode KeyCode;

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

        private:
            const static i32 _categoryFlags = std::to_underlying(EventCategory::Keyboard) | std::to_underlying(EventCategory::Input);
    };
} // namespace Vulkyrie 