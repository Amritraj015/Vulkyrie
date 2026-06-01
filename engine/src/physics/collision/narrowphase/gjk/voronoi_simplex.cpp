#include "physics/collision/narrowphase/gjk/voronoi_simplex.h"
#include "core/asserts.h"

namespace Vulkyrie {

    VoronoiSimplex::VoronoiSimplex()
        : _vertexCount(0)
        , _recomputeClosestPoint(false)
        , _closestPointValid(false) {
    }

    bool VoronoiSimplex::IsFull() const {
        return _vertexCount == 4;
    }

    bool VoronoiSimplex::IsEmpty() const {
        return _vertexCount == 0;
    }

    size_t VoronoiSimplex::GetSimplex(glm::vec3 *supportPointsOne, glm::vec3 *supportPointsTwo, glm::vec3 *vertices) const {
        for (size_t i = 0; i < _vertexCount; i++) {
            supportPointsOne[i] = _supportPointsOnShapeOne[i];
            supportPointsTwo[i] = _supportPointsOnShapeTwo[i];
            vertices[i] = _vertices[i];
        }

        return _vertexCount;
    }

    f32 VoronoiSimplex::GetMaxLengthSquareOfAPoint() const {
        f32 maxLengthSquare = 0.0f;

        // Iterate through the vertices of the simplex and compute the squared length of each vertex.
        // Keep track of the maximum squared length found.
        for (size_t i = 0; i < _vertexCount; i++) {
            f32 lengthSquare = glm::length2(_vertices[i]);

            if (lengthSquare > maxLengthSquare) {
                maxLengthSquare = lengthSquare;
            }
        }

        return maxLengthSquare;
    }

    void VoronoiSimplex::AddPoint(const glm::vec3 &point, const glm::vec3 &suppPointA, const glm::vec3 &suppPointB) {
        VASSERT(!IsFull(), "Cannot add point to simplex: simplex is already full with 4 vertices.");

        _vertices[_vertexCount] = point;
        _supportPointsOnShapeOne[_vertexCount] = suppPointA;
        _supportPointsOnShapeTwo[_vertexCount] = suppPointB;

        _vertexCount++;
        _recomputeClosestPoint = true;
    }

    void VoronoiSimplex::RemovePoint(size_t index) {
        VASSERT(_vertexCount > 0, "Cannot remove point from simplex: simplex is already empty.");

        _vertexCount--;
        _vertices[index] = _vertices[_vertexCount];
        _supportPointsOnShapeOne[index] = _supportPointsOnShapeOne[_vertexCount];
        _supportPointsOnShapeTwo[index] = _supportPointsOnShapeTwo[_vertexCount];
    }

    void VoronoiSimplex::ReduceSimplex(i32 bitsUsedPoints) {
        if ((_vertexCount >= 4 && bitsUsedPoints & 8) == 0) {
            RemovePoint(3);
        }

        if ((_vertexCount >= 3 && bitsUsedPoints & 4) == 0) {
            RemovePoint(2);
        }

        if ((_vertexCount >= 2 && bitsUsedPoints & 2) == 0) {
            RemovePoint(1);
        }

        if ((_vertexCount >= 1 && bitsUsedPoints & 1) == 0) {
            RemovePoint(0);
        }
    }

    bool VoronoiSimplex::IsPointInSimplex(const glm::vec3 &point) const {
        // For each point in the simplex,
        // check if it's within a small distance of the given point.
        // If so, we can consider it "in" the simplex.
        for (size_t i = 0; i < _vertexCount; i++) {
            f32 distanceSquared = glm::length2(_vertices[i] - point);

            if (distanceSquared <= VoronoiSimplex::EPSILON) {
                return true;
            }
        }

        return false;
    }

    bool VoronoiSimplex::IsAffinelyDependent() const {
    }

    void VoronoiSimplex::BackupClosestPointInSimplex(glm::vec3 &point) {
        point = _closestPoint;
    }

    void VoronoiSimplex::ComputeClosestPointsOfAandB(glm::vec3 &pA, glm::vec3 &pB) const {
        pA = _closestSupportPointOnShapeOne;
        pB = _closestSupportPointOnShapeTwo;
    }

    bool VoronoiSimplex::ComputeClosestPoint(glm::vec3 &v) {
    }

    // void VoronoiSimplex::setBarycentricCoords(f32 a, f32 b, f32 c, f32 d);
    // bool VoronoiSimplex::recomputeClosestPoint();
    // bool VoronoiSimplex::checkClosestPointValid() const;
    // void VoronoiSimplex::computeClosestPointOnSegment(const glm::vec3 &a, const glm::vec3 &b, int &bitUsedVertices, float &t) const;
    // void VoronoiSimplex::computeClosestPointOnTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, int &bitsUsedPoints, glm::vec3
    // &baryCoordsABC) const; bool VoronoiSimplex::computeClosestPointOnTetrahedron(const glm::vec3 &a,
    //                                       const glm::vec3 &b,
    //                                       const glm::vec3 &c,
    //                                       const glm::vec3 &d,
    //                                       int &bitsUsedPoints,
    //                                       glm::vec2 &baryCoordsAB,
    //                                       glm::vec2 &baryCoordsCD,
    //                                       bool &isDegenerate) const;
    // int VoronoiSimplex::testOriginOutsideOfPlane(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d) const;
    // int VoronoiSimplex::mapTriangleUsedVerticesToTetrahedron(int triangleUsedVertices, int first, int second, int third) const;

} // namespace Vulkyrie
