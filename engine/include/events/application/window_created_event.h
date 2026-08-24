#pragma once

#include "events/event.h"
#include "events/enums/event_category.h"
#include "core/types/application_types.h"

namespace Vulkyrie {

    /** @brief Event triggered during application bootstrap. */
    class WindowCreatedEvent final : public Event {
    public:
        WindowCreatedEvent(const Extent2D dimensions)
            : Dimensions(dimensions) {
        }

        /** @brief The dimensions of the created window. */
        const Extent2D Dimensions;

        [[nodiscard]] inline virtual EventType GetEventType() const override {
            return GetStaticEventType();
        }

        [[nodiscard]] inline virtual i32 GetCategoryFlags() const override {
            return _categoryFlags;
        }

        [[nodiscard]] inline std::string ToString() const override {
            return std::format("WindowCreatedEvent: {}x{}", Dimensions.Width, Dimensions.Height);
        }

        /** @brief Gets the static event type for this event class.
         * @returns The static event type.
         */
        [[nodiscard]] static inline EventType GetStaticEventType() {
            return EventType::WindowCreated;
        }

    private:
        const static i32 _categoryFlags = std::to_underlying(EventCategory::ApplicationEvent);
    };

} // namespace Vulkyrie
