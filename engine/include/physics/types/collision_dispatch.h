#pragma once

#include "physics/types/narrow_phase_algorithm.h"
#include "physics/collision/narrowphase/capsule_vs_capsule_algorithm.h"
#include "physics/collision/narrowphase/capsule_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/convex_polyhedron_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_capsule_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_convex_polyhedron_algorithm.h"
#include "physics/collision/narrowphase/sphere_vs_sphere_algorithm.h"

namespace Vulkyrie {

    class CollisionDispatch {
    public:
        CollisionDispatch();

        CollisionDispatch(const CollisionDispatch &) = delete;
        CollisionDispatch &operator=(const CollisionDispatch &) = delete;

        CollisionDispatch(CollisionDispatch &&) = delete;
        CollisionDispatch &operator=(CollisionDispatch &&) = delete;

        ~CollisionDispatch() = default;

        NarrowPhaseAlgorithm SelectNarrowPhaseAlgorithm(const CollisionShapeType shapeOne, const CollisionShapeType shapeTwo) const;

    private:
        NarrowPhaseAlgorithm _collisionMatrix[SUPPORTED_COLLISION_SHAPE_TYPE_COUNT][SUPPORTED_COLLISION_SHAPE_TYPE_COUNT];
        CapsuleVsCapsuleAlgorithm _capsuleVsCapsuleAlgorithm;
        CapsuleVsConvexPolyhedronAlgorithm _capsuleVsConvexPolyhedronAlgorithm;
        ConvexPolyhedronVsConvexPolyhedronAlgorithm _convexPolyhedronVsConvexPolyhedronAlgorithm;
        SphereVsCapsuleAlgorithm _sphereVsCapsuleAlgorithm;
        SphereVsConvexPolyhedronAlgorithm _sphereVsConvexPolyhedronAlgorithm;
        SphereVsSphereAlgorithm _sphereVsSphereAlgorithm;

        NarrowPhaseAlgorithm selectAlgorithm(i32 shapeOne, i32 shapeTwo);
    };

} // namespace Vulkyrie
