#include "physics/types/height_field.h"

namespace Vulkyrie {

    HeightField::HeightField(HalfEdgeMesh &triangleHalfEdgeStructure)
        : _triangleHalfEdgeStructure(triangleHalfEdgeStructure) {
    }

    bool HeightField::Initialize(
        size_t totalColumns, size_t totalRows, const void *heightFieldData, HeightDataType dataType, std::vector<Message> &messages, f32 integerHeightScale) {

        if (totalColumns < 2 || totalRows < 2) {
            messages.push_back(Message("The number of grid columns and grid rows must be at least two", Message::MessageType::Error));

            return false;
        }

        _heightFieldData.reserve(totalColumns * totalRows);
        _heightFieldData.resize(totalColumns * totalRows);

        _totalColumns = totalColumns;
        _totalRows = totalRows;
        _width = static_cast<f32>(totalColumns - 1);
        _length = static_cast<f32>(totalRows - 1);
        _integerHeightScale = integerHeightScale;
        _heightDataType = dataType;

        //  Copy the height values from the user into the height-field
        copyData(heightFieldData);

        VASSERT(_minHeight <= _maxHeight, "Height field min height must be <= max height.");

        const f32 halfHeight = (_maxHeight - _minHeight) * f32(0.5);

        VASSERT(halfHeight >= 0, "Half height must be >= 0.");
        VASSERT(_width >= 1, "Height field width must be >= 1.");
        VASSERT(_length >= 1, "Height field length must be >= 1.");

        // Compute the local AABB of the height field
        _bounds.SetMinMax(glm::vec3(-_width * f32(0.5), -halfHeight, -_length * f32(0.5)), glm::vec3(_width * f32(0.5), halfHeight, _length * f32(0.5)));

        VASSERT(_heightFieldData.size() == _totalRows * _totalColumns, "Size of height field data must be equal to _totalRows * _totalColumns.");

        return true;
    }

    void HeightField::ComputeOverlappingTriangles(const AABB &aabb,
                                                  std::vector<glm::vec3> &triangleVertices,
                                                  std::vector<glm::vec3> &triangleVerticesNormals,
                                                  std::vector<u32> &shapeIds,
                                                  const glm::vec3 &scale) const {
        std::array<i32, 3> minGridCoords;
        std::array<i32, 3> maxGridCoords;

        computeMinMaxGridCoordinates(minGridCoords, maxGridCoords, aabb);

        const size_t maxColumn = _totalColumns - 1;
        const size_t maxRow = _totalRows - 1;

        const size_t iMin = std::min(static_cast<size_t>(std::max(minGridCoords[0], 0)), maxColumn);
        const size_t iMax = std::min(static_cast<size_t>(std::max(maxGridCoords[0], 0)), maxColumn);
        const size_t jMin = std::min(static_cast<size_t>(std::max(minGridCoords[2], 0)), maxRow);
        const size_t jMax = std::min(static_cast<size_t>(std::max(maxGridCoords[2], 0)), maxRow);

        VASSERT(iMin < _totalColumns, "iMin must be less than _totalColumns.");
        VASSERT(iMax < _totalColumns, "iMax must be less than _totalColumns.");
        VASSERT(jMin < _totalRows, "jMin must be less than _totalRows.");
        VASSERT(jMax < _totalRows, "jMax must be less than _totalRows.");

        for (size_t i = iMin; i < iMax; i++) {
            for (size_t j = jMin; j < jMax; j++) {
                const glm::vec3 &p1 = GetVertexAt(i, j) * scale;
                const glm::vec3 &p2 = GetVertexAt(i, j + 1) * scale;
                const glm::vec3 &p3 = GetVertexAt(i + 1, j) * scale;
                const glm::vec3 &p4 = GetVertexAt(i + 1, j + 1) * scale;

                triangleVertices.push_back(p1);
                triangleVertices.push_back(p2);
                triangleVertices.push_back(p3);

                const glm::vec3 triangle1Normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));

                triangleVerticesNormals.push_back(triangle1Normal);
                triangleVerticesNormals.push_back(triangle1Normal);
                triangleVerticesNormals.push_back(triangle1Normal);

                shapeIds.push_back(computeTriangleShapeId(i, j, u32(0)));

                triangleVertices.push_back(p3);
                triangleVertices.push_back(p2);
                triangleVertices.push_back(p4);

                const glm::vec3 triangle2Normal = glm::normalize(glm::cross(p2 - p3, p4 - p3));

                triangleVerticesNormals.push_back(triangle2Normal);
                triangleVerticesNormals.push_back(triangle2Normal);
                triangleVerticesNormals.push_back(triangle2Normal);

                shapeIds.push_back(computeTriangleShapeId(i, j, u32(1)));
            }
        }
    }

    void HeightField::copyData(const void *heightFieldData) {
        for (size_t x = 0; x < _totalColumns; x++) {
            for (size_t y = 0; y < _totalRows; y++) {

                f32 height = 0.0;

                switch (_heightDataType) {
                    case HeightDataType::F32:
                        height = f32(((f32 *)heightFieldData)[y * _totalColumns + x]);
                        break;
                    case HeightDataType::F64:
                        height = f32(((f64 *)heightFieldData)[y * _totalColumns + x]);
                        break;
                    case HeightDataType::I32:
                        height = f32(static_cast<f32>(((i32 *)heightFieldData)[y * _totalColumns + x]) * _integerHeightScale);
                        break;
                    default:
                        VASSERT(false, "Unsupported data type for creating height field.");
                }

                _heightFieldData[y * _totalColumns + x] = height;

                if (x == 0 && y == 0) {
                    _minHeight = height;
                    _maxHeight = height;
                }

                if (height < _minHeight) {
                    _minHeight = height;
                }

                if (height > _maxHeight) {
                    _maxHeight = height;
                }
            }
        }

        _heightOrigin = -(_maxHeight - _minHeight) * f32(0.5) - _minHeight;
    }

    void HeightField::computeMinMaxGridCoordinates(std::array<i32, 3> &minCoords, std::array<i32, 3> &maxCoords, const AABB &aabbToCollide) const {
        glm::vec3 minPoint = glm::max(aabbToCollide.GetMin(), _bounds.GetMin());
        minPoint = glm::min(minPoint, _bounds.GetMax());

        glm::vec3 maxPoint = glm::min(aabbToCollide.GetMax(), _bounds.GetMax());
        maxPoint = glm::max(maxPoint, _bounds.GetMin());

        const glm::vec3 translateVec = _bounds.GetExtents() * f32(0.5);
        minPoint += translateVec;
        maxPoint += translateVec;

        VASSERT(minPoint.x >= 0 && minPoint.y >= 0 && minPoint.z >= 0, "");
        VASSERT(maxPoint.x >= 0 && maxPoint.y >= 0 && maxPoint.z >= 0, "");

        minCoords[0] = static_cast<i32>(minPoint.x + f32(0.5)) - 1;
        minCoords[1] = static_cast<i32>(minPoint.y + f32(0.5)) - 1;
        minCoords[2] = static_cast<i32>(minPoint.z + f32(0.5)) - 1;

        maxCoords[0] = static_cast<i32>(maxPoint.x + f32(0.5)) + 1;
        maxCoords[1] = static_cast<i32>(maxPoint.y + f32(0.5)) + 1;
        maxCoords[2] = static_cast<i32>(maxPoint.z + f32(0.5)) + 1;
    }

} // namespace Vulkyrie
