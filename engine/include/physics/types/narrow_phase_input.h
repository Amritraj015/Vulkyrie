#pragma once

#include "core/entity.h"
#include "physics/components/transform_component_store.h"
#include "physics/collision/shapes/collision_shape.h"
#include "physics/collision/narrowphase/narrow_phase_data_batch.h"
#include "physics/types/narrow_phase_algorithm.h"

namespace Vulkyrie {

    /**
     * @brief Aggregates narrow-phase test inputs, routing each overlapping pair to the correct algorithm-specific batch.
     *
     * Populated during broad-phase processing: each overlapping pair that requires a collision check is
     * submitted via AddNarrowPhaseTest, which dispatches it to one of six NarrowPhaseDataBatch instances
     * based on the pair's NarrowPhaseAlgorithm. The narrow-phase solver then processes each batch
     * independently, running the appropriate algorithm (e.g. sphere-sphere, GJK/EPA) against all pairs
     * in that batch.
     */
    class NarrowPhaseInput {
    public:
        /**
         * @brief Constructs a NarrowPhaseInput, forwarding the overlapping pairs reference to each batch.
         * @param overlappingPairs The set of overlapping pairs managed by the broad phase.
         */
        explicit NarrowPhaseInput();

        /**
         * @brief Returns the batch for sphere-vs-sphere narrow-phase tests.
         * @returns A reference to the sphere-vs-sphere NarrowPhaseDataBatch.
         */
        [[nodiscard]] VE_INLINE NarrowPhaseDataBatch &GetSphereVsSphereBatch() {
            return _sphereVsSphereBatch;
        }

        /**
         * @brief Returns the batch for sphere-vs-capsule narrow-phase tests.
         * @returns A reference to the sphere-vs-capsule NarrowPhaseDataBatch.
         */
        [[nodiscard]] VE_INLINE NarrowPhaseDataBatch &GetSphereVsCapsuleBatch() {
            return _sphereVsCapsuleBatch;
        }

        /**
         * @brief Returns the batch for capsule-vs-capsule narrow-phase tests.
         * @returns A reference to the capsule-vs-capsule NarrowPhaseDataBatch.
         */
        [[nodiscard]] VE_INLINE NarrowPhaseDataBatch &GetCapsuleVsCapsuleBatch() {
            return _capsuleVsCapsuleBatch;
        }

        /**
         * @brief Returns the batch for sphere-vs-convex-polyhedron narrow-phase tests.
         * @returns A reference to the sphere-vs-convex-polyhedron NarrowPhaseDataBatch.
         */
        [[nodiscard]] VE_INLINE NarrowPhaseDataBatch &GetSphereVsConvexPolyhedronBatch() {
            return _sphereVsConvexPolyhedronBatch;
        }

        /**
         * @brief Returns the batch for capsule-vs-convex-polyhedron narrow-phase tests.
         * @returns A reference to the capsule-vs-convex-polyhedron NarrowPhaseDataBatch.
         */
        [[nodiscard]] VE_INLINE NarrowPhaseDataBatch &GetCapsuleVsConvexPolyhedronBatch() {
            return _capsuleVsConvexPolyhedronBatch;
        }

        /**
         * @brief Returns the batch for convex-polyhedron-vs-convex-polyhedron narrow-phase tests.
         * @returns A reference to the convex-polyhedron-vs-convex-polyhedron NarrowPhaseDataBatch.
         */
        [[nodiscard]] VE_INLINE NarrowPhaseDataBatch &GetConvexPolyhedronVsConvexPolyhedronBatch() {
            return _convexPolyhedronVsConvexPolyhedronBatch;
        }

