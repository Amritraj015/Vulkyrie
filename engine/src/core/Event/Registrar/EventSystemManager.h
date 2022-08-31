#pragma once

#include "Defines.h"
#include "RegisteredEvent.h"

namespace Vkr
{
    // A class that manages the event system's callback registry.
    class EventSystemManager
    {
    private:
        static std::vector<RegisteredEvent> registry;

    public:
        EventSystemManager(const EventSystemManager &) = delete;
        void operator=(EventSystemManager const &) = delete;

        /**
         * Registers an event.
         * @param eventType The type of event to register
         * @param listenerType The listener type of this event.
         * @param function Callback function to invoke when the event occurs.
         * @returns StatusCode::Successful if the event was registered successfully;
         * otherwise StatusCode::EventAlreadyRegistered.
         */
        static StatusCode RegisterEvent(EventType eventType, ListenerType listenerType, EVENT_CALLBACK_FUNCTION function);

        /**
         * Unregisters an event.
         * @param eventType The type of event to unregister
         * @param listenerType The listener type of this event.
         * @returns StatusCode::Successful if the event was unregistered successfully;
         */
        static StatusCode UnregisterEvent(EventType eventType, ListenerType listenerType);

        /**
         * Unregisters all events.
         * @returns StatusCode::Successful if all events are unregistered successfully.
         */
        static StatusCode UnregisterAllEvents();

        /**
         * Dispatches an event.
         * @param event The event to dispatch.
         * @param senderType A code representing the event sender.
         * @param listenerType A code representing the Listener(s) to invoke.
         * @returns true if the event was dispatched successfully; otherwise false.
         */
        static bool Dispatch(Event *event, SenderType senderType = SenderType::Anonymous, ListenerType listenerType = ListenerType::All);
    };
}
