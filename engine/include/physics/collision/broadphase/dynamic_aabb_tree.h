#pragma once

#include "core/asserts.h"
#include "physics/physics_constants.h"
#include "physics/collision/shapes/aabb.h"

namespace Vulkyrie {

    struct AABBTreeNode final {
        public:
            Vulkyrie::AABB AABB;

            union {
                    i32 Children[2];
                    i32 Data;
                    void *DataPointer;
            };

            union {
                    i32 ParentNodeIndex;
                    i32 NextNodeIndex;
            };

            i32 Height;

            AABBTreeNode()
                : AABB(glm::vec3(0.0f), glm::vec3(0.0f))
                , Children{ AABB_TREE_NULL_NODE, AABB_TREE_NULL_NODE }
                , NextNodeIndex(AABB_TREE_NULL_NODE)
                , Height(-1) {};

            [[nodiscard]] VE_FORCE_INLINE bool IsLeaf() const {
                return Height == 0;
            }
    };

    class DynamicAABBTree final {
        public:
            DynamicAABBTree(f32 inflationPercentage = AABB_TREE_DEFAULT_INFLATION_PERCENTAGE,
                            size_t initialNodeCapacity = AABB_TREE_DEFAULT_INITIAL_NODE_CAPACITY);

            ~DynamicAABBTree() = default;

            VE_FORCE_INLINE i32 AddObject(const AABB &aabb, i32 data) {
                const i32 nodeIndex = allocateNode();

                const glm::vec3 inflation(aabb.GetExtents() * _inflationPercentage);
                _nodes[nodeIndex].AABB.SetMinMax(aabb.GetMin() - inflation, aabb.GetMax() + inflation);
                _nodes[nodeIndex].Height = 0;
                _nodes[nodeIndex].Data = data;

                insertLeafNode(nodeIndex);

                VASSERT(_nodes[nodeIndex].IsLeaf(), "Added node should be a leaf.");

                return nodeIndex;
            }

            VE_FORCE_INLINE i32 AddObject(const AABB &aabb, void *dataPointer) {
                const i32 nodeIndex = allocateNode();

                const glm::vec3 inflation(aabb.GetExtents() * _inflationPercentage);
                _nodes[nodeIndex].AABB.SetMinMax(aabb.GetMin() - inflation, aabb.GetMax() + inflation);
                _nodes[nodeIndex].Height = 0;
                _nodes[nodeIndex].DataPointer = dataPointer;

                insertLeafNode(nodeIndex);

                VASSERT(_nodes[nodeIndex].IsLeaf(), "Added node should be a leaf.");

                return nodeIndex;
            }

            bool UpdateObject(i32 nodeIndex, const AABB &newAABB, bool forceReinsert);

            VE_FORCE_INLINE void RemoveObject(i32 nodeIndex) {
                VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
                VASSERT(_nodes[nodeIndex].IsLeaf(), "Can only remove leaf nodes.");

                removeLeafNode(nodeIndex);
                releaseNode(nodeIndex);
            }

            [[nodiscard]] VE_FORCE_INLINE const AABB &GetFatAABB(i32 nodeIndex) const {
                VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");

                return _nodes[nodeIndex].AABB;
            }

            /** @brief Returns the AABB of the root node, which encompasses all objects in the tree. This is useful for quickly determining the overall bounds
             * of the scene or for broad-phase culling before performing more detailed collision checks. If the tree is empty, this will assert since there is
             * no valid root node. */
            [[nodiscard]] VE_FORCE_INLINE const AABB &GetRootNodeAABB() const {
                VASSERT(_rootNodeIndex != AABB_TREE_NULL_NODE, "Tree is empty.");

                return _nodes[_rootNodeIndex].AABB;
            }

            /** @brief Returns the data associated with the leaf node.
             * @param nodeIndex The index of the leaf node to retrieve the data from. Must be a valid index into the _nodes vector and must correspond to a leaf
             * node.
             * @return The data associated with the leaf node. */
            [[nodiscard]] VE_FORCE_INLINE i32 GetNodeData(i32 nodeIndex) const {
                VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
                VASSERT(_nodes[nodeIndex].IsLeaf(), "Can only get data from leaf nodes.");

                return _nodes[nodeIndex].Data;
            }