        /**
         * @brief Submits a narrow-phase test for an overlapping pair, routing it to the correct algorithm batch.
         *
         * Dispatches the pair's data to the NarrowPhaseDataBatch that corresponds to @p narrowPhaseAlgorithm.
         * Triggers a failed assertion if the algorithm is not supported.
         *
         * @param pairID The unique identifier of the overlapping pair.
         * @param colliderOne The entity of the first collider.
         * @param colliderTwo The entity of the second collider.
         * @param shapeOne The collision shape of the first collider.
         * @param shapeTwo The collision shape of the second collider.
         * @param shapeOneTransform World-space transform of the first shape.
         * @param shapeTwoTransform World-space transform of the second shape.
         * @param narrowPhaseAlgorithm The algorithm to use, which determines the target batch.
         * @param reportContacts Whether contact point data should be generated for this pair.
         * @param lastFrameInfo Cached collision info from the previous frame, used by warm-starting algorithms.
         */
        VE_INLINE void AddNarrowPhaseTest(u64 pairID,
                                                Entity colliderOne,
                                                Entity colliderTwo,
                                                CollisionShape &shapeOne,
                                                CollisionShape &shapeTwo,
                                                const TransformComponent &shapeOneTransform,
                                                const TransformComponent &shapeTwoTransform,
                                                NarrowPhaseAlgorithm narrowPhaseAlgorithm,
                                                bool reportContacts,
                                                LastFrameCollisionData &lastFrameInfo) {
            switch (narrowPhaseAlgorithm) {
                case NarrowPhaseAlgorithm::SphereVsSphere:
                    _sphereVsSphereBatch.AddNarrowPhaseData(
                        pairID, colliderOne, colliderTwo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts, lastFrameInfo);
                    break;
                case NarrowPhaseAlgorithm::SphereVsCapsule:
                    _sphereVsCapsuleBatch.AddNarrowPhaseData(
                        pairID, colliderOne, colliderTwo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts, lastFrameInfo);
                    break;
                case NarrowPhaseAlgorithm::CapsuleVsCapsule:
                    _capsuleVsCapsuleBatch.AddNarrowPhaseData(
                        pairID, colliderOne, colliderTwo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts, lastFrameInfo);
                    break;
                case NarrowPhaseAlgorithm::SphereVsConvexPolyhedron:
                    _sphereVsConvexPolyhedronBatch.AddNarrowPhaseData(
                        pairID, colliderOne, colliderTwo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts, lastFrameInfo);
                    break;
                case NarrowPhaseAlgorithm::CapsuleVsConvexPolyhedron:
                    _capsuleVsConvexPolyhedronBatch.AddNarrowPhaseData(
                        pairID, colliderOne, colliderTwo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts, lastFrameInfo);
                    break;
                case NarrowPhaseAlgorithm::ConvexPolyhedronVsConvexPolyhedron:
                    _convexPolyhedronVsConvexPolyhedronBatch.AddNarrowPhaseData(
                        pairID, colliderOne, colliderTwo, shapeOne, shapeTwo, shapeOneTransform, shapeTwoTransform, reportContacts, lastFrameInfo);
                    break;
                default:
                    VASSERT(false, "Unsupported narrow-phase algorithm.");
            }
        }

    private:
        /** @brief Batch of narrow-phase tests between sphere and sphere shapes. */
        NarrowPhaseDataBatch _sphereVsSphereBatch;

        /** @brief Batch of narrow-phase tests between sphere and capsule shapes. */
        NarrowPhaseDataBatch _sphereVsCapsuleBatch;

        /** @brief Batch of narrow-phase tests between capsule and capsule shapes. */
        NarrowPhaseDataBatch _capsuleVsCapsuleBatch;

        /** @brief Batch of narrow-phase tests between sphere and convex polyhedron shapes. */
        NarrowPhaseDataBatch _sphereVsConvexPolyhedronBatch;

        /** @brief Batch of narrow-phase tests between capsule and convex polyhedron shapes. */
        NarrowPhaseDataBatch _capsuleVsConvexPolyhedronBatch;

        /** @brief Batch of narrow-phase tests between two convex polyhedron shapes. */
        NarrowPhaseDataBatch _convexPolyhedronVsConvexPolyhedronBatch;
    };

} // namespace Vulkyrie
