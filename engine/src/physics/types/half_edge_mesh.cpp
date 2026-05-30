#include "physics/types/half_edge_mesh.h"

namespace Vulkyrie {

    void HalfEdgeMesh::ComputeHalfEdges() {
        VASSERT(!_faces.empty(), "ComputeHalfEdges requires at least one face.");
        VASSERT(!_vertices.empty(), "ComputeHalfEdges requires at least one vertex.");

        // Temporary maps used during construction — discarded after ComputeHalfEdges returns.
        std::unordered_map<VertexPair, Edge> edgeMap;                 // (v1,v2) → candidate half-edge, used to find twins.
        std::unordered_map<VertexPair, VertexPair> nextEdgeMap;       // (v1,v2) → (v2,v3), the next directed edge around the same face.
        std::unordered_map<VertexPair, size_t> edgeToIndexMap;        // (v1,v2) → final index in _edges.
        std::unordered_map<size_t, VertexPair> edgeIndexToKeyMap;     // final index → (v1,v2), the inverse of edgeToIndexMap.
        std::unordered_map<size_t, VertexPair> faceIndexToEdgeKeyMap; // face index → key of one half-edge on that face.

        std::vector<VertexPair> currentFaceEdges;
        currentFaceEdges.reserve(_faces[0].FaceVertices.size());

        // Note: this half-edge mesh only supports closed meshes. Boundary edges (those
        // adjacent to only one face) have no twin and will not be added to _edges.

        for (size_t f = 0; f < _faces.size(); ++f) {
            Face &face = _faces[f];
            VertexPair firstEdgeKey(0, 0);

            for (size_t v = 0; v < face.FaceVertices.size(); ++v) {
                const size_t vertexOneIndex = face.FaceVertices[v];
                // Wrap around to vertex 0 for the last edge, closing the face loop.
                const size_t vertexTwoIndex = face.FaceVertices[v == face.FaceVertices.size() - 1 ? 0 : v + 1];
                const VertexPair vertexOneTwoPair(vertexOneIndex, vertexTwoIndex);

                Edge edge;
                edge.FaceIndex = f;
                edge.StartVertexIndex = vertexOneIndex;

                if (0 == v) {
                    // Record the first edge of this face so the last edge can point back to it.
                    firstEdgeKey = vertexOneTwoPair;
                } else {
                    // Map the previous edge to this edge to build the next-edge chain.
                    nextEdgeMap.emplace(currentFaceEdges[currentFaceEdges.size() - 1], vertexOneTwoPair);
                }

                if (v == face.FaceVertices.size() - 1) {
                    // Close the cycle: the last edge's next is the first edge of this face.
                    nextEdgeMap.emplace(vertexOneTwoPair, firstEdgeKey);
                }

                edgeMap.emplace(vertexOneTwoPair, edge);

                const VertexPair vertexTwoOnePair(vertexTwoIndex, vertexOneIndex);

                // Record one edge key per face (only the first emplace wins) for the face EdgeIndex pass below.
                faceIndexToEdgeKeyMap.emplace(f, vertexOneTwoPair);

                // Check if the reverse half-edge (twin) has already been registered by an adjacent face.
                auto itEdge = edgeMap.find(vertexTwoOnePair);

                if (itEdge != edgeMap.end()) {
                    // Twin found: commit both half-edges to _edges as a consecutive pair.
                    const size_t edgeIndex = _edges.size();

                    // Wire up twin indices: each half-edge points at the other.
                    itEdge->second.TwinEdgeIndex = edgeIndex + 1;
                    edge.TwinEdgeIndex = edgeIndex;

                    // Record the bidirectional mapping between _edges index and VertexPair key.
                    edgeIndexToKeyMap.emplace(edgeIndex, vertexTwoOnePair);
                    edgeIndexToKeyMap.emplace(edgeIndex + 1, vertexOneTwoPair);

                    // Set the entry-point half-edge for each endpoint vertex.
                    _vertices[vertexOneIndex].EdgeIndex = edgeIndex + 1;
                    _vertices[vertexTwoIndex].EdgeIndex = edgeIndex;

                    edgeToIndexMap.emplace(vertexOneTwoPair, edgeIndex + 1);
                    edgeToIndexMap.emplace(vertexTwoOnePair, edgeIndex);

                    _edges.push_back(itEdge->second);
                    _edges.push_back(edge);
                }

                currentFaceEdges.push_back(vertexOneTwoPair);
            }

            currentFaceEdges.clear();
        }

        // Resolve NextEdgeIndex for each half-edge: map the edge's key to its next key via nextEdgeMap,
        // then convert that key back to a final _edges index via edgeToIndexMap.
        for (size_t i = 0; i < _edges.size(); ++i) {
            _edges[i].NextEdgeIndex = edgeToIndexMap.at(nextEdgeMap.at(edgeIndexToKeyMap.at(i)));
        }

        // Resolve the entry-point half-edge index for each face.
        for (size_t i = 0; i < _faces.size(); ++i) {
            _faces[i].EdgeIndex = edgeToIndexMap.at(faceIndexToEdgeKeyMap.at(i));
        }
    }

} // namespace Vulkyrie
