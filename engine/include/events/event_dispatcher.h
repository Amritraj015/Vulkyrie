#pragma once

#include "events/event.h"

namespace Vulkyrie::Events {
    class EventDispatcher {
        public:
            explicit EventDispatcher(Event &event) : _event(event) {
            }

            /** Dispatches the event to the provided function if the event type matches.
             * @param[in] func The function to call if the event type matches.
             * @return True if the event was dispatched, false otherwise.
             */
            template <typename T, typename F>
                requires std::is_base_of_v<Event, T> && std::is_invocable_r_v<bool, F, T &>
            bool Dispatch(F &&func) {
                if (_event.GetEventType() == T::GetStaticEventType() && !_event.handled) {
                    _event.handled |= func(static_cast<T &>(_event));

                    return true;
                }

                return false;
            }

        private:
            Event &_event;
    };
} // namespace Vulkyrie::Events
