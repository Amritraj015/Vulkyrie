#pragma once

#include "physics/collision/shapes/aabb.h"

namespace Vulkyrie {

    // struct DynamicAABBTreeNode final {
    //     public:
    //         static constexpr i32 NULL_NODE = -1;
    //
    //         Vulkyrie::AABB AABB;
    //         i32 Height;
    //
    //         union {
    //                 i32 ParentNodeID;
    //                 i32 NextNodeID;
    //         };
    //
    //         union {
    //                 struct Children {
    //                         i32 LeftChildNodeID;
    //                         i32 RightChildNodeID;
    //                 };
    //                 Entity Entity;
    //         };
    //
    //         DynamicAABBTreeNode()
    // };
    //
    // class DynamicAABBTree final {
    //     public:
    //     private:
    //         std::vector<DynamicAABBTreeNode> _nodes;
    //         i32 _rootNodeId;
    // };

} // namespace Vulkyrie
