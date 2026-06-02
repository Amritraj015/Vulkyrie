#pragma once

#include "vlkypch.h"

namespace Vulkyrie {

    /**
     * @class VoronoiSimplex
     * @brief Represents a simplex (a set of up to 4 points) used in the GJK collision detection algorithm.
     *
     * This class implements a Voronoi simplex which is used to compute the point on the simplex that is
     * closest to the origin. The implementation is based on Christer Ericson's "Real-Time Collision Detection"
     * and replaces Johnson's algorithm for computing the closest point and the minimal simplex needed to
     * represent that point.
     *
     * A simplex can contain:
     * - 0 points: Empty simplex
     * - 1 point: A single vertex
     * - 2 points: A line segment
     * - 3 points: A triangle
     * - 4 points: A tetrahedron
     *
     * The class maintains support points from two shapes (A and B) and computes the Minkowski difference (A - B).
     */
    class VoronoiSimplex final {
    public:
        /**
         * @brief Constructor for VoronoiSimplex.
         *
         * Initializes an empty simplex with zero vertices and sets the flags for recomputing
         * the closest point and its validity.
         */
        VoronoiSimplex();

        // Delete the copy constructor and the copy assignment operator.
        VoronoiSimplex(const VoronoiSimplex &) = delete;
        VoronoiSimplex &operator=(const VoronoiSimplex &) = delete;

        // Delete the move constructor and the move assignment operator.
        VoronoiSimplex(VoronoiSimplex &&) = delete;
        VoronoiSimplex &operator=(VoronoiSimplex &&) = delete;

        /**
         * @brief Destructor for VoronoiSimplex.
         */
        ~VoronoiSimplex() = default;

        /**
         * @brief Checks if the simplex is full (contains 4 vertices).
         *
         * A full simplex represents a tetrahedron and cannot accept more vertices.
         *
         * @return true if the simplex contains 4 vertices, false otherwise
         */
        [[nodiscard]] VE_INLINE bool IsFull() const {
            return _vertexCount == 4;
        }

        /**
         * @brief Checks if the simplex is empty (contains no vertices).
         *
         * @return true if the simplex contains 0 vertices, false otherwise
         */
        [[nodiscard]] VE_INLINE bool IsEmpty() const {
            return _vertexCount == 0;
        }

        /**
         * @brief Computes the point on the simplex closest to the origin.
         *
         * This method determines which subset of the simplex vertices contributes to the
         * closest point using Voronoi region analysis. The simplex is automatically reduced
         * to only include the vertices that participate in the closest point computation.
         *
         * @param[out] v The computed closest point on the simplex
         * @return true if a valid closest point was computed, false otherwise (e.g., degenerate simplex)
         */
        [[nodiscard]] VE_INLINE bool ComputeClosestPoint(glm::vec3 &v) {
            bool isValid = recomputeClosestPoint();
            v = _closestPoint;
            return isValid;
        }

        /**
         * @brief Retrieves the current simplex vertices and support points.
         *
         * @param[out] supportPointsOne Support points from shape A
         * @param[out] supportPointsTwo Support points from shape B
         * @param[out] vertices Simplex vertices (Minkowski difference points: A - B)
         * @return The number of vertices currently in the simplex
         */
        size_t GetSimplex(glm::vec3 *supportPointsOne, glm::vec3 *supportPointsTwo, glm::vec3 *vertices) const;

        /**
         * @brief Gets the maximum squared length of any vertex in the simplex.
         *
         * This is useful for determining the furthest distance from the origin to any
         * point in the simplex, which can be used for convergence checks in GJK.
         *
         * @return The maximum squared distance from the origin to any simplex vertex
         */
        f32 GetMaxLengthSquareOfAPoint() const;

        /**
         * @brief Adds a new point to the simplex.
         *
         * The point is typically obtained from a support function query in the GJK algorithm.
         * This method will assert if the simplex is already full (contains 4 vertices).
         *
         * @param point The vertex in Minkowski difference space (suppPointA - suppPointB)
         * @param suppPointA Support point from shape A
         * @param suppPointB Support point from shape B
         */
        void AddPoint(const glm::vec3 &point, const glm::vec3 &suppPointA, const glm::vec3 &suppPointB);

        /**
         * @brief Removes a vertex from the simplex at the specified index.
         *
         * This is done by swapping the vertex at the given index with the last vertex
         * and decrementing the vertex count. This method will assert if the simplex is empty.
         *
         * @param index The index of the vertex to remove (0-3)
         */
        void RemovePoint(size_t index);

