#pragma once

#include "core/entity.h"
#include "physics/physics_constants.h"
#include "physics/components/transform_component_store.h"
#include "physics/collision/shapes/collision_shape.h"

namespace Vulkyrie {

    struct NarrowPhaseDataBatch final {
        public:
            NarrowPhaseDataBatch() = default;

            NarrowPhaseDataBatch(const NarrowPhaseDataBatch &) = delete;
            NarrowPhaseDataBatch &operator=(const NarrowPhaseDataBatch &) = delete;

            NarrowPhaseDataBatch(NarrowPhaseDataBatch &&) = delete;
            NarrowPhaseDataBatch &operator=(NarrowPhaseDataBatch &&) = delete;

            ~NarrowPhaseDataBatch() = default;

            u64 OverlappingPairID;
            Entity ColliderOneEntity;
            Entity ColliderTwoEntity;
            TransformComponent ShapeOneWorldTransform;
            TransformComponent ShapeTwoWorldTransform;
            CollisionShape &ShapeOne;
            CollisionShape &ShapeTwo;
            bool ReportContacts;
            bool IsColliding;
            u8 ContactPointCount;
            ContactPoint ContactPoints[MAX_CONTACT_POINTS_PER_PAIR_IN_NARROW_PHASE];
    };

} // namespace Vulkyrie
