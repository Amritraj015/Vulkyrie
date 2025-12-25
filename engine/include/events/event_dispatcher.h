#pragma once

#include "events/event.h"

namespace Vulkyrie::Events {
    class EventDispatcher {
        public:
            explicit EventDispatcher(Event &event) : _event(event) {
            }

            template <typename T, typename F> bool Dispatch(F &&func) {
                if (_event.GetEventType() == T::GetStaticType()) {
                    _event.handled |= func(static_cast<T &>(_event));

                    return true;
                }

                return false;
            }

        private:
            Event &_event;
    };
} // namespace Vulkyrie::Events