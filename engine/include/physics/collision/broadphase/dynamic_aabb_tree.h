#pragma once

#include "core/asserts.h"
#include "core/pair.h"
#include "physics/physics_constants.h"
#include "physics/collision/shapes/aabb.h"

namespace Vulkyrie {

    /** @brief A node in the Dynamic AABB Tree. Each node contains an AABB that represents the spatial bounds of the node, as well as information about its
     * children, parent, and height in the tree. Leaf nodes represent actual objects in the world and contain user-defined data, while internal nodes are
     * used for spatial partitioning and do not contain user data. The structure is designed to be memory-efficient and to support fast queries for potential
     * collisions between objects by organizing them hierarchically based on their AABBs. */
    struct AABBTreeNode final {
    public:
        /** @brief Default constructor for the AABBTreeNode struct. Initializes the AABB to a default state with zero size, sets the child indices to null,
         * and marks the node as free by setting its height to -1. This constructor is used when creating new nodes in the tree, and the node will be
         * properly initialized with valid data when it is allocated and added to the tree. */
        AABBTreeNode();

        /** @brief The AABB that represents the spatial bounds of the node in the tree. For leaf nodes, this AABB is an inflated version of the
         * original AABB of the object to allow for some movement without needing to update the tree immediately. For internal nodes, this AABB encompasses
         * the AABBs of all descendant leaf nodes and is used for efficient spatial queries and collision detection. The AABB is a fundamental part of the
         * tree's structure and is used to optimize various operations such as insertion, removal, and querying for overlaps. */
        Vulkyrie::AABB AABB;

        union {
        public:
            /** @brief The indices of the child nodes in the tree. For internal nodes, these indices point to the left and right child nodes that are
             * used for spatial partitioning. For leaf nodes, these indices are not used and can be ignored. */
            i32 Children[2];

            /** @brief The user-defined data associated with a leaf node. For internal nodes, this field is not used and can be ignored. The caller is
             * responsible for ensuring that the data associated with leaf nodes remains valid for the duration of its use in the tree. */
            i32 Data;

            /** @brief A pointer to user-defined data associated with a leaf node. This can be used as an alternative to the integer Data field for
             * storing more complex information about the object represented by the leaf node. For internal nodes, this field is not used and can be
             * ignored. The caller is responsible for ensuring that the data pointed to by DataPointer remains valid for the duration of its use in the
             * tree. */
            void *DataPointer;
        };

        union {
        public:
            /** @brief The index of the parent node in the tree. For the root node, this index is set to a special null value (e.g., -1) to indicate
             * that it has no parent. For all other nodes, this index points to the parent node that contains this node as one of its children. The
             * parent node is used for navigating up the tree during operations such as insertion, removal, and balancing. */
            i32 ParentNodeIndex;

            /** @brief The index of the next free node in the tree's node pool. This is used for efficient allocation and deallocation of nodes when
             * adding or removing objects from the tree. When a node is allocated, it is removed from the free list, and when it is released, it is
             * added back to the free list. This allows the tree to reuse nodes without needing to perform expensive dynamic memory allocations for each
             * new node. */
            i32 NextNodeIndex;
        };

        /** @brief The height of the node in the tree. A leaf node has a height of 0, while internal nodes have a height greater than 0. The height is used
         * to determine if a node is a leaf or an internal node, and it can also be used for balancing the tree during insertions and removals. A height of
         * -1 indicates that the node is currently free and not part of the tree. */
        i32 Height;

        /** @brief Checks if this node is a leaf node in the AABB tree. A leaf node is defined as a node that has a height of 0, which means it does
         * not have any child nodes and represents an actual object in the world. Internal nodes, on the other hand, have a height greater than 0
         * and are used for spatial partitioning to optimize collision detection.
         * @returns True if this node is a leaf node (height == 0), false otherwise. */
        [[nodiscard]] VE_FORCE_INLINE bool IsLeaf() const {
            return Height == 0;
        }
    };

