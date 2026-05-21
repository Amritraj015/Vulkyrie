#include "physics/collision/broadphase/dynamic_aabb_tree.h"

namespace Vulkyrie {

    AABBTreeNode::AABBTreeNode()
        : AABB(glm::vec3(0.0f), glm::vec3(0.0f))
        , Children{ AABB_TREE_NULL_NODE, AABB_TREE_NULL_NODE }
        , NextNodeIndex(AABB_TREE_NULL_NODE)
        , Height(-1) {};

    DynamicAABBTree::DynamicAABBTree(f32 inflationPercentage, size_t initialNodeCapacity)
        : _rootNodeIndex(AABB_TREE_NULL_NODE)
        , _inflationPercentage(inflationPercentage) {

        // Reserve space in the query vector to avoid unnecessary allocations.
        _queryNodesToVisit.reserve(initialNodeCapacity);

        // Pre-allocate a pool of nodes for the tree to use.
        _nodes.resize(initialNodeCapacity);

        // Link all the nodes together in a free list. Each node's NextNodeIndex points to the next
        // free node in the list, and the last node's NextNodeIndex is set to AABB_TREE_NULL_NODE to indicate the end of the list.
        for (size_t i = 0; i < initialNodeCapacity - 1; ++i) {
            _nodes[i].NextNodeIndex = i + 1;
        }

        _nodes[initialNodeCapacity - 1].NextNodeIndex = AABB_TREE_NULL_NODE;

        // Set the index of the first free node to 0, because the tree is empty and all nodes are currently available for allocation.
        _firstFreeNodeIndex = 0;
    }

    bool DynamicAABBTree::UpdateObject(i32 nodeIndex, const AABB &newAABB, bool forceReinsert) {
        VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
        VASSERT(_nodes[nodeIndex].IsLeaf() && _nodes[nodeIndex].Height >= 0, "Can only update leaf nodes.");

        // If the new AABB is still contained within the existing fat AABB and we're not forcing a reinsert, we can skip updating the tree.
        if (!forceReinsert && _nodes[nodeIndex].AABB.Contains(newAABB)) {
            return false;
        }

        // Otherwise, we need to remove the node and reinsert it with the new AABB.
        // This may cause the tree to rebalance if necessary.
        removeLeafNode(nodeIndex);

        // Update the node's AABB to the new AABB, and inflate it by the specified percentage to
        // create a "fat" AABB that can accommodate some movement without needing to be updated immediately.
        AABBTreeNode &node = _nodes[nodeIndex];
        node.AABB = newAABB;
        const glm::vec3 inflation(newAABB.GetExtents() * _inflationPercentage);
        node.AABB.Inflate(inflation);

        VASSERT(node.AABB.Contains(newAABB), "Inflated AABB should still contain the original AABB.");

        // Reinsert the node into the tree with its updated AABB.
        insertLeafNode(nodeIndex);

        // Return true to indicate that the tree was updated and may need to be re-queried for collisions.
        return true;
    }

