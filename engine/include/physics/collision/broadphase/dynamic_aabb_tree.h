#pragma once

#include "physics/collision/shapes/aabb.h"

namespace Vulkyrie {

    struct DynamicAABBTreeNode final {
        public:
            static constexpr size_t NULL_NODE = -1;

            Vulkyrie::AABB AABB;
            size_t LeftChildID;
            size_t RightChildID;

            union {
                    size_t ParentNodeID;
                    size_t NextNodeID;
            };

            i32 Height;

            DynamicAABBTreeNode();
    };

    class DynamicAABBTree final {
        public:
            DynamicAABBTree();

            ~DynamicAABBTree() = default;

            void AddObject(const AABB &aabb);
            void RemoveObject(size_t nodeId);
            void UpdateObject(size_t nodeId, const AABB &newAABB);

        private:
            std::vector<DynamicAABBTreeNode> _nodes;
            size_t _rootNodeId;
    };

} // namespace Vulkyrie