    /** @brief A Dynamic AABB Tree is a spatial data structure used for efficient broad-phase collision detection in physics simulations. It organizes objects
     * in a hierarchical manner based on their axis-aligned bounding boxes (AABBs), allowing for fast queries to find potential collisions between objects. The
     * tree is "dynamic" because it can efficiently handle the insertion, removal, and movement of objects in the world without needing to rebuild the entire
     * tree from scratch. This implementation of a Dynamic AABB Tree uses a "fat" AABB approach, where the AABBs of objects are inflated by a certain percentage
     * to create a buffer zone that can accommodate some movement without needing to update the tree immediately. This helps reduce the frequency of tree
     * updates when objects move slightly, which can improve performance in scenarios where objects are frequently moving but not necessarily colliding. The
     * tree is implemented using a vector of nodes, where each node can either be a leaf node representing an actual object in the world or an internal node
     * used for spatial partitioning. The tree supports efficient insertion and removal of objects, as well as querying for potential collisions by traversing
     * the tree and checking for AABB overlaps. */
    class DynamicAABBTree final {
    public:
        /** @brief Constructs a new DynamicAABBTree with the specified inflation percentage and initial node capacity. The inflation percentage
         * determines how much larger the "fat" AABBs are compared to the original AABBs of the objects, which can help reduce the frequency of tree
         * updates when objects move slightly. The initial node capacity is used to pre-allocate memory for the tree's nodes, which can improve
         * performance by reducing the need for dynamic memory allocations as objects are added to the tree. If the tree needs to grow beyond the
         * initial capacity, it will automatically resize itself to accommodate more nodes.
         * @param inflationPercentage The percentage by which to inflate the AABBs of objects when they are added to the tree. This helps reduce the
         * frequency of tree updates when objects move slightly.
         * @param initialNodeCapacity The initial number of nodes to pre-allocate for the tree. This can improve performance by reducing the need
         * for dynamic memory allocations as objects are added to the tree. */
        DynamicAABBTree(f32 inflationPercentage = AABB_TREE_DEFAULT_INFLATION_PERCENTAGE, size_t initialNodeCapacity = AABB_TREE_DEFAULT_INITIAL_NODE_CAPACITY);

        // Delete the copy constructor and copy assignment operator to prevent accidental copying,
        DynamicAABBTree(const DynamicAABBTree &) = delete;
        DynamicAABBTree &operator=(const DynamicAABBTree &) = delete;

        // Delete the move constructor and move assignment operator to prevent accidental moves.
        DynamicAABBTree(DynamicAABBTree &&) = delete;
        DynamicAABBTree &operator=(DynamicAABBTree &&) = delete;

        /** @brief Destructor for the DynamicAABBTree class. */
        ~DynamicAABBTree() = default;

        /** @brief Adds a new object to the tree with the specified AABB and associated data. The AABB will be inflated by the specified percentage
         * to create a "fat" AABB that can accommodate some movement without needing to be updated immediately. The function returns the index of
         * the newly added node in the tree, which can be used for future updates or removals. The caller is responsible for ensuring that the data
         * associated with the node remains valid for the duration of its use in the tree.
         * @param aabb The axis-aligned bounding box representing the spatial bounds of the object being added to the tree. This AABB will be
         * inflated by the specified percentage to create a "fat" AABB that can accommodate some movement without needing to be updated immediately.
         * @param data An integer value representing user-defined data associated with the object being added to the tree. This could be an ID, a
         * type identifier, or any other integer value that helps identify or categorize the object. The caller is responsible for ensuring that
         * this data remains valid for the duration of its use in the tree.
         * @returns The index of the newly added node in the tree, which can be used for future updates or removals. */
        VE_FORCE_INLINE i32 AddObject(const AABB &aabb, i32 data) {
            // Allocate a new node from the pool of available nodes.
            // This will give us an index into the _nodes vector where we can store the new node's data.
            const i32 nodeIndex = allocateNode();

            // Inflate the AABB by the specified percentage to create a "fat" AABB that
            // can accommodate some movement without needing to be updated immediately.
            const glm::vec3 inflation(aabb.GetExtents() * _inflationPercentage);
            _nodes[nodeIndex].AABB.SetMinMax(aabb.GetMin() - inflation, aabb.GetMax() + inflation);

            // Set the height of the new leaf node to 0.
            _nodes[nodeIndex].Height = 0;

            // Store the user-defined data in the node.
            _nodes[nodeIndex].Data = data;

            // Insert the new leaf node into the tree, which may cause the tree to rebalance if necessary.
            insertLeafNode(nodeIndex);

            VASSERT(_nodes[nodeIndex].IsLeaf(), "Added node should be a leaf.");

            // Return the index of the newly inserted node.
            return nodeIndex;
        }

