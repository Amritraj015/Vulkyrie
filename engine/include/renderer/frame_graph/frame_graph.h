#pragma once

#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/resource_node.h"

namespace Vulkyrie::Renderer {

    class FrameGraph final {
        public:
            FrameGraph() = default;
            ~FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph &operator=(const FrameGraph &) = delete;

            FrameGraph(FrameGraph &&) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;

            class Builder final {
                    friend class FrameGraph;

                public:
                    Builder() = delete;
                    ~Builder() = default;

                    Builder(const Builder &) = delete;
                    Builder(Builder &&) = delete;

                    Builder &operator=(const Builder &) = delete;
                    Builder &operator=(Builder &&) = delete;

                    template <FrameGraphResourceBackend T>
                    [[nodiscard]] ResourceID Create(const std::string_view name, const typename T::Descriptor &descriptor) {

                        // static_assert(std::is_same<typename resource_type::description_type, description_type>::value, "Description does not match the
                        // resource.");

                        const ResourceID resourceId = _frameGraph._resourceNodes.size();
                        const ResourceNode &resource = _frameGraph._resourceNodes.emplace_back(name, resourceId);
                        _passNode._creates.push_back(resource.GetResourceID());

                        return resource.GetResourceID();
                    }

                    [[nodiscard]] ResourceID Read(const ResourceID resourceId) {
                        if (_passNode.ReadsResource(resourceId)) {
                            return resourceId;
                        }

                        ResourceNode &resource = _frameGraph._resourceNodes[resourceId];
                        resource._readBy.push_back(_passNode.GetPassID());
                        _passNode._reads.push_back(resource.GetResourceID());

                        return resource.GetResourceID();
                    }

                    [[nodiscard]] ResourceID Write(const ResourceID resourceId) {
                        if (_passNode.WritesToResource(resourceId)) {
                            return resourceId;
                        }

                        ResourceNode &resource = _frameGraph._resourceNodes[resourceId];
                        resource._writtenBy.push_back(_passNode.GetPassID());
                        _passNode._writes.push_back(resource.GetResourceID());

                        return resource.GetResourceID();
                    }

                private:
                    Builder(FrameGraph &fg, PassNode &node)
                        : _frameGraph{ fg }
                        , _passNode{ node } {
                    }

                    FrameGraph &_frameGraph;
                    PassNode &_passNode;
            };

            [[nodiscard]] bool IsValid() const;

            template <typename PassData, typename SetupFunc, typename ExecuteFunc>
                requires std::is_invocable_v<SetupFunc, Builder &, PassData &> && std::is_invocable_v<ExecuteFunc, const PassData &> &&
                         (sizeof(ExecuteFunc) <= 1024)
            const PassData &AddPass(const std::string_view name, SetupFunc &&setupFunc, ExecuteFunc &&executeFunc) {
                auto pass = std::make_unique<FrameGraphPass<PassData, ExecuteFunc>>(std::forward<ExecuteFunc>(executeFunc));
                auto &passNode = CreatePassNode(name, std::move(pass));

                Builder builder{ *this, passNode };
                setupFunc(builder, pass->data);

                // TODO: WATCHOUT!!!
                // TODO: WATCHOUT!!!
                return passNode->Data;
                // TODO: WATCHOUT!!!
                // TODO: WATCHOUT!!!
            }

        private:
            std::vector<PassNode> _passNodes;
            std::vector<ResourceNode> _resourceNodes;

            [[nodiscard]] PassNode &CreatePassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept> &&executionFunction) {
                return _passNodes.emplace_back(name, _passNodes.size(), std::move(executionFunction));
            }

            [[nodiscard]] ResourceID Clone(ResourceID resourse);
    };

} // namespace Vulkyrie::Renderer
