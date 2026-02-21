#pragma once

#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/resource_node.h"
#include "renderer/frame_graph/resource_entry.h"
#include <stack>

namespace Vulkyrie::Renderer {

    class FrameGraph final {
        public:
            FrameGraph() = default;
            ~FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph &operator=(const FrameGraph &) = delete;

            FrameGraph(FrameGraph &&) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;

            void Compile() {
                for (PassNode &passNode : _passNodes) {
                    passNode._refCount = passNode._writes.size();

                    for (const ResourceID resourceId : passNode._reads) {
                        auto &consumedResource = _resourceNodes[resourceId];
                        consumedResource._refCount++;
                    }

                    for (const ResourceID resourceId : passNode._writes) {
                        auto &writtenToResource = _resourceNodes[resourceId];
                        writtenToResource._creator = &passNode;
                    }
                }

                std::stack<ResourceNode *> unreferencedResources;
                for (ResourceNode &resourceNode : _resourceNodes) {
                    if (resourceNode._refCount == 0) {
                        unreferencedResources.push(&resourceNode);
                    }
                }

                while (!unreferencedResources.empty()) {
                    ResourceNode *unreferencedResource = unreferencedResources.top();
                    unreferencedResources.pop();

                    PassNode *creator = unreferencedResource->_creator;

                    if (creator == nullptr || creator->_hasSideEffects) {
                        continue;
                    }

                    assert(creator->_refCount >= 1);

                    if (--creator->_refCount == 0) {
                        for (const ResourceID resourceId : creator->_reads) {
                            ResourceNode &consumedResource = _resourceNodes[resourceId];

                            if (--consumedResource._refCount == 0) {
                                unreferencedResources.push(&consumedResource);
                            }
                        }
                    }
                }

                for (PassNode &passNode : _passNodes) {
                    if (passNode._refCount == 0) {
                        continue;
                    }

                    for (const ResourceID resourceId : passNode._creates) {
                        GetResourceEntry(resourceId).SetCreator(&passNode);
                    }

                    for (const ResourceID resourceId : passNode._writes) {
                        GetResourceEntry(resourceId).SetLastUsedBy(&passNode);
                    }

                    for (const ResourceID resourceId : passNode._reads) {
                        GetResourceEntry(resourceId).SetLastUsedBy(&passNode);
                    }
                }
            }

            [[nodiscard]] ResourceEntry &GetResourceEntry(ResourceID id) {
                return const_cast<ResourceEntry &>(const_cast<const FrameGraph *>(this)->GetResourceEntry(id));
            }

            [[nodiscard]] const ResourceEntry &GetResourceEntry(const ResourceNode &node) const {
                assert(node._resourceID < _resourceRegistry.size());
                return _resourceRegistry[node._resourceID];
            }

            [[nodiscard]] const ResourceEntry &GetResourceEntry(ResourceID id) const {
                return GetResourceEntry(GetResourceNode(id));
            }

            [[nodiscard]] const ResourceNode &GetResourceNode(ResourceID id) const {
                assert(id < _resourceNodes.size());
                return _resourceNodes[id];
            }

            void Execute(void *context, void *allocator) {
                for (const PassNode &passNode : _passNodes) {
                    if (!passNode.CanExecute()) {
                        continue;
                    }
                }
            }

            class Builder final {
                    friend class FrameGraph;

                public:
                    Builder() = delete;
                    ~Builder() = default;

                    Builder(const Builder &) = delete;
                    Builder &operator=(const Builder &) = delete;

                    Builder(Builder &&) = delete;
                    Builder &operator=(Builder &&) = delete;

                    template <FrameGraphResourceBackend T>
                    [[nodiscard]] ResourceID Create(const std::string_view name, const typename T::Descriptor &descriptor) {
                        return 1;
                    }

                    [[nodiscard]] ResourceID Read(const ResourceID resourceId) {
                        return resourceId;
                    }

                    [[nodiscard]] ResourceID Write(const ResourceID resourceId) {
                        return resourceId;
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
            std::vector<ResourceEntry> _resourceRegistry;

            [[nodiscard]] PassNode &CreatePassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept> &&executionFunction) {
                return _passNodes.emplace_back(name, _passNodes.size(), std::move(executionFunction));
            }

            // [[nodiscard]] decltype(auto) _getResourceEntry(FrameGraphResource id) {
            //     return const_cast<ResourceBackend &>(const_cast<const FrameGraph *>(this)->_getResourceEntry(id));
            // }

            [[nodiscard]] ResourceID Clone(ResourceID resourse);
    };

} // namespace Vulkyrie::Renderer