        /** @brief Adds a new object to the tree with the specified AABB and associated data pointer. The AABB will be inflated by the specified
         * percentage to create a "fat" AABB that can accommodate some movement without needing to be updated immediately. The function returns the
         * index of the newly added node in the tree, which can be used for future updates or removals. The caller is responsible for ensuring that
         * the data pointed to by dataPointer remains valid for the duration of its use in the tree.
         * @param aabb The axis-aligned bounding box representing the spatial bounds of the object being added to the tree. This AABB will be
         * inflated by the specified percentage to create a "fat" AABB that can accommodate some movement without needing to be updated immediately.
         * @param dataPointer A pointer to user-defined data associated with the object being added to the tree. This could point to any type of
         * data structure that helps identify or categorize the object. The caller is responsible for ensuring that this pointer remains valid for
         * the duration of its use in the tree and that it points to a valid memory location containing the intended data.
         * @returns The index of the newly added node in the tree, which can be used for future updates or removals. */
        VE_FORCE_INLINE i32 AddObject(const AABB &aabb, void *dataPointer) {
            // Allocate a new node from the pool of available nodes.
            // This will give us an index into the _nodes vector where we can store the new node's data.
            const i32 nodeIndex = allocateNode();

            // Inflate the AABB by the specified percentage to create a "fat" AABB that
            // can accommodate some movement without needing to be updated immediately.
            const glm::vec3 inflation(aabb.GetExtents() * _inflationPercentage);
            _nodes[nodeIndex].AABB.SetMinMax(aabb.GetMin() - inflation, aabb.GetMax() + inflation);

            // Set the height of the new leaf node to 0.
            _nodes[nodeIndex].Height = 0;

            // Store the user-defined data pointer in the node.
            _nodes[nodeIndex].DataPointer = dataPointer;

            // Insert the new leaf node into the tree, which may cause the tree to rebalance if necessary.
            insertLeafNode(nodeIndex);

            VASSERT(_nodes[nodeIndex].IsLeaf(), "Added node should be a leaf.");

            // Return the index of the newly inserted node.
            return nodeIndex;
        }

        /** @brief Updates the AABB of an existing object in the tree. The node index must correspond to a valid leaf node in the tree, and the new AABB
         * will be used to update the "fat" AABB stored for that node. If the new AABB is still contained within the existing fat AABB and we're not
         * forcing a reinsert, the function will return false and the tree will not be updated. Otherwise, the node will be removed from the tree and
         * reinserted with its updated AABB, which may cause the tree to rebalance if necessary. The function returns true if the tree was updated and
         * may need to be re-queried for collisions, or false if no update was necessary.
         * @param nodeIndex The index of the node to update in the tree. This must be a valid index for a leaf node.
         * @param newAABB The new axis-aligned bounding box representing the spatial bounds of the object associated with the specified node index. This
         * AABB will be used to update the "fat" AABB stored for that node in the tree.
         * @param forceReinsert If true, forces the node to be removed and reinserted even if the new AABB is still contained within the existing fat
         * AABB. This can be useful if you want to ensure that any changes to the object's position or size are reflected in the tree immediately, even
         * if they don't require an update based on containment checks.
         * @returns True if the tree was updated and may need to be re-queried for collisions, or false if no update was necessary because the new AABB
         * is still contained within the existing fat AABB and we're not forcing a reinsert. */
        bool UpdateObject(i32 nodeIndex, const AABB &newAABB, bool forceReinsert);

        /** @brief Removes the object corresponding to the specified node index from the tree. The node index must correspond to a valid leaf node
         * in the tree, and after removal, the node will be released back to the free list for future use. The caller is responsible for ensuring
         * that any data associated with the removed node is properly cleaned up or invalidated as needed, since the tree will no longer maintain
         * any references to it after removal.
         * @param nodeIndex The index of the node to remove from the tree. This must be a valid index into the _nodes vector and must correspond to
         * a leaf node. */
        VE_FORCE_INLINE void RemoveObject(i32 nodeIndex) {
            VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
            VASSERT(_nodes[nodeIndex].IsLeaf(), "Can only remove leaf nodes.");

            // Remove the leaf node from the tree, which may cause the tree to rebalance if necessary.
            removeLeafNode(nodeIndex);

            // Release the node back to the free list so it can be reused for future insertions.
            releaseNode(nodeIndex);
        }

