#pragma once

#include "physics/types/narrow_phase_data.h"

namespace Vulkyrie {

    class NarrowPhaseDataBatch final {
    public:
        NarrowPhaseDataBatch() = default;

        ~NarrowPhaseDataBatch() {
            Dispose();
        }

        std::vector<NarrowPhaseData> Data;

        VE_INLINE void AddNarrowPhaseData(u64 pairID,
                                          Entity colliderOne,
                                          Entity colliderTwo,
                                          CollisionShape &shapeOne,
                                          CollisionShape &shapeTwo,
                                          const TransformComponent &shapeOneTransform,
                                          const TransformComponent &shapeTwoTransform,
                                          bool reportContacts,
                                          LastFrameCollisionData &lastFrameInfo) {
            Data.emplace_back(pairID, colliderOne, colliderTwo, lastFrameInfo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts);
        }

        VE_INLINE void AddContactPoint(size_t index,
                                       const glm::vec3 &contactNormal,
                                       f32 penetrationDepth,
                                       const glm::vec3 &localSpaceContactPointOnBodyOne,
                                       const glm::vec3 &localSpaceContactPointOnBodyTwo) {

            VASSERT(penetrationDepth > f32(0.0), "Penetration depth should be greater than zero for a valid contact point.");

            NarrowPhaseData &data = Data[index];

            if (data.ContactPointCount < MAX_CONTACT_POINTS_PER_PAIR_IN_NARROW_PHASE) {
                VASSERT(glm::length2(contactNormal) == f32(1.0), "Contact normal should be a normalized vector with length of 1.");

                data.ContactPoints[data.ContactPointCount] = {
                    contactNormal, localSpaceContactPointOnBodyOne, localSpaceContactPointOnBodyTwo, penetrationDepth
                };

                data.ContactPointCount++;
            }
        }

        VE_INLINE void ResetContactPoints(size_t index) {
            Data[index].ContactPointCount = 0;
        }

        // void ReserveMemory();
        VE_INLINE void Dispose() {

            // TODO: Not a fan of this, need to figure out a better way to manage the lifetime of the triangle shapes that are created for concave collision
            // detection without having to manually delete them here in the destructor of the narrow-phase data batch. The pointers are initialized in
            // CollisionSystem.cpp

            for (NarrowPhaseData &data : Data) {
                if (data.ShapeOne.GetName() == CollisionShapeName::Triangle) {
                    delete &data.ShapeOne;
                }

                if (data.ShapeTwo.GetName() == CollisionShapeName::Triangle) {
                    delete &data.ShapeTwo;
                }
            }
        }
    };

} // namespace Vulkyrie