        /**
         * @brief Reduces the simplex by removing vertices that don't contribute to the closest point.
         *
         * The bitsUsedPoints parameter is a bitmask where each bit indicates whether the
         * corresponding vertex is used in the closest point calculation:
         * - Bit 0 (0x1): Vertex 0 is used
         * - Bit 1 (0x2): Vertex 1 is used
         * - Bit 2 (0x4): Vertex 2 is used
         * - Bit 3 (0x8): Vertex 3 is used
         *
         * Vertices with their corresponding bit set to 0 are removed from the simplex.
         *
         * @param bitsUsedPoints Bitmask indicating which vertices participate in the closest point
         */
        void ReduceSimplex(i32 bitsUsedPoints);

        /**
         * @brief Checks if a given point already exists in the simplex.
         *
         * A point is considered to be in the simplex if it's within EPSILON distance
         * of any existing vertex. This prevents adding duplicate or nearly-duplicate points.
         *
         * @param point The point to check
         * @return true if the point is already in the simplex, false otherwise
         */
        bool IsPointInSimplex(const glm::vec3 &point) const;

        /**
         * @brief Checks if the simplex vertices are affinely dependent.
         *
         * A set of points is affinely dependent if one point can be expressed as an affine
         * combination of the others. This happens when:
         * - 2 points are very close together (degenerate line)
         * - 3 points are collinear (degenerate triangle with zero area)
         * - 4 points are coplanar (degenerate tetrahedron with zero volume)
         *
         * @return true if the simplex is affinely dependent (degenerate), false otherwise
         */
        bool IsAffinelyDependent() const;

        /**
         * @brief Backs up the current closest point to the provided variable.
         *
         * This is useful for preserving the closest point before modifying the simplex.
         *
         * @param[out] point Variable to store the current closest point
         */
        void BackupClosestPointInSimplex(glm::vec3 &point);

        /**
         * @brief Computes the closest points on shapes A and B.
         *
         * These points are computed using the barycentric coordinates from the closest
         * point calculation, applied to the support points from each shape:
         * - pA = sum(lambda_i * supportPointsA_i)
         * - pB = sum(lambda_i * supportPointsB_i)
         *
         * where lambda_i are the barycentric coordinates.
         *
         * @param[out] pA Closest point on shape A
         * @param[out] pB Closest point on shape B
         */
        void ComputeClosestPointsOfAandB(glm::vec3 &pA, glm::vec3 &pB) const;

    private:
        /// Simplex vertices in Minkowski difference space (A - B)
        std::array<glm::vec3, 4> _vertices;

        /// Support points from shape A corresponding to each vertex
        std::array<glm::vec3, 4> _supportPointsOnShapeOne;

        /// Support points from shape B corresponding to each vertex
        std::array<glm::vec3, 4> _supportPointsOnShapeTwo;

        /// Barycentric coordinates of the closest point using simplex vertices
        std::array<f32, 4> _barycentricCoordinates;

        /// Squared length of each vertex (cached for performance)
        std::array<f32, 4> _pointsLengthSquare;

        /// Current point on the simplex closest to the origin
        glm::vec3 _closestPoint;

        /// Closest support point on shape A (computed using barycentric coords)
        glm::vec3 _closestSupportPointOnShapeOne;

        /// Closest support point on shape B (computed using barycentric coords)
        glm::vec3 _closestSupportPointOnShapeTwo;

        /// Number of vertices currently in the simplex (0-4)
        size_t _vertexCount;

        /// Flag indicating whether the closest point needs to be recomputed
        bool _recomputeClosestPoint;

        /// Flag indicating whether the last computed closest point is valid
        bool _closestPointValid;

        /// Epsilon value for numerical comparisons
        constexpr static f32 EPSILON = f32(0.0001);

        /**
         * @brief Sets the barycentric coordinates for the closest point.
         *
         * @param a Barycentric coordinate for vertex 0
         * @param b Barycentric coordinate for vertex 1
         * @param c Barycentric coordinate for vertex 2
         * @param d Barycentric coordinate for vertex 3
         */
        VE_INLINE void setBarycentricCoords(f32 a, f32 b, f32 c, f32 d) {
            _barycentricCoordinates[0] = a;
            _barycentricCoordinates[1] = b;
            _barycentricCoordinates[2] = c;
            _barycentricCoordinates[3] = d;
        }

