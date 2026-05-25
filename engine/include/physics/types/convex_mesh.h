#pragma once

#include "vlkypch.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/types/half_edge_mesh.h"

namespace Vulkyrie {

    class ConvexMesh {
    public:
    private:
        HalfEdgeMesh _halfEdgeMesh;
        std::vector<glm::vec3> _vertices;
        std::vector<glm::vec3> _faceNormals;
        glm::vec3 _centroid;
        AABB _bounds;
        f32 _volume;
    };

} // namespace Vulkyrie
