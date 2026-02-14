#pragma once

#include "vlkypch.h"

namespace Vulkyrie::Renderer {

    using ResourceID = i32;

    class GraphNode {
        public:
            GraphNode() = delete;
            GraphNode(const GraphNode &) = delete;
            GraphNode(GraphNode &&) = delete;
            GraphNode &operator=(const GraphNode &) = delete;
            GraphNode &operator=(GraphNode &&) = delete;

            virtual void Execute() = 0;

            [[nodiscard]] inline std::string_view GetName() const {
                return _name;
            }

            [[nodiscard]] inline u32 GetRefCount() const {
                return _refCount;
            }

            [[nodiscard]] inline u32 GetID() const {
                return _index;
            }

        protected:
            GraphNode(const std::string_view name, u32 index)
                : _name(name)
                , _index(index)
                , _refCount(0) {
            }

        private:
            const std::string_view _name;
            const u32 _index;
            u32 _refCount;
    };

    template <typename PassData, typename ExecuteFunc>
        requires std::is_invocable_v<ExecuteFunc, const PassData &>
    class PassNode : public GraphNode {
        public:
            PassNode(const std::string_view name, u32 index, ExecuteFunc &&executeFunc)
                : GraphNode(name, index)
                , _executeFunc(std::forward<ExecuteFunc>(executeFunc)) {
                _readResources.reserve(20);
                _writeResources.reserve(20);
                _createdResources.reserve(20);
            }

            void Execute() override {
                _executeFunc(Data);
            }

            PassData Data;

            [[nodiscard]] bool CreatesResource(ResourceID resource) const {
                return std::ranges::find(_createdResources, resource) != _createdResources.cend();
            }

            [[nodiscard]] bool ReadsResource(ResourceID resource) const {
                return std::ranges::find(_readResources, resource) != _readResources.cend();
            }

            [[nodiscard]] bool WritesToResource(ResourceID resource) const {
                return std::ranges::find(_writeResources, resource) != _writeResources.cend();
            }

        private:
            ExecuteFunc _executeFunc;
            std::vector<ResourceID> _readResources;
            std::vector<ResourceID> _writeResources;
            std::vector<ResourceID> _createdResources;
    };

    class ResourceNode : public GraphNode {
        public:
            ResourceNode(const ResourceNode &) = delete;
            ResourceNode(ResourceNode &&) noexcept = delete;
            ResourceNode &operator=(const ResourceNode &) = delete;
            ResourceNode &operator=(ResourceNode &&) noexcept = delete;

            [[nodiscard]] auto GetResourceId() const {
                return _resourceId;
            }

            [[nodiscard]] auto GetVersion() const {
                return _version;
            }

        private:
            ResourceNode(const std::string_view name, uint32_t nodeId, uint32_t resourceId, uint32_t version)
                : GraphNode{ name, nodeId }
                , _resourceId{ resourceId }
                , _version{ version } {
            }

            // Index to virtual resource (FrameGraph::m_resourceRegistry).
            const uint32_t _resourceId;
            const uint32_t _version;

            // PassNode *m_producer{ nullptr };
            // PassNode *m_last{ nullptr };
    };

    class RenderGraph {
        public:
            RenderGraph() = default;
            RenderGraph(const RenderGraph &) = delete;
            RenderGraph(RenderGraph &&) = delete;

            RenderGraph &operator=(const RenderGraph &) = delete;
            RenderGraph &operator=(RenderGraph &&) = delete;

            void Compile();

            void Execute() {
                for (auto &passNode : _passNodes) {
                    passNode->Execute();
                }
            }

            class Builder {
                public:
                    Builder() = delete;
                    Builder(const Builder &) = delete;
                    Builder(Builder &&) = delete;

                    Builder &operator=(const Builder &) = delete;
                    Builder &operator=(Builder &&) = delete;

                    ResourceID Read(const ResourceID resource);
                    ResourceID Write(const ResourceID resource);

                private:
                    Builder(RenderGraph &fg, GraphNode &node)
                        : _renderGraph{ fg }
                        , _passNode{ node } {
                    }

                    RenderGraph &_renderGraph;
                    GraphNode &_passNode;
            };

            template <typename PassData, typename SetupFunc, typename ExecuteFunc>
                requires std::is_invocable_v<SetupFunc, Builder &, PassData &> && std::is_invocable_v<ExecuteFunc, const PassData &>
            const PassData AddPass(const std::string_view name, SetupFunc &&setupFunc, ExecuteFunc &&executeFunc) {
                const u32 insertIndex = static_cast<u32>(_passNodes.size());

                auto passNode = CreateScope<PassNode<PassData, ExecuteFunc>>(name, insertIndex, executeFunc);

                Builder builder(*this, *passNode);

                setupFunc(builder, passNode->Data);

                _passNodes.emplace_back(std::move(passNode));

                return passNode->Data;
            }

        private:
            std::vector<Scope<GraphNode>> _passNodes;
    };

} // namespace Vulkyrie::Renderer
