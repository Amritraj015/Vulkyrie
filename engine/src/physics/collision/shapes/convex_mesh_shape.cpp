#include "physics/collision/shapes/convex_mesh_shape.h"
#include "core/constants.h"
#include "core/utilities.h"

namespace Vulkyrie {

    ConvexMeshShape::ConvexMeshShape(ConvexMesh *convexMesh, const glm::vec3 &scale)
        : ConvexPolyhedronShape(CollisionShapeName::ConvexMesh)
        , _convexMesh(convexMesh)
        , _scale(scale) {

        _scaledFaceNormals.reserve(convexMesh->GetFacesCount());

        computeScaledFaceNormals();
    }

    glm::vec3 ConvexMeshShape::GetLocalSupportPointWithoutMargin(const glm::vec3 &direction) const {
        f32 maxDotProduct = VE_DECIMAL_MIN;
        size_t indexMaxDotProduct = 0;

        for (size_t i = 0; i < _convexMesh->GetVerticesCount(); i++) {
            f32 dotProduct = glm::dot(direction, _convexMesh->GetVertex(i));

            if (dotProduct > maxDotProduct) {
                indexMaxDotProduct = i;
                maxDotProduct = dotProduct;
            }
        }

        VASSERT(maxDotProduct >= f32(0.0), "maxDotProduct must be >= f32(0.0)");

        return _convexMesh->GetVertex(indexMaxDotProduct) * _scale;
    }

    bool ConvexMeshShape::ContainsPoint(const glm::vec3 &point) const {
        const HalfEdgeMesh &halfEdgeStructure = _convexMesh->GetHalfEdgeMesh();

        for (size_t f = 0; f < _convexMesh->GetFacesCount(); f++) {

            const HalfEdgeMesh::Face &face = halfEdgeStructure.GetFace(f);
            const glm::vec3 &faceNormal = GetFaceNormal(f);

            const HalfEdgeMesh::Vertex &faceVertex = halfEdgeStructure.GetVertex(face.FaceVertices[0]);
            const glm::vec3 &facePoint = _convexMesh->GetVertex(faceVertex.VertexIndex);

            if (ComputePointToPlaneDistance(point, faceNormal, facePoint) > f32(0.0)) return false;
        }

        return true;
    }

    void ConvexMeshShape::computeScaledFaceNormals() {
        _scaledFaceNormals.clear();

        for (size_t f = 0; f < _convexMesh->GetFacesCount(); f++) {
            glm::vec3 normal = _convexMesh->GetFaceNormal(f);

            normal = (f32(1.0) / _scale) * normal;

            const f32 normalLength = glm::length(normal);
            VASSERT(VE_MACHINE_EPSILON < normalLength, "Normal length must be greater than VE_MACHINE_EPSILON.");

            normal /= normalLength;

            _scaledFaceNormals.push_back(normal);
        }
    }

} // namespace Vulkyrie
