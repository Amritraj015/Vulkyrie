#pragma once

#include "vlkypch.h"

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

} // namespace Vulkyrie
