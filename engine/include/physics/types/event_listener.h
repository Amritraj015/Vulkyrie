#pragma once

#include "physics/types/collision_callback.h"
#include "physics/types/overlap_callback.h"

namespace Vulkyrie {

    class EventListener : public CollisionCallback {
    public:
        EventListener() = default;

        EventListener(const EventListener &) = delete;
        EventListener &operator=(const EventListener &) = delete;

        EventListener(EventListener &&) = delete;
        EventListener &operator=(EventListener &&) = delete;

        virtual ~EventListener() override = default;

        virtual void OnCollision([[maybe_unused]] const CollisionCallback::Data &collisionData) override {
        }

        virtual void OnTrigger(const OverlapCallback::Data &overlapData) = 0;
    };

} // namespace Vulkyrie
