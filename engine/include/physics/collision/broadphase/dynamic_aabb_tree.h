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

            DynamicAABBTreeNode()
                : AABB(glm::vec3(0.0f), glm::vec3(0.0f))
                , NextNodeID(NULL_NODE)
                , Height(-1) {};

            [[nodiscard]] VE_FORCE_INLINE bool IsLeaf() const {
                return Height == 0;
            }
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
