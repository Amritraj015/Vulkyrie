#include "EventSystemManager.h"

namespace Vkr
{
    std::vector<RegisteredEvent> EventSystemManager::registry;

    StatusCode EventSystemManager::RegisterEvent(EventType eventType, ListenerType listenerType, EVENT_CALLBACK_FUNCTION function)
    {
        if (registry.capacity() == 0)
            registry.reserve(20);
        else if (registry.size() == registry.capacity())
            registry.reserve(registry.capacity() + 20);

        for (const auto &entry : registry)
        {
            if (entry.eventType == eventType && entry.listenerType == listenerType)
            {
                VERROR("EventType: `%i` and ListenerType: `%i`, has already been registered, Exiting!", to_underlying(eventType), listenerType)
                return StatusCode::EventAlreadyRegistered;
            }
        }

        registry.emplace_back(eventType, listenerType, function);

        return StatusCode::Successful;
    }

    StatusCode EventSystemManager::UnregisterEvent(EventType eventType, ListenerType listenerType)
    {
        registry.erase(std::remove_if(registry.begin(), registry.end(), [&](const RegisteredEvent &re)
                                      { return re.eventType == eventType && re.listenerType == listenerType; }),
                       registry.end());

        return StatusCode::Successful;
    }

    StatusCode EventSystemManager::UnregisterAllEvents()
    {
        registry.clear();
        return StatusCode::Successful;
    }

    bool EventSystemManager::Dispatch(Event *event, SenderType senderType, ListenerType listenerType)
    {
        bool handled = false;

        if (listenerType == ListenerType::All)
        {
            for (const auto &ev : registry)
            {
                if (ev.eventType == event->GetEventType())
                {
                    handled = ev.callback(ev.eventType, senderType, listenerType, event);

                    if (handled && !event->handled)
                        event->handled = handled;
                }
            }
        }
        else
        {
            for (const auto &ev : registry)
            {
                if (ev.eventType == event->GetEventType() && ev.listenerType == listenerType)
                {
                    handled = ev.callback(ev.eventType, senderType, listenerType, event);

                    if (handled && !event->handled)
                        event->handled = handled;
                }
            }
        }

        return event->handled;
    }
}
