#pragma once

#include "vlkypch.h"
#include "physics/types/narrow_phase_algorithm.h"
#include "physics/collision/narrowphase/capsule_vs_capsule_algorithm.h"
#include "physics/collision/narrowphase/capsule_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/convex_polyhedron_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_capsule_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_sphere_algorithm.h"

namespace Vulkyrie {

    /** @brief Responsible for dispatching the correct narrow-phase collision detection algorithm based on the types of the two shapes involved in a potential
     * collision.
     *
     * The CollisionDispatch class maintains a collision matrix that maps pairs of CollisionShapeType to the appropriate NarrowPhaseAlgorithm. It also holds
     * instances of each narrow-phase algorithm implementation, which can be used by the collision system when processing overlapping pairs. The
     * SelectNarrowPhaseAlgorithm method is used to query the matrix and retrieve the correct algorithm for any given pair of shape types.
     */
    class CollisionDispatch {
    public:
        /** @brief Constructs the CollisionDispatch, initializing the collision matrix with the appropriate algorithms. */
        CollisionDispatch();

        VE_DELETE_MOVE_AND_COPY(CollisionDispatch);

        /** @brief Default destructor for CollisionDispatch. */
        ~CollisionDispatch() = default;

        /** @brief Selects the appropriate narrow-phase algorithm for the given pair of collision shape types.
         *
         * @param shapeOne The type of the first collision shape in the pair.
         * @param shapeTwo The type of the second collision shape in the pair.
         * @returns The narrow-phase algorithm to use for this pair of shape types, as determined by the collision matrix.
         */
        NarrowPhaseAlgorithm SelectNarrowPhaseAlgorithm(CollisionShapeType shapeOne, CollisionShapeType shapeTwo) const;

        [[nodiscard]] VE_INLINE CapsuleVsCapsuleAlgorithm &GetCapsuleVsCapsuleAlgorithm() {
            return _capsuleVsCapsuleAlgorithm;
        }

        [[nodiscard]] VE_INLINE CapsuleVsConvexPolyhedronAlgorithm &GetCapsuleVsConvexPolyhedronAlgorithm() {
            return _capsuleVsConvexPolyhedronAlgorithm;
        }

        [[nodiscard]] VE_INLINE ConvexPolyhedronVsConvexPolyhedronAlgorithm &GetConvexPolyhedronVsConvexPolyhedronAlgorithm() {
            return _convexPolyhedronVsConvexPolyhedronAlgorithm;
        }

        [[nodiscard]] VE_INLINE SphereVsCapsuleAlgorithm &GetSphereVsCapsuleAlgorithm() {
            return _sphereVsCapsuleAlgorithm;
        }

        [[nodiscard]] VE_INLINE SphereVsConvexPolyhedronAlgorithm &GetSphereVsConvexPolyhedronAlgorithm() {
            return _sphereVsConvexPolyhedronAlgorithm;
        }

        [[nodiscard]] VE_INLINE SphereVsSphereAlgorithm &GetSphereVsSphereAlgorithm() {
            return _sphereVsSphereAlgorithm;
        }

    private:
        /** @brief A 2D array mapping pairs of CollisionShapeType indices to their corresponding NarrowPhaseAlgorithm. This matrix is initialized in the
         * constructor and used by SelectNarrowPhaseAlgorithm to quickly determine the correct algorithm for any pair of shape types. The indices correspond to
         * the enum values of CollisionShapeType, and the matrix is symmetric (i.e., the algorithm for (shapeOne, shapeTwo) is the same as for (shapeTwo,
         * shapeOne)). */
        NarrowPhaseAlgorithm _collisionMatrix[SUPPORTED_COLLISION_SHAPE_TYPE_COUNT][SUPPORTED_COLLISION_SHAPE_TYPE_COUNT];

        /** @brief Instance of the capsule-vs-capsule narrow-phase algorithm, used for collision checks between pairs of capsule shapes. */
        CapsuleVsCapsuleAlgorithm _capsuleVsCapsuleAlgorithm;

        /** @brief Instance of the capsule-vs-convex-polyhedron narrow-phase algorithm, used for collision checks between pairs of capsule and convex polyhedron
         * shapes. */
        CapsuleVsConvexPolyhedronAlgorithm _capsuleVsConvexPolyhedronAlgorithm;

        /** @brief Instance of the convex-polyhedron-vs-convex-polyhedron narrow-phase algorithm, used for collision checks between pairs of convex polyhedron
         * shapes. */
        ConvexPolyhedronVsConvexPolyhedronAlgorithm _convexPolyhedronVsConvexPolyhedronAlgorithm;

        /** @brief Instance of the sphere-vs-capsule narrow-phase algorithm, used for collision checks between pairs of sphere and capsule shapes. */
        SphereVsCapsuleAlgorithm _sphereVsCapsuleAlgorithm;

        /** @brief Instance of the sphere-vs-convex-polyhedron narrow-phase algorithm, used for collision checks between pairs of sphere and convex polyhedron
         * shapes. */
        SphereVsConvexPolyhedronAlgorithm _sphereVsConvexPolyhedronAlgorithm;

        /** @brief Instance of the sphere-vs-sphere narrow-phase algorithm, used for collision checks between pairs of sphere shapes. */
        SphereVsSphereAlgorithm _sphereVsSphereAlgorithm;

        /** @brief Helper function to determine the narrow-phase algorithm for a given pair of shape type indices.
         *
         * @param shapeOne The index of the first shape type (corresponding to CollisionShapeType enum values).
         * @param shapeTwo The index of the second shape type (corresponding to CollisionShapeType enum values).
         * @returns The narrow-phase algorithm to use for this pair of shape type indices, used to populate the collision matrix during initialization. */
        static NarrowPhaseAlgorithm selectAlgorithm(i32 shapeOne, i32 shapeTwo);
    };

} // namespace Vulkyrie
