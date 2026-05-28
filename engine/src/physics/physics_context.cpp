#include "physics/physics_context.h"

namespace Vulkyrie {

    PhysicsContext::PhysicsContext()
        : _boxShapeHalfEdgeMesh(6, 8, 24)
        , _triangleShapeHalfEdgeMesh(2, 3, 6) {
    }
} // namespace Vulkyrie
