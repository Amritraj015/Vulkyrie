#include "physics/collision/narrowphase/gjk/voronoi_simplex.h"
#include "core/asserts.h"
#include "core/constants.h"

namespace Vulkyrie {

    VoronoiSimplex::VoronoiSimplex()
        : _vertexCount(0)
        , _recomputeClosestPoint(false)
        , _closestPointValid(false) {
    }

    size_t VoronoiSimplex::GetSimplex(glm::vec3 *supportPointsOne, glm::vec3 *supportPointsTwo, glm::vec3 *vertices) const {
        // Copy all current simplex data to the output arrays
        for (size_t i = 0; i < _vertexCount; i++) {
            supportPointsOne[i] = _supportPointsOnShapeOne[i];
            supportPointsTwo[i] = _supportPointsOnShapeTwo[i];
            vertices[i] = _vertices[i];
        }

        // Return how many vertices are in the simplex
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

        // Store the new vertex and its corresponding support points
        _vertices[_vertexCount] = point;
        _supportPointsOnShapeOne[_vertexCount] = suppPointA;
        _supportPointsOnShapeTwo[_vertexCount] = suppPointB;

        // Increment vertex count and mark closest point for recomputation
        _vertexCount++;
        _recomputeClosestPoint = true;
    }

    void VoronoiSimplex::RemovePoint(size_t index) {
        VASSERT(_vertexCount > 0, "Cannot remove point from simplex: simplex is already empty.");

        // Decrement vertex count first
        _vertexCount--;

        // Move the last vertex to the index being removed (swap and pop approach)
        // This is more efficient than shifting all elements
        _vertices[index] = _vertices[_vertexCount];
        _supportPointsOnShapeOne[index] = _supportPointsOnShapeOne[_vertexCount];
        _supportPointsOnShapeTwo[index] = _supportPointsOnShapeTwo[_vertexCount];
    }

