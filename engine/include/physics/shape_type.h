#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Physics {
    enum class ShapeType : u32 {
        Box = 0,
        Sphere = 1,
        Capsule = 2,
        Mesh = 3,
        SoftBody = 4,
    };
}
