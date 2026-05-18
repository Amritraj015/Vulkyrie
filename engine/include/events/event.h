#pragma once

#include "vlkypch.h"
#include "enums/event_type.h"

namespace Vulkyrie {
    /** @brief Base event class that needs to be inherited by every event in the engine. */
    class Event {
    public:
        virtual ~Event() = default;

        /** @brief Indicates whether the event has been handled. */
        bool handled = false;

        /** @brief Gets the event type. */
        [[nodiscard]] inline virtual EventType GetEventType() const = 0;

        /** @brief Gets the category flags for this vent. */
        [[nodiscard]] inline virtual i32 GetCategoryFlags() const = 0;

        /** @brief Checks if the event is in a specific category.
         * @param[in] category The category to check against.
         * @returns True if the event is in the specified category, false otherwise.
         */
        [[nodiscard]] inline bool IsInCategory(i32 category) const {
            return GetCategoryFlags() & category;
        }

        /** @brief Converts the event to a string representation.
         * @returns A string representation of the event.
         */
        [[nodiscard]] inline virtual std::string ToString() const {
            return "General Event";
        }

        /** @brief Gets the static event type for this event class.
         * @returns The static event type.
         */
        [[nodiscard]] static inline EventType GetStaticEventType() {
            return EventType::Unknown;
        }
    };
} // namespace Vulkyrie