    void VoronoiSimplex::ReduceSimplex(i32 bitsUsedPoints) {
        // Remove vertices in reverse order (from highest index to lowest)
        // to avoid index shifting issues during removal

        // Check if vertex 3 should be removed (bit 3 not set)
        if (_vertexCount >= 4 && (bitsUsedPoints & 8) == 0) {
            RemovePoint(3);
        }

        // Check if vertex 2 should be removed (bit 2 not set)
        if (_vertexCount >= 3 && (bitsUsedPoints & 4) == 0) {
            RemovePoint(2);
        }

        // Check if vertex 1 should be removed (bit 1 not set)
        if (_vertexCount >= 2 && (bitsUsedPoints & 2) == 0) {
            RemovePoint(1);
        }

        // Check if vertex 0 should be removed (bit 0 not set)
        if (_vertexCount >= 1 && (bitsUsedPoints & 1) == 0) {
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
        VASSERT(_vertexCount <= 4, "Invalid simplex: vertex count cannot exceed 4.");

        switch (_vertexCount) {
            case 0:
            case 1:
                return false; // A simplex with 0 or 1 vertex is always affinely independent.
            case 2:
                // Two points are affinely dependent if they are very close to each other.
                return glm::length2(_vertices[1] - _vertices[0]) <= VoronoiSimplex::EPSILON;
            case 3: {
                glm::vec3 AB = _vertices[1] - _vertices[0];
                glm::vec3 AC = _vertices[2] - _vertices[0];
                glm::vec3 normal = glm::cross(AB, AC);

                // Three points are affinely dependent if they are collinear (the normal of the triangle they form is very small).
                return glm::length2(normal) <= VoronoiSimplex::EPSILON;
            }
            case 4: {
                glm::vec3 AB = _vertices[1] - _vertices[0];
                glm::vec3 AC = _vertices[2] - _vertices[0];
                glm::vec3 AD = _vertices[3] - _vertices[0];

                // The volume of the tetrahedron formed by the four points is proportional to the
                // dot product of the normal of the base triangle and the vector to the fourth point.
                glm::vec3 normal = glm::cross(AB, AC);
                f32 volumeTimes6 = glm::dot(normal, AD);

                // Four points are affinely dependent if they are coplanar
                // (the volume of the tetrahedron they form is very small).
                return std::abs(volumeTimes6) <= VoronoiSimplex::EPSILON;
            }
            default:
                return false;
        }
    }

    void VoronoiSimplex::BackupClosestPointInSimplex(glm::vec3 &point) {
        point = _closestPoint;
    }

    void VoronoiSimplex::ComputeClosestPointsOfAandB(glm::vec3 &pA, glm::vec3 &pB) const {
        pA = _closestSupportPointOnShapeOne;
        pB = _closestSupportPointOnShapeTwo;
    }

    bool VoronoiSimplex::recomputeClosestPoint() {
        VASSERT(_vertexCount <= 4, "Invalid simplex: vertex count cannot exceed 4.");

        // Only recompute if the simplex has been modified (flag is set)
        if (_recomputeClosestPoint) {
            _recomputeClosestPoint = false;

            // Dispatch to the appropriate computation based on number of vertices
            switch (_vertexCount) {
                case 0:
                    // Empty simplex - no valid closest point
                    _closestPointValid = false;
                    break;

                case 1: {
                    // Single vertex - it is the closest point by definition
                    _closestPoint = _vertices[0];
                    _closestSupportPointOnShapeOne = _supportPointsOnShapeOne[0];
                    _closestSupportPointOnShapeTwo = _supportPointsOnShapeTwo[0];
                    // Barycentric coordinate: 100% of vertex 0
                    setBarycentricCoords(1.0f, 0.0f, 0.0f, 0.0f);
                    _closestPointValid = checkClosestPointValid();

                    break;
                }
                case 2: {
                    // Line segment - find closest point on segment AB
                    i32 bitsUsedVertices = 0;
                    f32 t = 0.0f;

                    // Compute the parameter t and which vertices are used
                    computeClosestPointOnSegment(_vertices[0], _vertices[1], bitsUsedVertices, t);

                    // Interpolate support points using parameter t
                    _closestSupportPointOnShapeOne = _supportPointsOnShapeOne[0] + t * (_supportPointsOnShapeOne[1] - _supportPointsOnShapeOne[0]);
                    _closestSupportPointOnShapeTwo = _supportPointsOnShapeTwo[0] + t * (_supportPointsOnShapeTwo[1] - _supportPointsOnShapeTwo[0]);
                    _closestPoint = _closestSupportPointOnShapeOne - _closestSupportPointOnShapeTwo;

                    // Barycentric coordinates: (1-t) for vertex 0, t for vertex 1
                    setBarycentricCoords(1.0f - t, t, 0.0f, 0.0f);
                    _closestPointValid = checkClosestPointValid();

                    // Remove vertices not contributing to the closest point
                    ReduceSimplex(bitsUsedVertices);

                    break;
                }
                case 3: {
                    // Triangle - find closest point on triangle ABC
                    i32 bitsUsedVertices = 0;
                    glm::vec3 barycentricCoords;

                    // Compute barycentric coordinates and which vertices are used
                    computeClosestPointOnTriangle(_vertices[0], _vertices[1], _vertices[2], bitsUsedVertices, barycentricCoords);

                    // Interpolate support points using barycentric coordinates
                    _closestSupportPointOnShapeOne = _supportPointsOnShapeOne[0] * barycentricCoords.x + _supportPointsOnShapeOne[1] * barycentricCoords.y +
                                                     _supportPointsOnShapeOne[2] * barycentricCoords.z;
                    _closestSupportPointOnShapeTwo = _supportPointsOnShapeTwo[0] * barycentricCoords.x + _supportPointsOnShapeTwo[1] * barycentricCoords.y +
                                                     _supportPointsOnShapeTwo[2] * barycentricCoords.z;
                    _closestPoint = _closestSupportPointOnShapeOne - _closestSupportPointOnShapeTwo;

                    setBarycentricCoords(barycentricCoords.x, barycentricCoords.y, barycentricCoords.z, 0.0f);
                    _closestPointValid = checkClosestPointValid();

                    // Remove vertices not contributing to the closest point
                    ReduceSimplex(bitsUsedVertices);

                    break;
                }
                case 4: {
                    // Tetrahedron - find closest point on tetrahedron ABCD
                    i32 bitsUsedVertices = 0;
                    glm::vec2 barycentricCoordsAB, barycentricCoordsCD;
                    bool isDegenerate = false;

                    // Check if origin is outside the tetrahedron and find closest face
                    bool isOutside = computeClosestPointOnTetrahedron(
                        _vertices[0], _vertices[1], _vertices[2], _vertices[3], bitsUsedVertices, barycentricCoordsAB, barycentricCoordsCD, isDegenerate);

                    if (isOutside) {
                        // Origin is outside - use barycentric coordinates from closest face
                        _closestSupportPointOnShapeOne =
                            _supportPointsOnShapeOne[0] * barycentricCoordsAB.x + _supportPointsOnShapeOne[1] * barycentricCoordsAB.y +
                            _supportPointsOnShapeOne[2] * barycentricCoordsCD.x + _supportPointsOnShapeOne[3] * barycentricCoordsCD.y;
                        _closestSupportPointOnShapeTwo =
                            _supportPointsOnShapeTwo[0] * barycentricCoordsAB.x + _supportPointsOnShapeTwo[1] * barycentricCoordsAB.y +
                            _supportPointsOnShapeTwo[2] * barycentricCoordsCD.x + _supportPointsOnShapeTwo[3] * barycentricCoordsCD.y;
                        _closestPoint = _closestSupportPointOnShapeOne - _closestSupportPointOnShapeTwo;

                        setBarycentricCoords(barycentricCoordsAB.x, barycentricCoordsAB.y, barycentricCoordsCD.x, barycentricCoordsCD.y);
                        _closestPointValid = checkClosestPointValid();

                        // Remove vertices not contributing to the closest point
                        ReduceSimplex(bitsUsedVertices);

                    } else {
                        // Origin is inside the tetrahedron
                        if (isDegenerate) {
                            // Degenerate tetrahedron (all points coplanar)
                            _closestPointValid = false;
                        } else {
                            // Valid tetrahedron containing origin - closest point is origin itself
                            setBarycentricCoords(0.0f, 0.0f, 0.0f, 0.0f);

                            _closestSupportPointOnShapeOne = glm::vec3(0.0f);
                            _closestSupportPointOnShapeTwo = glm::vec3(0.0f);
                            _closestPoint = glm::vec3(0.0f);

                            _closestPointValid = true;
                        }
                    }

                    break;
                }
            }
        }

        return _closestPointValid;
    }

    void VoronoiSimplex::computeClosestPointOnSegment(const glm::vec3 &a, const glm::vec3 &b, i32 &bitUsedVertices, float &t) const {
        // Vector from A to origin (P = origin = (0,0,0))
        const glm::vec3 AP = -a;
        // Vector from A to B
        const glm::vec3 AB = b - a;
        // Project AP onto AB
        const f32 APDotAB = glm::dot(AP, AB);

        // Case 1: Origin is closest to vertex A
        // (projection falls before point A along the AB direction)
        if (APDotAB <= 0.0f) {
            t = 0.0f;
            bitUsedVertices = 1; // Only vertex A is used (bit 0 set)
        } else {
            // Squared length of segment AB
            const f32 ABLengthSquared = glm::length2(AB);

            // Case 2: Origin is closest to vertex B
            // (projection falls beyond point B along the AB direction)
            if (APDotAB >= ABLengthSquared) {
                t = 1.0f;
                bitUsedVertices = 2; // Only vertex B is used (bit 1 set)
            } else {
                // Case 3: Origin projects onto the interior of segment AB
                // Compute parametric position t in [0,1]
                t = APDotAB / ABLengthSquared;
                bitUsedVertices = 3; // Both vertices A and B are used (bits 0 and 1 set)
            }
        }
    }

    void VoronoiSimplex::computeClosestPointOnTriangle(
        const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, i32 &bitsUsedPoints, glm::vec3 &barycentricCoordsABC) const {

        // This implements Christer Ericson's Voronoi region-based triangle closest point algorithm
        // It tests the origin against each Voronoi region of the triangle (vertices, edges, and face)

        // Edges from vertex A
        const glm::vec3 AB = b - a;
        const glm::vec3 AC = c - a;
        const glm::vec3 AP = -a;         // Vector from A to origin
        const f32 d1 = glm::dot(AB, AP); // Project AP onto AB
        const f32 d2 = glm::dot(AC, AP); // Project AP onto AC

        // Check if origin is in the Voronoi region of vertex A
        // (both projections are negative, meaning origin is "behind" both edges from A)
        if (d1 <= 0.0f && d2 <= 0.0f) {
            bitsUsedPoints = 1; // Only vertex A is used (bit 0 set)
            barycentricCoordsABC = glm::vec3(1.0f, 0.0f, 0.0f);
            return;
        }

        // Check Voronoi region of vertex B
        const glm::vec3 BP = -b;         // Vector from B to origin
        const f32 d3 = glm::dot(AB, BP); // Project BP onto AB
        const f32 d4 = glm::dot(AC, BP); // Project BP onto AC (from B's perspective: BC direction)

        // Origin is in the Voronoi region of vertex B
        if (d3 >= 0.0f && d4 <= d3) {
            bitsUsedPoints = 2; // Only vertex B is used (bit 1 set)
            barycentricCoordsABC = glm::vec3(0.0f, 1.0f, 0.0f);
            return;
        }

        // Check if origin is in the Voronoi region of edge AB
        const f32 vc = d1 * d4 - d3 * d2;

        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {

            VASSERT(std::abs(d1 - d3) > VE_K_MACHINE_EPSILON, "Degenerate triangle: vertices A and B are too close to each other.");

            bitsUsedPoints = 3; // Vertices A and B are used (bits 0 and 1 set)
            // Compute parametric position along edge AB
            f32 v = d1 / (d1 - d3);
            barycentricCoordsABC = glm::vec3(1.0f - v, v, 0.0f);
            return;
        }

        // Check Voronoi region of vertex C
        const glm::vec3 CP = -c; // Vector from C to origin
        const f32 d5 = glm::dot(AB, CP);
        const f32 d6 = glm::dot(AC, CP); // Project CP onto AC

        // Origin is in the Voronoi region of vertex C
        if (d6 >= 0.0f && d5 <= d6) {
            bitsUsedPoints = 4; // Only vertex C is used (bit 2 set)
            barycentricCoordsABC = glm::vec3(0.0f, 0.0f, 1.0f);
            return;
        }

        // Check if origin is in the Voronoi region of edge AC
        const f32 vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {

            VASSERT(std::abs(d2 - d6) > VE_K_MACHINE_EPSILON, "Degenerate triangle: vertices A and C are too close to each other.");

            bitsUsedPoints = 5; // Vertices A and C are used (bits 0 and 2 set)
            // Compute parametric position along edge AC
            f32 w = d2 / (d2 - d6);
            barycentricCoordsABC = glm::vec3(1.0f - w, 0.0f, w);
            return;
        }

        // Check if origin is in the Voronoi region of edge BC
        const f32 va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            VASSERT(std::abs((d4 - d3) + (d5 - d6)) > VE_K_MACHINE_EPSILON, "Degenerate triangle: vertices B and C are too close to each other.");

            bitsUsedPoints = 6; // Vertices B and C are used (bits 1 and 2 set)
            // Compute parametric position along edge BC
            f32 w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            barycentricCoordsABC = glm::vec3(0.0f, 1.0f - w, w);
            return;
        }

        // Origin is in the Voronoi region of the triangle face
        // Compute barycentric coordinates using the sub-triangle areas (va, vb, vc)
        bitsUsedPoints = 7; // All vertices A, B, and C are used (bits 0, 1, and 2 set)
        const f32 denom = 1.0f / (va + vb + vc);
        const f32 v = vb * denom;
        const f32 w = vc * denom;
        barycentricCoordsABC = glm::vec3(1.0f - v - w, v, w);
    }

