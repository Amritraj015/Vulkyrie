#pragma once

#include "core/asserts.h"
#include "core/pair.h"

namespace Vulkyrie {

    /** @brief Represents a closed polyhedron mesh using a half-edge data structure. Each undirected mesh edge is split into two directed half-edges pointing
     * in opposite directions, one belonging to each of the two adjacent faces. This representation enables efficient O(1) traversal of face edges, twin edges,
     * and vertex-adjacent edges purely through index lookups. This class only supports closed (watertight) meshes — boundary edges (adjacent to only one face)
     * are not handled and will be silently omitted from the structure. Typical usage: add all vertices and faces first, then call `ComputeHalfEdges()` once to
     * build the full connectivity. */
    class HalfEdgeMesh final {
    public:
        /** @brief Alias for a directed edge key represented as an ordered pair of vertex indices (start, end). Used as the key type in the internal lookup
         * maps during half-edge construction. */
        using VertexPair = Pair<size_t, size_t>;

        /** @brief Represents a single directed half-edge in the mesh. Each undirected mesh edge corresponds to exactly two half-edges pointing in opposite
         * directions, linked via `TwinEdgeIndex`. Half-edges are ordered around their face in a counter-clockwise winding. */
        struct Edge final {
            /** @brief Index of the vertex this half-edge originates from. */
            size_t StartVertexIndex;

            /** @brief Index of the twin half-edge (the half-edge on the adjacent face pointing in the opposite direction). */
            size_t TwinEdgeIndex;

            /** @brief Index of the face this half-edge belongs to. */
            size_t FaceIndex;

            /** @brief Index of the next half-edge around this face (counter-clockwise). */
            size_t NextEdgeIndex;
        };

        /** @brief Represents a polygonal face in the mesh. Stores its vertex indices and an entry-point half-edge index. Any half-edge belonging to this
         * face can be used as the entry point — once you have one, the full face boundary can be walked via `NextEdgeIndex`. */
        struct Face final {
            /** @brief Ordered list of vertex indices forming this face. */
            std::vector<size_t> FaceVertices;

            /** @brief Index of one half-edge belonging to this face. Use as the entry point for walking the face boundary. */
            size_t EdgeIndex;

            /** @brief Construct a face from an ordered list of vertex indices.
             * @param faceVertices The ordered vertex indices defining this face (counter-clockwise winding).
             */
            Face(const std::vector<size_t> &faceVertices)
                : FaceVertices(faceVertices)
                , EdgeIndex(0) {
            }
        };

        /** @brief Represents a vertex in the mesh, storing its index in the original vertex array and an entry-point half-edge. Once you have one outgoing
         * half-edge, all other edges incident to this vertex can be reached by following twin and next links. */
        struct Vertex final {
            /** @brief Index of this vertex in the original vertex position array supplied by the caller. */
            size_t VertexIndex;

            /** @brief Index of one half-edge originating from this vertex. Use as the entry point for walking vertex-adjacent edges. */
            size_t EdgeIndex;

            /** @brief Construct a vertex with the given position index.
             * @param vertexIndex The index of this vertex in the caller's vertex position array.
             */
            Vertex(size_t vertexIndex)
                : VertexIndex(vertexIndex)
                , EdgeIndex(0) {
            }
        };

        /** @brief Construct a half-edge mesh and pre-allocate internal storage.
         * @param faceCount Expected number of faces, used to reserve storage.
         * @param vertexCount Expected number of vertices, used to reserve storage.
         * @param edgeCount Expected number of half-edges, used to reserve storage.
         */
        HalfEdgeMesh(const size_t faceCount, const size_t vertexCount, const size_t edgeCount) {
            _faces.reserve(faceCount);
            _vertices.reserve(vertexCount);
            _edges.reserve(edgeCount);
        }

        /** @brief Default destructor. */
        ~HalfEdgeMesh() = default;

        /** @brief Build the half-edge connectivity from the previously added vertices and faces. Must be called once after all vertices and faces have been
         * added via `AddVertex()` and `AddFace()`. Populates `TwinEdgeIndex`, `NextEdgeIndex`, and `EdgeIndex` on all edges, faces, and vertices. Only
         * supports closed (watertight) meshes — boundary edges with no twin will not be added to the mesh. */
        void ComputeHalfEdges();

        /** @brief Add a vertex to the mesh.
         * @param vertexIndex The index of this vertex in the caller's vertex position array.
         * @returns The index of the newly added vertex within this half-edge mesh.
         */
        [[nodiscard]] VE_INLINE size_t AddVertex(size_t vertexIndex) {
            _vertices.emplace_back(vertexIndex);

            return _vertices.size() - 1;
        }

        /** @brief Add a polygonal face to the mesh.
         * @param faceVertices Ordered vertex indices defining the face boundary (counter-clockwise winding).
         */
        VE_INLINE void AddFace(const std::vector<size_t> &faceVertices) {
            _faces.emplace_back(faceVertices);
        }

        /** @brief Get the number of faces in the mesh.
         * @returns The number of faces.
         */
        [[nodiscard]] VE_INLINE size_t GetFaceCount() const {
            return _faces.size();
        }

        /** @brief Get the number of half-edges in the mesh. For a closed mesh this is twice the number of undirected edges.
         * @returns The number of half-edges.
         */
        [[nodiscard]] VE_INLINE size_t GetHalfEdgeCount() const {
            return _edges.size();
        }

        /** @brief Get the number of vertices in the mesh.
         * @returns The number of vertices.
         */
        [[nodiscard]] VE_INLINE size_t GetVertexCount() const {
            return _vertices.size();
        }

        /** @brief Get a face by index.
         * @param faceIndex The index of the face to retrieve.
         * @returns A const reference to the face at the given index.
         */
        [[nodiscard]] VE_INLINE const Face &GetFace(size_t faceIndex) const {
            VASSERT(faceIndex < _faces.size(), "Face index should be within the bounds of the number of faces in the half-edge mesh.");

            return _faces[faceIndex];
        }

        /** @brief Get a half-edge by index.
         * @param edgeIndex The index of the half-edge to retrieve.
         * @returns A const reference to the half-edge at the given index.
         */
        [[nodiscard]] VE_INLINE const Edge &GetHalfEdge(size_t edgeIndex) const {
            VASSERT(edgeIndex < _edges.size(), "Edge index should be within the bounds of the number of edges in the half-edge mesh.");

            return _edges[edgeIndex];
        }

        [[nodiscard]] VE_INLINE const std::vector<HalfEdgeMesh::Edge> &GetHalfEdges() const {
            return _edges;
        }

        /** @brief Get a vertex by index.
         * @param vertexIndex The index of the vertex to retrieve.
         * @returns A const reference to the vertex at the given index.
         */
        [[nodiscard]] VE_INLINE const Vertex &GetVertex(size_t vertexIndex) const {
            VASSERT(vertexIndex < _vertices.size(), "Vertex index should be within the bounds of the number of vertices in the half-edge mesh.");

            return _vertices[vertexIndex];
        }

    private:
        /** @brief All half-edges in the mesh. Populated by `ComputeHalfEdges()`. */
        std::vector<HalfEdgeMesh::Edge> _edges;

        /** @brief All faces in the mesh. Populated incrementally via `AddFace()`. */
        std::vector<HalfEdgeMesh::Face> _faces;

        /** @brief All vertices in the mesh. Populated incrementally via `AddVertex()`. */
        std::vector<HalfEdgeMesh::Vertex> _vertices;
    };

} // namespace Vulkyrie
