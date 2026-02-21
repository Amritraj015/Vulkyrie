#pragma once

#include "renderer/frame_graph/pass_node.h"

namespace Vulkyrie::Renderer {
    class PassNode;

    using ResourceID = size_t;

    class ResourceNode final {
            friend class FrameGraph;

        public:
            ResourceNode(const std::string_view name, ResourceID resourceID, u32 version = 1U)
                : _name(name)
                , _resourceID(resourceID)
                , _refCount(0)
                , _version{ version } {
            }

            ResourceNode(const ResourceNode &) = delete;
            ResourceNode &operator=(const ResourceNode &) = delete;

            ResourceNode(ResourceNode &&) = default;
            ResourceNode &operator=(ResourceNode &&) = delete;

            [[nodiscard]] inline std::string_view GetName() const {
                return _name;
            }

            [[nodiscard]] inline size_t GetRefCount() const {
                return _refCount;
            }

            [[nodiscard]] inline ResourceID GetResourceID() const {
                return _resourceID;
            }

            [[nodiscard]] inline bool Transient() const {
                // return _writtenBy.empty();
                return false;
            }

            [[nodiscard]] inline u32 GetVersion() const {
                return _version;
            }

        private:
            const std::string_view _name;
            const ResourceID _resourceID;
            size_t _refCount;
            const u32 _version;
            PassNode *_creator{ nullptr };
            PassNode *_lastUsedBy{ nullptr };
    };
} // namespace Vulkyrie::Renderer