    bool VoronoiSimplex::computeClosestPointOnTetrahedron(const glm::vec3 &a,
                                                          const glm::vec3 &b,
                                                          const glm::vec3 &c,
                                                          const glm::vec3 &d,
                                                          i32 &bitsUsedPoints,
                                                          glm::vec2 &barycentricCoordsAB,
                                                          glm::vec2 &barycentricCoordsCD,
                                                          bool &isDegenerate) const {

        // Initialize assuming origin is inside the tetrahedron
        isDegenerate = false;

        // Start with all vertices marked as used (bits 0-3 all set = 15)
        bitsUsedPoints = 15; // 1111 (A, B, C and D are all potentially used)
        barycentricCoordsAB = glm::vec2(0.0f);
        barycentricCoordsCD = glm::vec2(0.0f);

        // Test if the origin is outside each of the four triangular faces
        // Each test returns:
        //   1 = origin is outside (on opposite side from the 4th vertex)
        //   0 = origin is inside (on same side as the 4th vertex)
        //  -1 = degenerate case (4th vertex is coplanar with the face)
        i32 isOriginOutsideFaceABC = testOriginOutsideOfPlane(a, b, c, d);
        i32 isOriginOutsideFaceACD = testOriginOutsideOfPlane(a, c, d, b);
        i32 isOriginOutsideFaceADB = testOriginOutsideOfPlane(a, d, b, c);
        i32 isOriginOutsideFaceBDC = testOriginOutsideOfPlane(b, d, c, a);

        // Check for degenerate tetrahedron (all vertices coplanar)
        if (isOriginOutsideFaceABC < 0 || isOriginOutsideFaceACD < 0 || isOriginOutsideFaceADB < 0 || isOriginOutsideFaceBDC < 0) {

            // The tetrahedron is degenerate (has zero volume)
            isDegenerate = true;
            return false;
        }

        // If origin is not outside any face, it must be inside the tetrahedron
        if (isOriginOutsideFaceABC == 0 && isOriginOutsideFaceACD == 0 && isOriginOutsideFaceADB == 0 && isOriginOutsideFaceBDC == 0) {

            // The origin is inside the tetrahedron - it is the closest point
            return false; // false = not outside
        }

        // Origin is outside at least one face - find which face is closest
        // Track the minimum squared distance to any face
        f32 closestSquareDistance = std::numeric_limits<f32>::max();
        i32 tempUsedVertices;
        glm::vec3 triangleBarycentricCoords;

        // Test face ABC (opposite to vertex D)
        if (isOriginOutsideFaceABC) {

            // Compute the closest point on this triangle face
            computeClosestPointOnTriangle(a, b, c, tempUsedVertices, triangleBarycentricCoords);
            glm::vec3 closestPoint = triangleBarycentricCoords[0] * a + triangleBarycentricCoords[1] * b + triangleBarycentricCoords[2] * c;
            f32 squareDist = glm::length2(closestPoint);

            // If this is the closest face so far, update our result
            if (squareDist < closestSquareDistance) {

                closestSquareDistance = squareDist;

                // Map triangle vertices (A,B,C) to tetrahedron vertices (0,1,2,3)
                // barycentricCoordsAB = (A, B), barycentricCoordsCD = (C, D)
                barycentricCoordsAB.x = triangleBarycentricCoords[0]; // Contribution from A
                barycentricCoordsAB.y = triangleBarycentricCoords[1]; // Contribution from B

                barycentricCoordsCD.x = triangleBarycentricCoords[2]; // Contribution from C
                barycentricCoordsCD.y = 0.0;                          // No contribution from D

                bitsUsedPoints = tempUsedVertices;
            }
        }

        // Test face ACD (opposite to vertex B)
        if (isOriginOutsideFaceACD) {

            // Compute the closest point on this triangle face
            computeClosestPointOnTriangle(a, c, d, tempUsedVertices, triangleBarycentricCoords);
            glm::vec3 closestPoint = triangleBarycentricCoords[0] * a + triangleBarycentricCoords[1] * c + triangleBarycentricCoords[2] * d;
            f32 squareDist = glm::length2(closestPoint);

            // If this is the closest face so far, update our result
            if (squareDist < closestSquareDistance) {

                closestSquareDistance = squareDist;

                // Map triangle vertices (A,C,D) to tetrahedron vertices
                barycentricCoordsAB.x = triangleBarycentricCoords[0]; // Contribution from A
                barycentricCoordsAB.y = 0.0f;                         // No contribution from B

                barycentricCoordsCD.x = triangleBarycentricCoords[1]; // Contribution from C
                barycentricCoordsCD.y = triangleBarycentricCoords[2]; // Contribution from D

                // Remap triangle used bits (0,1,2) to tetrahedron bits (0,2,3)
                bitsUsedPoints = mapTriangleUsedVerticesToTetrahedron(tempUsedVertices, 0, 2, 3);
            }
        }

        // Test face ADB (opposite to vertex C)
        if (isOriginOutsideFaceADB) {

            // Compute the closest point on this triangle face
            computeClosestPointOnTriangle(a, d, b, tempUsedVertices, triangleBarycentricCoords);
            glm::vec3 closestPoint = triangleBarycentricCoords[0] * a + triangleBarycentricCoords[1] * d + triangleBarycentricCoords[2] * b;
            f32 squareDist = glm::length2(closestPoint);

            // If this is the closest face so far, update our result
            if (squareDist < closestSquareDistance) {

                closestSquareDistance = squareDist;

                // Map triangle vertices (A,D,B) to tetrahedron vertices
                barycentricCoordsAB.x = triangleBarycentricCoords[0]; // Contribution from A
                barycentricCoordsAB.y = triangleBarycentricCoords[2]; // Contribution from B

                barycentricCoordsCD.x = 0.0f;                         // No contribution from C
                barycentricCoordsCD.y = triangleBarycentricCoords[1]; // Contribution from D

                // Remap triangle used bits (0,1,2) to tetrahedron bits (0,3,1)
                bitsUsedPoints = mapTriangleUsedVerticesToTetrahedron(tempUsedVertices, 0, 3, 1);
            }
        }

        // Test face BDC (opposite to vertex A)
        if (isOriginOutsideFaceBDC) {

            // Compute the closest point on this triangle face
            computeClosestPointOnTriangle(b, d, c, tempUsedVertices, triangleBarycentricCoords);
            glm::vec3 closestPoint = triangleBarycentricCoords[0] * b + triangleBarycentricCoords[1] * d + triangleBarycentricCoords[2] * c;
            f32 squareDist = glm::length2(closestPoint);

            // If this is the closest face so far, update our result
            if (squareDist < closestSquareDistance) {

                // No need to update closestSquareDistance as this is the last face

                // Map triangle vertices (B,D,C) to tetrahedron vertices
                barycentricCoordsAB.x = 0.0f;                         // No contribution from A
                barycentricCoordsAB.y = triangleBarycentricCoords[0]; // Contribution from B

                barycentricCoordsCD.x = triangleBarycentricCoords[2]; // Contribution from C
                barycentricCoordsCD.y = triangleBarycentricCoords[1]; // Contribution from D

                // Remap triangle used bits (0,1,2) to tetrahedron bits (1,3,2)
                bitsUsedPoints = mapTriangleUsedVerticesToTetrahedron(tempUsedVertices, 1, 3, 2);
            }
        }

        // Return true indicating the origin is outside the tetrahedron
        return true;
    }

