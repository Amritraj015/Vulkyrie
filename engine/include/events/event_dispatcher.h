#pragma once

#include "events/event.h"

namespace Vulkyrie {
    class EventDispatcher {
    public:
        explicit EventDispatcher(Event &event)
            : _event(event) {
        }

        /** Dispatches the event to the provided function if the event type matches.
         * @tparam T The type of the event to dispatch.
         * @tparam F The type of the function to call if the event type matches.
         * @param[in] func The function to call if the event type matches.
         * @returns True if the event was dispatched, false otherwise.
         */
        template <typename T, typename F>
            requires std::derived_from<T, Event> && std::is_invocable_r_v<bool, F, T &>
        bool Dispatch(F &&func) {
            if (_event.GetEventType() == T::GetStaticEventType() && !_event.Handled) {
                _event.Handled |= func(static_cast<T &>(_event));

                return true;
            }

            return false;
        }

    private:
        Event &_event;
    };
} // namespace Vulkyrie