            /** @brief Returns a pointer to the data associated with the leaf node. The caller is responsible for ensuring that the pointer is used safely and
             * that the data it points to remains valid for the duration of its use.
             * @param nodeIndex The index of the leaf node to retrieve the data pointer from. Must be a valid index into the _nodes vector and must correspond
             * to a leaf node.
             * @return A pointer to the data associated with the leaf node. */
            [[nodiscard]] VE_FORCE_INLINE void *GetNodeDataPointer(i32 nodeIndex) const {
                VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
                VASSERT(_nodes[nodeIndex].IsLeaf(), "Can only get data from leaf nodes.");

                return _nodes[nodeIndex].DataPointer;
            }

            /** @brief Queries the tree for all leaf nodes whose AABBs overlap with the specified query AABB. The results are returned as a vector of node
             * indices, which can be used to retrieve the associated data for each overlapping node. This is useful for broad-phase collision detection, where
             * you want to quickly find potential collisions before performing more detailed checks.
             * @param queryAABB The AABB to query against. All leaf nodes whose AABBs overlap with this AABB will be included in the results.
             * @param outResults A vector that will be populated with the indices of the overlapping leaf nodes. The caller is responsible for clearing this
             * vector before calling the function if they want to avoid appending to existing results. */
            void QueryOverlaps(const AABB &queryAABB, std::vector<i32> &outResults);

            /** @brief Queries the tree for all pairs of leaf nodes that overlap with each other among the specified node indices. This is useful for finding
             * potential collisions between objects in the tree without needing to specify a separate query AABB, as it will check for overlaps between all
             * pairs of nodes in the input vector. The results are returned as a vector of pairs of node indices, where each pair represents two overlapping
             * nodes. To avoid duplicate pairs (e.g., both (A, B) and (B, A)), the function can enforce a consistent ordering of the indices in the pairs.
             * @param nodeIndices A vector of node indices to check for overlaps. Each index must be a valid index into the _nodes vector and should correspond
             * to a leaf node.
             * @param outOverlappingPairs A vector that will be populated with pairs of indices representing overlapping nodes. The caller is responsible for
             * clearing this vector before calling the function if they want to avoid appending to existing results. */
            void QueryOverlappingPairs(const std::vector<i32> &nodeIndices, std::vector<std::pair<i32, i32>> &outOverlappingPairs);

        private:
            /** @brief The vector of nodes in the dynamic AABB tree. Each node represents either a leaf (which corresponds to an actual object in the world) or
             * an internal node (which is used for spatial partitioning and does not correspond to a real object). The tree is stored as a contiguous array of
             * nodes, where the parent-child relationships are maintained through indices. The root node is at index _rootNodeIndex, and each node's
             * LeftChildIndex and RightChildIndex point to its children in the vector. Leaf nodes have their LeftChildIndex and RightChildIndex set to
             * AABB_TREE_NULL_NODE. */
            std::vector<AABBTreeNode> _nodes;

            // WARN: This is not thread-safe. If you need to perform queries from multiple threads,
            // you should use a separate instance of this vector for each thread.
            std::vector<i32> _queryNodesToVisit;

            /** @brief The index of the root node in the _nodes vector. This is used to quickly access the top-level AABB that encompasses all objects in the
             * tree. If the tree is empty, this will be set to AABB_TREE_NULL_NODE. */
            i32 _rootNodeIndex;

            /** @brief The index of the first free node in the _nodes vector. This is used to efficiently manage the allocation and deallocation of nodes in the
             * tree. When a new node is needed, it can be taken from the free list starting at this index, and when a node is removed, it can be added back to
             * the free list. This helps to minimize memory fragmentation and improve performance by reusing existing nodes instead of constantly resizing the
             * vector. */
            i32 _firstFreeNodeIndex;

            /** @brief The percentage by which to inflate the AABBs of leaf nodes when they are inserted or updated. This creates "fat" AABBs that can
             * accommodate some movement without needing to be updated immediately, which can improve performance by reducing the frequency of tree updates at
             * the cost of potentially more false positives during collision queries. For example, an inflation percentage of 0.1 would inflate the AABB by 10%
             * of its extents in each direction. */
            f32 _inflationPercentage;

            void removeLeafNode(i32 leafNodeIndex);
            void insertLeafNode(i32 leafNodeIndex);
            void releaseNode(i32 nodeIndex);
            i32 balanceSubtree(i32 nodeIndex);
            i32 allocateNode();
    };

} // namespace Vulkyrie
