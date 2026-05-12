#pragma once

namespace Vulkyrie {

    enum class NarrowPhaseAlgorithm : i32 {
        NoCollisionCheck,
        SphereVsSphere,
        SphereVsCapsule,
        CapsuleVsCapsule,
        SphereVsConvexPolyhedron,
        CapsuleVsConvexPolyhedron,
        ConvexPolyhedronVsConvexPolyhedron
    };

    class CollisionDispatcher final {
        public:
            CollisionDispatcher() = default;

            CollisionDispatcher(const CollisionDispatcher &) = delete;
            CollisionDispatcher &operator=(const CollisionDispatcher &) = delete;

            CollisionDispatcher(CollisionDispatcher &&) = delete;
            CollisionDispatcher &operator=(CollisionDispatcher &&) = delete;

            ~CollisionDispatcher() = default;

            void SetSphereVsSphereAlgorithm(size_t pairID);
    };

} // namespace Vulkyrie
