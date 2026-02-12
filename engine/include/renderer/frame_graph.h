#pragma once

#include "core/uuid.h"

namespace Vulkyrie::Renderer {

    using FrameGraphResource = u32;

    class GraphNode {
        public:
            GraphNode() = delete;
            GraphNode(const GraphNode &) = delete;
            GraphNode(GraphNode &&) = delete;
            GraphNode &operator=(const GraphNode &) = delete;
            GraphNode &operator=(GraphNode &&) = delete;

            [[nodiscard]] inline std::string_view GetName() const {
                return _name;
            }

            [[nodiscard]] inline u32 GetRefCount() const {
                return _refCount;
            }

            [[nodiscard]] inline Core::UUID GetID() const {
                return _id;
            }

        protected:
            GraphNode(std::string_view name, Core::UUID id = {})
                : _name(name)
                , _id(id)
                , _refCount(0) {
            }

        private:
            std::string_view _name;
            Core::UUID _id;
            u32 _refCount;
    };

    class FrameGraphBuilder {
        public:
    };

    class RenderPass {
        public:
            virtual ~RenderPass() = default;
    };

    class FrameGraph {
        public:
            FrameGraph() = default;
            ~FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph &operator=(const FrameGraph &) = delete;

            FrameGraph(FrameGraph &&) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;
    };

} // namespace Vulkyrie::Renderer
