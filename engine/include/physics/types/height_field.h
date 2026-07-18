#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/message.h"
#include "physics/collision/shapes/aabb.h"
#include "physics/types/half_edge_mesh.h"

namespace Vulkyrie {

    class HeightField final {
    public:
        HeightField(HalfEdgeMesh &triangleHalfEdgeStructure);

        VE_DELETE_MOVE_AND_COPY(HeightField);

        enum class HeightDataType : i32 { F32, F64, I32 };

        [[nodiscard]] VE_INLINE size_t GetTotalRows() const {
            return _totalRows;
        }

        [[nodiscard]] VE_INLINE size_t GetTotalColumns() const {
            return _totalColumns;
        }

        [[nodiscard]] VE_INLINE f32 GetMinHeight() const {
            return _minHeight;
        }

        [[nodiscard]] VE_INLINE f32 GetMaxHeight() const {
            return _maxHeight;
        }

        [[nodiscard]] VE_INLINE f32 GetIntegerHeightScale() const {
            return _integerHeightScale;
        }

        [[nodiscard]] VE_INLINE HeightDataType GetHeightDataType() const {
            return _heightDataType;
        }

        [[nodiscard]] VE_INLINE const AABB &GetBounds() const {
            return _bounds;
        }

        [[nodiscard]] VE_INLINE f32 GetHeightAt(size_t x, size_t y) const {
            VASSERT(x < _totalColumns, "Column index out of bounds.");
            VASSERT(y < _totalRows, "Row index out of bounds.");

            return _heightFieldData[y * _totalColumns + x];
        }

        [[nodiscard]] VE_INLINE glm::vec3 GetVertexAt(size_t x, size_t y) const {
            const f32 height = GetHeightAt(x, y);

            const glm::vec3 v = glm::vec3(-_width * f32(0.5) + static_cast<f32>(x), _heightOrigin + height, -_length * f32(0.5) + static_cast<f32>(y));

            VASSERT(_bounds.Contains(v), "Vertex must be inside the bounds of the AABB of the height field.");

            return v;
        }

        bool Initialize(size_t totalColumns,
                        size_t totalRows,
                        const void *heightFieldData,
                        HeightDataType dataType,
                        std::vector<Message> &messages,
                        f32 integerHeightScale = 1.0f);

        void ComputeOverlappingTriangles(const AABB &aabb,
                                         std::vector<glm::vec3> &triangleVertices,
                                         std::vector<glm::vec3> &triangleVerticesNormals,
                                         std::vector<u32> &shapeIds,
                                         const glm::vec3 &scale) const;

    private:
        std::vector<f32> _heightFieldData;
        AABB _bounds;
        HalfEdgeMesh &_triangleHalfEdgeStructure;
        size_t _totalColumns;
        size_t _totalRows;
        f32 _width;
        f32 _length;
        f32 _minHeight;
        f32 _maxHeight;
        f32 _heightOrigin;
        f32 _integerHeightScale;
        HeightDataType _heightDataType;

        [[nodiscard]] VE_INLINE u32 computeTriangleShapeId(size_t iIndex, size_t jIndex, u32 secondTriangleIncrement) const {
            return static_cast<u32>((jIndex * (_totalColumns - 1) + iIndex) * 2 + secondTriangleIncrement);
        }

        void copyData(const void *heightFieldData);
        void computeMinMaxGridCoordinates(std::array<i32, 3> &minCoords, std::array<i32, 3> &maxCoords, const AABB &aabbToCollide) const;

        // bool raycastTriangle(const Ray& ray, const Vector3& p1, const Vector3& p2, const Vector3& p3, uint32 shapeId,
        //                      Collider* collider, RaycastInfo& raycastInfo, decimal& smallestHitFraction,
        //                      TriangleRaycastSide testSide, MemoryAllocator& allocator) const;
        //
        // bool raycast(const Ray& ray, RaycastInfo& raycastInfo, Collider* collider, TriangleRaycastSide testSide,
        //              MemoryAllocator& allocator) const;
        //
        // bool computeEnteringRayGridCoordinates(const Ray& ray, int32& i, int32& j, Vector3& outHitPoint) const;
        //
        // void computeOverlappingTriangles(const AABB& aabb, Array<Vector3>& triangleVertices,
        //                                  Array<Vector3>& triangleVerticesNormals,
        //                                  Array<uint32>& shapeIds, const Vector3& scale) const;
    };

} // namespace Vulkyrie
