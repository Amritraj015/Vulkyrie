#pragma once
#include "Core/Event/Event.h"
#include "Core/Event/Enums/SenderType.h"
#include "Core/Event/Enums/ListenerType.h"

namespace Vkr
{
#define EVENT_CALLBACK_FUNCTION std::function<bool(SenderType, ListenerType, Event *)>

    struct RegisteredEvent
    {
        RegisteredEvent(EventType type, ListenerType listenerType, EVENT_CALLBACK_FUNCTION &callbackFn)
            : eventType(type), listenerType(listenerType), callback(callbackFn)
        {
        }

        RegisteredEvent(const RegisteredEvent &entry)
            : eventType(entry.eventType), listenerType(entry.listenerType), callback(entry.callback)
        {
            VDEBUG("!!\tRegisteredEvent - COPIED\t!!");
        }

        EventType eventType;
        ListenerType listenerType;
        EVENT_CALLBACK_FUNCTION callback;
    };
};
