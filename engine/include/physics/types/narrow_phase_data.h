#pragma once

#include "core/entity.h"
#include "physics/physics_constants.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/components/transform_component_store.h"
#include "physics/types/contact_point_data.h"
#include "physics/types/last_frame_collision_info.h"

namespace Vulkyrie {

    struct NarrowPhaseData final {
        public:
            NarrowPhaseData(u64 pairID,
                            Entity colliderOneEntity,
                            Entity colliderTwoEntity,
                            LastFrameCollisionInfo &lastFrameCollisionInfo,
                            CollisionShape &shapeOne,
                            CollisionShape &shapeTwo,
                            const TransformComponent &shapeOneWorldTransform,
                            const TransformComponent &shapeTwoWorldTransform,
                            bool reportContacts)
                : ShapeOneWorldTransform(shapeOneWorldTransform)
                , ShapeTwoWorldTransform(shapeTwoWorldTransform)
                , LastFrameCollisionInfo(lastFrameCollisionInfo)
                , ColliderOneEntity(colliderOneEntity)
                , ColliderTwoEntity(colliderTwoEntity)
                , ShapeOne(shapeOne)
                , ShapeTwo(shapeTwo)
                , OverlappingPairID(pairID)
                , ContactPointCount(0)
                , ReportContacts(reportContacts)
                , IsColliding(false) {
            }

            ~NarrowPhaseData() = default;

            ContactPointData ContactPoints[MAX_CONTACT_POINTS_PER_PAIR_IN_NARROW_PHASE];
            TransformComponent ShapeOneWorldTransform;
            TransformComponent ShapeTwoWorldTransform;
            LastFrameCollisionInfo &LastFrameCollisionInfo;
            Entity ColliderOneEntity;
            Entity ColliderTwoEntity;
            CollisionShape &ShapeOne;
            CollisionShape &ShapeTwo;
            u64 OverlappingPairID;
            u8 ContactPointCount;
            bool ReportContacts;
            bool IsColliding;
    };

} // namespace Vulkyrie
