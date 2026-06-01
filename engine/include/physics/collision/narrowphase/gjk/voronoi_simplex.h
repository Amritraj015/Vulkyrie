#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    class VoronoiSimplex final {
    public:
        VoronoiSimplex();

        VoronoiSimplex(const VoronoiSimplex &) = delete;
        VoronoiSimplex &operator=(const VoronoiSimplex &) = delete;

        VoronoiSimplex(VoronoiSimplex &&) = delete;
        VoronoiSimplex &operator=(VoronoiSimplex &&) = delete;

        ~VoronoiSimplex() = default;

        bool IsFull() const;
        bool IsEmpty() const;
        size_t GetSimplex(glm::vec3 *supportPointsOne, glm::vec3 *supportPointsTwo, glm::vec3 *vertices) const;
        f32 GetMaxLengthSquareOfAPoint() const;
        void AddPoint(const glm::vec3 &point, const glm::vec3 &suppPointA, const glm::vec3 &suppPointB);
        void RemovePoint(size_t index);
        void ReduceSimplex(i32 bitsUsedPoints);
        bool IsPointInSimplex(const glm::vec3 &point) const;
        bool IsAffinelyDependent() const;
        void BackupClosestPointInSimplex(glm::vec3 &point);
        void ComputeClosestPointsOfAandB(glm::vec3 &pA, glm::vec3 &pB) const;
        bool ComputeClosestPoint(glm::vec3 &v);

    private:
        std::array<glm::vec3, 4> _vertices;
        std::array<glm::vec3, 4> _barycentricCoordinates;
        std::array<glm::vec3, 4> _pointsLengthSquare;
        std::array<glm::vec3, 4> _supportPointsOnShapeOne;
        std::array<glm::vec3, 4> _supportPointsOnShapeTwo;
        glm::vec3 _closestPoint;
        glm::vec3 _closestSupportPointOnShapeOne;
        glm::vec3 _closestSupportPointOnShapeTwo;

        size_t _vertexCount;
        bool _recomputeClosestPoint;
        bool _closestPointValid;

        constexpr static f32 EPSILON = f32(0.0001);

        void setBarycentricCoords(f32 a, f32 b, f32 c, f32 d);
        bool recomputeClosestPoint();
        bool checkClosestPointValid() const;
        void computeClosestPointOnSegment(const glm::vec3 &a, const glm::vec3 &b, int &bitUsedVertices, f32 &t) const;
        void computeClosestPointOnTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, int &bitsUsedPoints, glm::vec3 &baryCoordsABC) const;
        bool computeClosestPointOnTetrahedron(const glm::vec3 &a,
                                              const glm::vec3 &b,
                                              const glm::vec3 &c,
                                              const glm::vec3 &d,
                                              int &bitsUsedPoints,
                                              glm::vec2 &baryCoordsAB,
                                              glm::vec2 &baryCoordsCD,
                                              bool &isDegenerate) const;
        int testOriginOutsideOfPlane(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d) const;
        int mapTriangleUsedVerticesToTetrahedron(int triangleUsedVertices, int first, int second, int third) const;
    };

} // namespace Vulkyrie