        /**
         * @brief Validates the barycentric coordinates.
         *
         * Valid barycentric coordinates must all be non-negative. This is used to verify
         * that the computed closest point is indeed valid.
         *
         * @return true if all barycentric coordinates are >= 0, false otherwise
         */
        [[nodiscard]] VE_INLINE bool checkClosestPointValid() const {
            return _barycentricCoordinates[0] >= 0.0f && _barycentricCoordinates[1] >= 0.0f && _barycentricCoordinates[2] >= 0.0f &&
                   _barycentricCoordinates[3] >= 0.0f;
        }

        /**
         * @brief Recomputes the closest point if the simplex has been modified.
         *
         * This is the core method that dispatches to the appropriate computation based on
         * the number of vertices in the simplex (point, segment, triangle, or tetrahedron).
         *
         * @return true if a valid closest point was computed, false for degenerate cases
         */
        bool recomputeClosestPoint();

        /**
         * @brief Computes the point on a line segment closest to the origin.
         *
         * Uses Voronoi region analysis to determine if the closest point is vertex A,
         * vertex B, or somewhere on the segment AB.
         *
         * @param a First vertex of the segment
         * @param b Second vertex of the segment
         * @param[out] bitUsedVertices Bitmask indicating which vertices are used (1=A, 2=B, 3=both)
         * @param[out] t Parametric value [0,1] of the closest point on segment AB
         */
        void computeClosestPointOnSegment(const glm::vec3 &a, const glm::vec3 &b, i32 &bitUsedVertices, f32 &t) const;

        /**
         * @brief Computes the point on a triangle closest to the origin.
         *
         * Uses Voronoi region analysis to determine which feature of the triangle
         * (vertex, edge, or face) contains the closest point to the origin.
         *
         * @param a First vertex of the triangle
         * @param b Second vertex of the triangle
         * @param c Third vertex of the triangle
         * @param[out] bitsUsedPoints Bitmask indicating which vertices are used
         * @param[out] barycentricCoordsABC Barycentric coordinates of the closest point
         */
        void computeClosestPointOnTriangle(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, i32 &bitsUsedPoints, glm::vec3 &barycentricCoordsABC) const;

        /**
         * @brief Computes the point on a tetrahedron closest to the origin.
         *
         * Tests if the origin is outside any of the four triangular faces and finds
         * the face (or feature of a face) that is closest to the origin.
         *
         * @param a First vertex of the tetrahedron
         * @param b Second vertex of the tetrahedron
         * @param c Third vertex of the tetrahedron
         * @param d Fourth vertex of the tetrahedron
         * @param[out] bitsUsedPoints Bitmask indicating which vertices are used
         * @param[out] barycentricCoordsAB Barycentric coordinates for vertices A and B
         * @param[out] barycentricCoordsCD Barycentric coordinates for vertices C and D
         * @param[out] isDegenerate Set to true if the tetrahedron is degenerate (coplanar)
         * @return true if the origin is outside the tetrahedron, false if inside
         */
        bool computeClosestPointOnTetrahedron(const glm::vec3 &a,
                                              const glm::vec3 &b,
                                              const glm::vec3 &c,
                                              const glm::vec3 &d,
                                              i32 &bitsUsedPoints,
                                              glm::vec2 &barycentricCoordsAB,
                                              glm::vec2 &barycentricCoordsCD,
                                              bool &isDegenerate) const;
        /**
         * @brief Tests if the origin is outside a plane defined by a triangle.
         *
         * Determines if the origin and point d are on opposite sides of the plane
         * defined by triangle (a, b, c).
         *
         * @param a First vertex of the triangle
         * @param b Second vertex of the triangle
         * @param c Third vertex of the triangle
         * @param d Test point (usually the 4th vertex of a tetrahedron)
         * @return 1 if origin and d are on opposite sides, 0 if same side, -1 if degenerate
         */
        i32 testOriginOutsideOfPlane(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d) const;

        /**
         * @brief Maps triangle vertex usage bits to tetrahedron vertex usage bits.
         *
         * When computing the closest point on a triangular face of a tetrahedron,
         * we need to remap which triangle vertices (0,1,2) correspond to which
         * tetrahedron vertices (0,1,2,3).
         *
         * @param triangleUsedVertices Bitmask of triangle vertices used (0-7)
         * @param first Tetrahedron vertex index corresponding to triangle vertex 0
         * @param second Tetrahedron vertex index corresponding to triangle vertex 1
         * @param third Tetrahedron vertex index corresponding to triangle vertex 2
         * @return Bitmask of tetrahedron vertices used
         */
        i32 mapTriangleUsedVerticesToTetrahedron(i32 triangleUsedVertices, i32 first, i32 second, i32 third) const;
    };

} // namespace Vulkyrie