    void DynamicAABBTree::QueryOverlaps(const AABB &queryAABB, std::vector<i32> &outResults) {

        // If the tree is empty, there are no overlaps to find, so we can return early.
        if (_rootNodeIndex == AABB_TREE_NULL_NODE) {
            return;
        }

        // Start the query by adding the root node to the list of nodes to visit.
        // We will perform a depth-first traversal of the tree, checking for AABB overlaps at each node.
        _queryNodesToVisit.emplace_back(_rootNodeIndex);

        // Continue traversing the tree until there are no more nodes to visit.
        while (!_queryNodesToVisit.empty()) {
            // Pop the next node index from the list of nodes to visit.
            // This will be the current node we are checking for overlap with the query AABB.
            const i32 currentNodeIndex = _queryNodesToVisit.back();
            _queryNodesToVisit.pop_back();

            VASSERT(currentNodeIndex >= 0 && currentNodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");

            // If the current node index is AABB_TREE_NULL_NODE, it means we've reached a leaf node that
            // has been removed from the tree, so we can skip it and continue with the next node in the list.
            if (currentNodeIndex == AABB_TREE_NULL_NODE) {
                continue;
            }

            const AABBTreeNode &currentNode = _nodes[currentNodeIndex];

            // Check if the query AABB overlaps with the current node's AABB.
            // If it does, we need to check if it's a leaf node or an internal node.
            if (queryAABB.CollidesWith(currentNode.AABB)) {

                // If the current node is a leaf, it means it corresponds to an actual object in
                // the world that overlaps with the query AABB, so we add its index to the results.
                if (currentNode.IsLeaf()) {
                    outResults.emplace_back(currentNodeIndex);
                } else {
                    // If the current node is an internal node, it means it is used for spatial partitioning and does not correspond to a real
                    // object, so we need to add its children to the list of nodes to visit so that we can check them for overlap as well.
                    _queryNodesToVisit.emplace_back(currentNode.Children[0]);
                    _queryNodesToVisit.emplace_back(currentNode.Children[1]);
                }
            }
        }
    }

    void DynamicAABBTree::QueryOverlappingPairs(const std::vector<i32> &nodeIndices, std::vector<Pair<i32, i32>> &outOverlappingPairs) {

        // If the tree is empty, there are no overlapping pairs to find, so we can return early.
        if (_rootNodeIndex == AABB_TREE_NULL_NODE) {
            return;
        }

        // For each node index in the input vector, we will perform a query against the tree to find all overlapping nodes.
        for (const i32 testNodeIndex : nodeIndices) {
            VASSERT(testNodeIndex >= 0 && testNodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");

            // Start the query for this node by adding the root node to the list of nodes to visit.
            // We will perform a depth-first traversal of the tree, checking for AABB overlaps between the test node and the nodes in the tree.
            _queryNodesToVisit.emplace_back(_rootNodeIndex);

            // Get the AABB of the test node that we will be checking for overlaps against the nodes in the tree.
            const AABB &testNodeAABB = _nodes[testNodeIndex].AABB;

            // Continue traversing the tree until there are no more nodes to visit.
            while (!_queryNodesToVisit.empty()) {

                // Pop the next node index from the list of nodes to visit. This will be the current node we are checking for overlap with the test node's AABB.
                const i32 currentNodeIndex = _queryNodesToVisit.back();
                _queryNodesToVisit.pop_back();

                // Skip comparing the test node with itself, as we are only interested in finding pairs of distinct nodes that overlap.
                // And, If the current node index is AABB_TREE_NULL_NODE, it means we've reached a leaf node that
                // has been removed from the tree, so we can skip it and continue with the next node in the list.
                if (testNodeIndex == currentNodeIndex || currentNodeIndex == AABB_TREE_NULL_NODE) {
                    continue;
                }

                const AABBTreeNode &currentNode = _nodes[currentNodeIndex];

                // Check if the test node's AABB overlaps with the current node's AABB.
                // If it does, we need to check if it's a leaf node or an internal node.
                if (testNodeAABB.CollidesWith(currentNode.AABB)) {

                    // If the current node is a leaf, it means it corresponds to an actual object in the world that overlaps
                    // with the test node's AABB, so we add the pair of indices (test node index, current node index) to the results.
                    if (currentNode.IsLeaf()) {

                        // To avoid adding duplicate pairs in the results (e.g., both (A, B) and (B, A)), we can enforce a consistent
                        // ordering of the indices in the pair. For example, we can always store the pair with the smaller index first.
                        if (testNodeIndex < currentNodeIndex) {
                            outOverlappingPairs.emplace_back(testNodeIndex, currentNodeIndex);
                        } else {
                            outOverlappingPairs.emplace_back(currentNodeIndex, testNodeIndex);
                        }
                    } else {

                        // If the current node is an internal node, it means it is used for spatial partitioning and does not correspond to a
                        // real object, so we need to add its children to the list of nodes to visit so that we can check them for overlap as well.
                        _queryNodesToVisit.emplace_back(currentNode.Children[0]);
                        _queryNodesToVisit.emplace_back(currentNode.Children[1]);
                    }
                }
            }
        }
    }

    void DynamicAABBTree::removeLeafNode(i32 leafNodeIndex) {
        VASSERT(leafNodeIndex >= 0 && leafNodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
        VASSERT(_nodes[leafNodeIndex].IsLeaf(), "Can only remove leaf nodes.");

        // If the leaf node to remove is the root, we can simply clear the tree.
        if (_rootNodeIndex == leafNodeIndex) {
            _rootNodeIndex = AABB_TREE_NULL_NODE;
            return;
        }

        i32 siblingNodeIndex;
        i32 parentNodeIndex = _nodes[leafNodeIndex].ParentNodeIndex;
        i32 grandParentNodeIndex = _nodes[parentNodeIndex].ParentNodeIndex;

        // Determine the sibling node of the leaf being removed.
        // The sibling is the other child of the parent node, which will replace the parent in the tree after we remove the leaf.
        if (_nodes[parentNodeIndex].Children[0] == leafNodeIndex) {
            siblingNodeIndex = _nodes[parentNodeIndex].Children[1];
        } else {
            siblingNodeIndex = _nodes[parentNodeIndex].Children[0];
        }

        // If the parent of the node to remove is not the root,
        // we need to update the grandparent to point to the sibling of the removed node,
        // and then we can release the parent node back to the free list.
        if (grandParentNodeIndex != AABB_TREE_NULL_NODE) {
            if (_nodes[grandParentNodeIndex].Children[0] == parentNodeIndex) {
                _nodes[grandParentNodeIndex].Children[0] = siblingNodeIndex;
            } else {
                VASSERT(_nodes[grandParentNodeIndex].Children[1] == parentNodeIndex, "Parent node should be a child of its grandparent.");
                _nodes[grandParentNodeIndex].Children[1] = siblingNodeIndex;
            }

            _nodes[siblingNodeIndex].ParentNodeIndex = grandParentNodeIndex;
            releaseNode(parentNodeIndex);

            // After removing a leaf node and replacing its parent with its sibling, we need to walk back up the tree from the grandparent node and update the
            // AABBs and heights of all ancestor nodes, and perform any necessary balancing to maintain the properties of the tree.
            i32 currentNodeIndex = grandParentNodeIndex;

            while (currentNodeIndex != AABB_TREE_NULL_NODE) {
                // First, we balance the subtree rooted at the current node.
                // This may change the structure of the tree and update the current node index to point to the new root of the subtree after balancing.
                currentNodeIndex = balanceSubtree(currentNodeIndex);

                VASSERT(!_nodes[currentNodeIndex].IsLeaf(), "Internal nodes should never be leaves.");

                // After balancing, we need to update the AABB and height of the current node based on its children.
                i32 leftChildIndex = _nodes[currentNodeIndex].Children[0];
                i32 rightChildIndex = _nodes[currentNodeIndex].Children[1];

                _nodes[currentNodeIndex].AABB.MergeTwoAABBs(_nodes[leftChildIndex].AABB, _nodes[rightChildIndex].AABB);
                _nodes[currentNodeIndex].Height = 1 + std::max(_nodes[leftChildIndex].Height, _nodes[rightChildIndex].Height);

                VASSERT(_nodes[currentNodeIndex].Height > 0, "Node height should be greater than 0 after balancing.");

                currentNodeIndex = _nodes[currentNodeIndex].ParentNodeIndex;
            }
        } else {
            // If the parent node of the leaf being removed is the root node,
            // we need to update the root node index to point to the sibling of the removed leaf,
            // which will become the new root of the tree.
            _rootNodeIndex = siblingNodeIndex;
            _nodes[siblingNodeIndex].ParentNodeIndex = AABB_TREE_NULL_NODE;
            releaseNode(parentNodeIndex);
        }
    }

    void DynamicAABBTree::insertLeafNode(i32 leafNodeIndex) {
        // If the tree is currently empty, we can simply set the root node to be the new leaf node.
        if (_rootNodeIndex == AABB_TREE_NULL_NODE) {
            _rootNodeIndex = leafNodeIndex;
            _nodes[_rootNodeIndex].ParentNodeIndex = AABB_TREE_NULL_NODE;
            return;
        }

        const AABB newNodeAABB = _nodes[leafNodeIndex].AABB;
        i32 currentNodeIndex = _rootNodeIndex;

        while (!_nodes[currentNodeIndex].IsLeaf()) {
            i32 leftChildIndex = _nodes[currentNodeIndex].Children[0];
            i32 rightChildIndex = _nodes[currentNodeIndex].Children[1];

            const f32 currentNodeAABBVolume = _nodes[currentNodeIndex].AABB.GetVolume();
            const f32 costHere = AABB::With(_nodes[currentNodeIndex].AABB, newNodeAABB).GetVolume();
            const f32 inheritedCost = costHere - currentNodeAABBVolume;

            f32 costLeft;
            const f32 currentAndLeftCombinedAABBVolume = AABB::With(_nodes[leftChildIndex].AABB, newNodeAABB).GetVolume();

            if (_nodes[leftChildIndex].IsLeaf()) {
                costLeft = currentAndLeftCombinedAABBVolume + inheritedCost;
            } else {
                const f32 leftChildAABBVolume = _nodes[leftChildIndex].AABB.GetVolume();
                costLeft = inheritedCost + currentAndLeftCombinedAABBVolume - leftChildAABBVolume;
            }

            f32 costRight;
            const f32 currentAndRightCombinedAABBVolume = AABB::With(_nodes[rightChildIndex].AABB, newNodeAABB).GetVolume();

            if (_nodes[rightChildIndex].IsLeaf()) {
                costRight = currentAndRightCombinedAABBVolume + inheritedCost;
            } else {
                const f32 rightChildAABBVolume = _nodes[rightChildIndex].AABB.GetVolume();
                costRight = inheritedCost + currentAndRightCombinedAABBVolume - rightChildAABBVolume;
            }

            if (costHere < costLeft && costHere < costRight) {
                break;
            }

            currentNodeIndex = costLeft < costRight ? leftChildIndex : rightChildIndex;
        }

        const i32 oldParentNodeIndex = _nodes[currentNodeIndex].ParentNodeIndex;
        const i32 newParentNodeIndex = allocateNode();
        _nodes[newParentNodeIndex].ParentNodeIndex = oldParentNodeIndex;
        _nodes[newParentNodeIndex].AABB.MergeTwoAABBs(_nodes[currentNodeIndex].AABB, newNodeAABB);
        _nodes[newParentNodeIndex].Height = _nodes[currentNodeIndex].Height + 1;

        VASSERT(_nodes[newParentNodeIndex].Height > 0, "New parent node height should be greater than 0.");

        if (oldParentNodeIndex != AABB_TREE_NULL_NODE) {
            VASSERT(!_nodes[oldParentNodeIndex].IsLeaf(), "Old parent node should not be a leaf.");

            if (_nodes[oldParentNodeIndex].Children[0] == currentNodeIndex) {
                _nodes[oldParentNodeIndex].Children[0] = newParentNodeIndex;
            } else {
                _nodes[oldParentNodeIndex].Children[1] = newParentNodeIndex;
            }

            _nodes[newParentNodeIndex].Children[0] = currentNodeIndex;
            _nodes[newParentNodeIndex].Children[1] = leafNodeIndex;
            _nodes[currentNodeIndex].ParentNodeIndex = newParentNodeIndex;
            _nodes[leafNodeIndex].ParentNodeIndex = newParentNodeIndex;
        } else {
            _nodes[newParentNodeIndex].Children[0] = currentNodeIndex;
            _nodes[newParentNodeIndex].Children[1] = leafNodeIndex;
            _nodes[currentNodeIndex].ParentNodeIndex = newParentNodeIndex;
            _nodes[leafNodeIndex].ParentNodeIndex = newParentNodeIndex;
            _rootNodeIndex = newParentNodeIndex;
        }

        currentNodeIndex = _nodes[leafNodeIndex].ParentNodeIndex;

        VASSERT(!_nodes[currentNodeIndex].IsLeaf(), "Parent node of a leaf should not be a leaf.");

        while (currentNodeIndex != AABB_TREE_NULL_NODE) {
            currentNodeIndex = balanceSubtree(currentNodeIndex);

            VASSERT(_nodes[leafNodeIndex].IsLeaf(), "Inserted node should still be a leaf after balancing.");
            VASSERT(!_nodes[currentNodeIndex].IsLeaf(), "Internal nodes should never be leaves.");

            const i32 leftChildIndex = _nodes[currentNodeIndex].Children[0];
            const i32 rightChildIndex = _nodes[currentNodeIndex].Children[1];

            VASSERT(leftChildIndex != AABB_TREE_NULL_NODE && rightChildIndex != AABB_TREE_NULL_NODE, "Internal nodes should have two children.");

            _nodes[currentNodeIndex].Height = 1 + std::max(_nodes[leftChildIndex].Height, _nodes[rightChildIndex].Height);

            VASSERT(_nodes[currentNodeIndex].Height > 0, "Node height should be greater than 0 after balancing.");

            _nodes[currentNodeIndex].AABB.MergeTwoAABBs(_nodes[leftChildIndex].AABB, _nodes[rightChildIndex].AABB);

            currentNodeIndex = _nodes[currentNodeIndex].ParentNodeIndex;
        }

        VASSERT(_nodes[leafNodeIndex].IsLeaf(), "Inserted node should be a leaf.");
    }

    i32 DynamicAABBTree::balanceSubtree(i32 nodeIndex) {
        VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");

        AABBTreeNode &nodeA = _nodes[nodeIndex];

        // If the node is a leaf or has a height less than 2, it cannot be unbalanced, so we can return it as is.
        if (nodeA.IsLeaf() || nodeA.Height < 2) {
            return nodeIndex;
        }

        const i32 nodeBIndex = nodeA.Children[0];
        const i32 nodeCIndex = nodeA.Children[1];

        VASSERT(nodeBIndex >= 0 && nodeBIndex < static_cast<i32>(_nodes.size()), "Invalid left child index.");
        VASSERT(nodeCIndex >= 0 && nodeCIndex < static_cast<i32>(_nodes.size()), "Invalid right child index.");

        AABBTreeNode &nodeB = _nodes[nodeBIndex];
        AABBTreeNode &nodeC = _nodes[nodeCIndex];

        const i32 balanceFactor = nodeC.Height - nodeB.Height;

        if (balanceFactor < -1) {
            VASSERT(!nodeB.IsLeaf(), "Left child should not be a leaf if balance factor is less than -1.");

            const i32 nodeDIndex = nodeB.Children[0];
            const i32 nodeEIndex = nodeB.Children[1];

            VASSERT(nodeDIndex >= 0 && nodeDIndex < static_cast<i32>(_nodes.size()), "Invalid left-left child index.");
            VASSERT(nodeEIndex >= 0 && nodeEIndex < static_cast<i32>(_nodes.size()), "Invalid left-right child index.");

            AABBTreeNode &nodeD = _nodes[nodeDIndex];
            AABBTreeNode &nodeE = _nodes[nodeEIndex];

            nodeB.Children[0] = nodeIndex;
            nodeB.ParentNodeIndex = nodeA.ParentNodeIndex;
            nodeA.ParentNodeIndex = nodeBIndex;

            if (nodeB.ParentNodeIndex != AABB_TREE_NULL_NODE) {

                if (_nodes[nodeB.ParentNodeIndex].Children[0] == nodeIndex) {
                    _nodes[nodeB.ParentNodeIndex].Children[0] = nodeBIndex;
                } else {
                    VASSERT(_nodes[nodeB.ParentNodeIndex].Children[1] == nodeIndex, "Node A should be a child of its parent.");
                    _nodes[nodeB.ParentNodeIndex].Children[1] = nodeBIndex;
                }

            } else {
                _rootNodeIndex = nodeBIndex;
            }

            VASSERT(!nodeB.IsLeaf() && !nodeA.IsLeaf(), "Node B and Node A should not be a leaf after rotation.");

            if (nodeD.Height > nodeE.Height) {
                nodeB.Children[1] = nodeDIndex;
                nodeA.Children[0] = nodeEIndex;
                nodeE.ParentNodeIndex = nodeIndex;

                nodeA.AABB.MergeTwoAABBs(nodeC.AABB, nodeE.AABB);
                nodeB.AABB.MergeTwoAABBs(nodeA.AABB, nodeD.AABB);

                nodeA.Height = 1 + std::max(nodeC.Height, nodeE.Height);
                nodeB.Height = 1 + std::max(nodeA.Height, nodeD.Height);

                VASSERT(nodeA.Height > 0 && nodeB.Height > 0, "Node heights should be greater than 0 after rotation.");
            } else {
                nodeB.Children[1] = nodeEIndex;
                nodeA.Children[0] = nodeDIndex;
                nodeD.ParentNodeIndex = nodeIndex;

                nodeA.AABB.MergeTwoAABBs(nodeC.AABB, nodeD.AABB);
                nodeB.AABB.MergeTwoAABBs(nodeA.AABB, nodeE.AABB);

                nodeA.Height = 1 + std::max(nodeC.Height, nodeD.Height);
                nodeB.Height = 1 + std::max(nodeA.Height, nodeE.Height);

                VASSERT(nodeA.Height > 0 && nodeB.Height > 0, "Node heights should be greater than 0 after rotation.");
            }

            return nodeBIndex;
        }

        if (balanceFactor > 1) {
            VASSERT(!nodeC.IsLeaf(), "Right child should not be a leaf if balance factor is greater than 1.");

            const i32 nodeFIndex = nodeC.Children[0];
            const i32 nodeGIndex = nodeC.Children[1];

            VASSERT(nodeFIndex >= 0 && nodeFIndex < static_cast<i32>(_nodes.size()), "Invalid right-left child index.");
            VASSERT(nodeGIndex >= 0 && nodeGIndex < static_cast<i32>(_nodes.size()), "Invalid right-right child index.");

            AABBTreeNode &nodeF = _nodes[nodeFIndex];
            AABBTreeNode &nodeG = _nodes[nodeGIndex];

            nodeC.Children[0] = nodeIndex;
            nodeC.ParentNodeIndex = nodeA.ParentNodeIndex;
            nodeA.ParentNodeIndex = nodeCIndex;

            if (nodeC.ParentNodeIndex != AABB_TREE_NULL_NODE) {

                if (_nodes[nodeC.ParentNodeIndex].Children[0] == nodeIndex) {
                    _nodes[nodeC.ParentNodeIndex].Children[0] = nodeCIndex;
                } else {
                    VASSERT(_nodes[nodeC.ParentNodeIndex].Children[1] == nodeIndex, "Node A should be a child of its parent.");
                    _nodes[nodeC.ParentNodeIndex].Children[1] = nodeCIndex;
                }

            } else {
                _rootNodeIndex = nodeCIndex;
            }

            VASSERT(!nodeC.IsLeaf() && !nodeA.IsLeaf(), "Node C and Node A should not be a leaf after rotation.");

            if (nodeF.Height > nodeG.Height) {
                nodeC.Children[1] = nodeFIndex;
                nodeA.Children[1] = nodeGIndex;
                nodeG.ParentNodeIndex = nodeIndex;

                nodeA.AABB.MergeTwoAABBs(nodeB.AABB, nodeG.AABB);
                nodeC.AABB.MergeTwoAABBs(nodeA.AABB, nodeF.AABB);

                nodeA.Height = 1 + std::max(nodeB.Height, nodeG.Height);
                nodeC.Height = 1 + std::max(nodeA.Height, nodeF.Height);

                VASSERT(nodeA.Height > 0 && nodeC.Height > 0, "Node heights should be greater than 0 after rotation.");
            } else {
                nodeC.Children[1] = nodeGIndex;
                nodeA.Children[1] = nodeFIndex;
                nodeF.ParentNodeIndex = nodeIndex;

                nodeA.AABB.MergeTwoAABBs(nodeB.AABB, nodeF.AABB);
                nodeC.AABB.MergeTwoAABBs(nodeA.AABB, nodeG.AABB);

                nodeA.Height = 1 + std::max(nodeB.Height, nodeF.Height);
                nodeC.Height = 1 + std::max(nodeA.Height, nodeG.Height);

                VASSERT(nodeA.Height > 0 && nodeC.Height > 0, "Node heights should be greater than 0 after rotation.");
            }

            return nodeCIndex;
        }

        return nodeIndex;
    }

    void DynamicAABBTree::releaseNode(i32 nodeIndex) {
        VASSERT(!_nodes.empty(), "Node pool should not be empty when releasing a node.");
        VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
        VASSERT(_nodes[nodeIndex].Height >= 0, "Node being released should be currently allocated.");

        _nodes[nodeIndex].NextNodeIndex = _firstFreeNodeIndex;
        _nodes[nodeIndex].Height = -1;
        _firstFreeNodeIndex = nodeIndex;
    }

    i32 DynamicAABBTree::allocateNode() {
        if (_firstFreeNodeIndex == AABB_TREE_NULL_NODE) {
            size_t oldNodeCount = _nodes.size();

            VASSERT(oldNodeCount * 2 <= static_cast<size_t>(std::numeric_limits<i32>::max()), "Exceeded maximum node capacity.");

            _nodes.resize(oldNodeCount * 2);

            for (size_t i = oldNodeCount; i < _nodes.size() - 1; ++i) {
                _nodes[i].NextNodeIndex = i + 1;
            }

            _firstFreeNodeIndex = static_cast<i32>(oldNodeCount);
        }

        i32 allocatedNodeIndex = _firstFreeNodeIndex;
        _firstFreeNodeIndex = _nodes[allocatedNodeIndex].NextNodeIndex;
        _nodes[allocatedNodeIndex].Height = 0;

        return allocatedNodeIndex;
    }

} // namespace Vulkyrie
