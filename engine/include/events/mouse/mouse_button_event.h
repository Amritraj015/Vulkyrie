#pragma once

#include "events/event.h"
#include "events/enums/mouse_button.h"
#include "events/enums/event_category.h"

namespace Vulkyrie {
    class MouseButtonEvent : public Event {
        public:
            explicit MouseButtonEvent(const MouseButton mouseButton)
                : MouseButton(mouseButton) {
            }

            /** @brief The mouse button associated with the event. */
            const MouseButton MouseButton;

            [[nodiscard]] inline i32 GetCategoryFlags() const override {
                return _categoryFlags;
            }

            [[nodiscard]] inline std::string ToString() const override {
                return std::format("MouseButtonEvent: {}", std::to_underlying(MouseButton));
            }

        private:
            const static i32 _categoryFlags =
                std::to_underlying(EventCategory::Mouse) | std::to_underlying(EventCategory::Input) | std::to_underlying(EventCategory::MouseButton);
    };
} // namespace Vulkyrie