        /** @brief Returns the AABB of the specified node index. The node index must correspond to a valid node in the tree, and the returned AABB
         * will be the "fat" AABB that was stored for that node, which may be larger than the original AABB of the object to allow for some movement
         * without needing immediate updates. This function is useful for retrieving the spatial bounds of a node for collision checks or other
         * spatial queries. The caller is responsible for ensuring that the node index is valid and that the returned AABB is used safely within its
         * intended context.
         * @param nodeIndex The index of the node to retrieve the AABB from. This must be a valid index into the _nodes vector.
         * @returns The AABB associated with the specified node index. */
        [[nodiscard]] VE_FORCE_INLINE const AABB &GetFatAABB(i32 nodeIndex) const {
            VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");

            return _nodes[nodeIndex].AABB;
        }

        /** @brief Returns the AABB of the root node, which encompasses all objects in the tree. This is useful for quickly determining the overall
         * bounds of the scene or for broad-phase culling before performing more detailed collision checks. If the tree is empty, this will assert
         * since there is no valid root node. */
        [[nodiscard]] VE_FORCE_INLINE const AABB &GetRootNodeAABB() const {
            VASSERT(_rootNodeIndex != AABB_TREE_NULL_NODE, "Tree is empty.");

            return _nodes[_rootNodeIndex].AABB;
        }

        /** @brief Returns the data associated with the leaf node.
         * @param nodeIndex The index of the leaf node to retrieve the data from. Must be a valid index into the _nodes vector and must correspond
         * to a leaf node.
         * @returns The data associated with the leaf node. */
        [[nodiscard]] VE_FORCE_INLINE i32 GetNodeData(i32 nodeIndex) const {
            VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
            VASSERT(_nodes[nodeIndex].IsLeaf(), "Can only get data from leaf nodes.");

            return _nodes[nodeIndex].Data;
        }

        /** @brief Returns a pointer to the data associated with the leaf node. The caller is responsible for ensuring that the pointer is used
         * safely and that the data it points to remains valid for the duration of its use.
         * @param nodeIndex The index of the leaf node to retrieve the data pointer from. Must be a valid index into the _nodes vector and must
         * correspond to a leaf node.
         * @returns A pointer to the data associated with the leaf node. */
        [[nodiscard]] VE_FORCE_INLINE void *GetNodeDataPointer(i32 nodeIndex) const {
            VASSERT(nodeIndex >= 0 && nodeIndex < static_cast<i32>(_nodes.size()), "Invalid node index.");
            VASSERT(_nodes[nodeIndex].IsLeaf(), "Can only get data from leaf nodes.");

            return _nodes[nodeIndex].DataPointer;
        }

        /** @brief Queries the tree for all leaf nodes whose AABBs overlap with the specified query AABB. The results are returned as a vector of
         * node indices, which can be used to retrieve the associated data for each overlapping node. This is useful for broad-phase collision
         * detection, where you want to quickly find potential collisions before performing more detailed checks.
         * @param queryAABB The AABB to query against. All leaf nodes whose AABBs overlap with this AABB will be included in the results.
         * @param outResults A vector that will be populated with the indices of the overlapping leaf nodes. The caller is responsible for clearing
         * this vector before calling the function if they want to avoid appending to existing results. */
        void QueryOverlaps(const AABB &queryAABB, std::vector<i32> &outResults);

        /** @brief Queries the tree for all pairs of leaf nodes that overlap with each other among the specified node indices. This is useful for
         * finding potential collisions between objects in the tree without needing to specify a separate query AABB, as it will check for overlaps
         * between all pairs of nodes in the input vector. The results are returned as a vector of pairs of node indices, where each pair represents
         * two overlapping nodes. To avoid duplicate pairs (e.g., both (A, B) and (B, A)), the function can enforce a consistent ordering of the
         * indices in the pairs.
         * @param nodeIndices A vector of node indices to check for overlaps. Each index must be a valid index into the _nodes vector and should
         * correspond to a leaf node.
         * @param outOverlappingPairs A vector that will be populated with pairs of indices representing overlapping nodes. The caller is
         * responsible for clearing this vector before calling the function if they want to avoid appending to existing results. */
        void QueryOverlappingPairs(const std::vector<i32> &nodeIndices, std::vector<Pair<i32, i32>> &outOverlappingPairs);

    private:
        /** @brief The vector of nodes in the dynamic AABB tree. Each node represents either a leaf (which corresponds to an actual object in the
         * world) or an internal node (which is used for spatial partitioning and does not correspond to a real object). The tree is stored as a
         * contiguous array of nodes, where the parent-child relationships are maintained through indices. The root node is at index _rootNodeIndex,
         * and each node's LeftChildIndex and RightChildIndex point to its children in the vector. Leaf nodes have their LeftChildIndex and
         * RightChildIndex set to AABB_TREE_NULL_NODE. */
        std::vector<AABBTreeNode> _nodes;

