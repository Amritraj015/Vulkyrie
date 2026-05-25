#include "physics/types/convex_mesh.h"

namespace Vulkyrie {

    ConvexMesh::ConvexMesh()
        : _halfEdgeMesh(6, 8, 24)
        , _volume(0.0f) {
    }

    glm::vec3 ConvexMesh::computeFaceNormal(size_t faceIndex) const {
        glm::vec3 normal(0);

        const HalfEdgeMesh::Face &face = _halfEdgeMesh.GetFace(faceIndex);
        VASSERT(face.FaceVertices.size() >= 3, "A face must have at least 3 vertices to compute a normal.");

        // Use Newell's method to compute the face normal
        for (size_t i = face.FaceVertices.size() - 1, j = 0; j < face.FaceVertices.size(); i = j, j++) {

            const glm::vec3 &v1 = GetVertex(face.FaceVertices[i]);
            const glm::vec3 &v2 = GetVertex(face.FaceVertices[j]);

            normal += glm::vec3((v1.y - v2.y) * (v1.z + v2.z), (v1.z - v2.z) * (v1.x + v2.x), (v1.x - v2.x) * (v1.y + v2.y));
        }

        return normal;
    }

    void ConvexMesh::computeVolume() {
        f32 sum(0.0f);

        // For each face of the mesh
        for (size_t f = 0; f < GetFacesCount(); f++) {

            const HalfEdgeMesh::Face &face = _halfEdgeMesh.GetFace(f);
            const f32 faceArea = glm::length(computeFaceNormal(f)) * f32(0.5);
            const glm::vec3 faceNormal = _faceNormals[f];
            const glm::vec3 &faceVertex = GetVertex(face.FaceVertices[0]);

            sum += glm::dot(faceVertex, faceNormal) * faceArea;
        }

        _volume = std::abs(sum) / f32(3.0);
    }

} // namespace Vulkyrie