    i32 VoronoiSimplex::testOriginOutsideOfPlane(const glm::vec3 &a, const glm::vec3 &b, const glm::vec3 &c, const glm::vec3 &d) const {
        // Compute the (unnormalized) plane normal for triangle (a, b, c) and the
        // signed distances (dot products) of the origin and point `d` to that plane.
        // Return values:
        //  -1  : degenerate case (point `d` lies approximately in the plane of a,b,c)
        //   1  : origin and `d` are on opposite sides of the plane (origin is "outside")
        //   0  : origin and `d` are on the same side of the plane
        // The test uses the sign of the dot products with the triangle normal; if
        // the signs differ the origin and `d` lie on opposite sides.
        glm::vec3 normal = glm::cross(b - a, c - a);
        f32 originSignedDistanceToPlane = glm::dot(-a, normal); // signed distance of origin.
        f32 dSignedDistanceToPlane = glm::dot(d - a, normal);   // signed distance of point d.

        // If point `d` lies (approximately) in the plane of (a,b,c) treat this as
        // a degenerate tetrahedron and return -1. The caller should avoid adding
        // affinely-dependent points (see `IsAffinelyDependent()`).
        if (dSignedDistanceToPlane * dSignedDistanceToPlane < VoronoiSimplex::EPSILON * VoronoiSimplex::EPSILON) {
            return -1;
        }

        return originSignedDistanceToPlane * dSignedDistanceToPlane < 0.0f;
    }