        // WARN: This is not thread-safe. If you need to perform queries from multiple threads,
        // you should use a separate instance of this vector for each thread.
        std::vector<i32> _queryNodesToVisit;

        /** @brief The index of the root node in the _nodes vector. This is used to quickly access the top-level AABB that encompasses all objects
         * in the tree. If the tree is empty, this will be set to AABB_TREE_NULL_NODE. */
        i32 _rootNodeIndex;

        /** @brief The index of the first free node in the _nodes vector. This is used to efficiently manage the allocation and deallocation of
         * nodes in the tree. When a new node is needed, it can be taken from the free list starting at this index, and when a node is removed, it
         * can be added back to the free list. This helps to minimize memory fragmentation and improve performance by reusing existing nodes instead
         * of constantly resizing the vector. */
        i32 _firstFreeNodeIndex;

        /** @brief The percentage by which to inflate the AABBs of leaf nodes when they are inserted or updated. This creates "fat" AABBs that can
         * accommodate some movement without needing to be updated immediately, which can improve performance by reducing the frequency of tree
         * updates at the cost of potentially more false positives during collision queries. For example, an inflation percentage of 0.1 would
         * inflate the AABB by 10% of its extents in each direction. */
        f32 _inflationPercentage;

        /** @brief Removes a leaf node from the tree. This function is called when an object is removed from the tree, and it will update the tree structure
         * accordingly to maintain the properties of the tree. The caller is responsible for ensuring that the specified node index corresponds to a valid
         * leaf node in the tree, and after this function is called, the node will be removed from the tree and should not be used until it is reallocated
         * and reinserted.
         * @param leafNodeIndex The index of the leaf node to remove from the tree. This must be a valid index into the _nodes vector and must correspond to
         * a leaf node. After this function is called, the node will be removed from the tree and should not be used until it is reallocated and reinserted.
         * */
        void removeLeafNode(i32 leafNodeIndex);

        /** @brief Inserts a leaf node into the tree. This function is called when a new object is added to the tree, and it will find the appropriate
         * location for the new leaf node based on its AABB and insert it into the tree structure. The function will also perform any necessary balancing of
         * the tree after insertion to maintain its properties and ensure efficient queries. The caller is responsible for ensuring that the specified node
         * index corresponds to a valid leaf node in the tree, and after this function is called, the node will be part of the tree and can be used for
         * queries and updates.
         * @param leafNodeIndex The index of the leaf node to insert into the tree. This must be a valid index into the _nodes vector and must correspond
         * to a leaf node. After this function is called, the node will be part of the tree and can be used for queries and updates. */
        void insertLeafNode(i32 leafNodeIndex);

        /** @brief Releases a node back to the free list. This function is called when a node is removed from the tree, and it adds the node back to the
         * free list so that it can be reused for future insertions.
         * @param nodeIndex The index of the node to release back to the free list. This must be a valid index into the _nodes vector. */
        void releaseNode(i32 nodeIndex);

        /** @brief Balances the subtree rooted at the specified node index. This function is called after inserting or removing a leaf node to ensure that
         * the tree remains balanced and efficient for queries. The balancing process may involve performing rotations on the subtree to maintain
         * the properties of the tree, such as keeping the heights of child nodes within a certain range. The function returns the new index of the
         * root of the balanced subtree, which may be different from the input node index if rotations were performed. The caller is responsible for
         * updating any parent nodes accordingly after balancing.
         * @param nodeIndex The index of the node at the root of the subtree to balance. This should be a valid index into the _nodes vector.
         * @returns The new index of the root of the balanced subtree after performing any necessary rotations. */
        i32 balanceSubtree(i32 nodeIndex);

        /** @brief Allocates a new node from the pool of available nodes. This function will check if there are any free nodes in the free list, and if
         * so, it will return the index of the first free node and update the free list accordingly. If there are no free nodes available, it will
         * resize the _nodes vector to create more nodes and then return the index of the newly allocated node. The caller is responsible for ensuring
         * that the returned node index is used to properly initialize the node's data before inserting it into the tree. */
        i32 allocateNode();
    };

} // namespace Vulkyrie
