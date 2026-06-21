#pragma once

#include "vlkypch.h"
#include "physics/types/collision_callback.h"
#include "physics/types/overlap_callback.h"

namespace Vulkyrie {

    class EventListener : public CollisionCallback {
    public:
        EventListener() = default;

        VE_DELETE_MOVE_AND_COPY(EventListener);

        virtual ~EventListener() override = default;

        virtual void OnCollision([[maybe_unused]] const CollisionCallback::Data &collisionData) override {
        }

        virtual void OnTrigger(const OverlapCallback::Data &overlapData) = 0;
    };

} // namespace Vulkyrie