    i32 VoronoiSimplex::mapTriangleUsedVerticesToTetrahedron(i32 triangleUsedVertices, i32 first, i32 second, i32 third) const {
        VASSERT(triangleUsedVertices <= 7, "Invalid triangle used vertices: must be between 0 and 7 (inclusive).");

        // Map triangle vertex usage bits to tetrahedron vertex usage bits
        // triangleUsedVertices uses bits 0,1,2 for the three triangle vertices
        // This function remaps them to the specified tetrahedron vertex indices (first, second, third)
        //
        // Example: If triangle uses vertices (0,2) with bits = 5 (binary 101),
        //          and we map to tetrahedron indices (0,2,3),
        //          result will have bits set at positions 0 and 2
        i32 tetrahedronUsedVertices = (((1 & triangleUsedVertices) != 0) << first) |  // Check bit 0 and shift to 'first' position
                                      (((2 & triangleUsedVertices) != 0) << second) | // Check bit 1 and shift to 'second' position
                                      (((4 & triangleUsedVertices) != 0) << third);   // Check bit 2 and shift to 'third' position

        VASSERT(tetrahedronUsedVertices <= 14, "Invalid tetrahedron used vertices: must be between 0 and 14 (inclusive).");

        return tetrahedronUsedVertices;
    }

} // namespace Vulkyrie
