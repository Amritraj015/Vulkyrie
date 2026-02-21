#pragma once

#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/resource_node.h"
#include "renderer/frame_graph/resource_entry.h"
#include "vlkypch.h"

namespace Vulkyrie::Renderer {

    class FrameGraph final {
        public:
            FrameGraph() = default;
            ~FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph &operator=(const FrameGraph &) = delete;

            FrameGraph(FrameGraph &&) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;

            /** @brief Compiles the frame graph by performing reference count calculation, resource culling, and execution order determination. */
            void Compile() {
                // REFERENCE COUNT CALCULATION
                // First, we calculate the reference counts for each pass and resource node.
                for (PassNode &passNode : _passNodes) {
                    // The reference count of a pass is determined by the number of resources it writes to.
                    passNode._refCount = passNode._writes.size();

                    // The reference count of a resource is determined by the number of passes that read from it.
                    for (const auto [resourceId, _] : passNode._reads) {
                        auto &consumedResource = _resourceNodes[resourceId];
                        consumedResource._refCount++;
                    }

                    // We also set the creator of each resource to the pass that writes
                    for (const auto [resourceId, _] : passNode._writes) {
                        auto &writtenToResource = _resourceNodes[resourceId];
                        writtenToResource._creator = &passNode;
                    }
                }

                // RESOURCE CULLING
                // Next, we identify unreferenced resources and cull any passes that are only referenced by unreferenced resources.
                std::stack<ResourceNode *> unreferencedResources;

                // We start by pushing all resources with a reference count of zero onto the stack.
                for (ResourceNode &resourceNode : _resourceNodes) {
                    if (resourceNode._refCount == 0) {
                        unreferencedResources.push(&resourceNode);
                    }
                }

                // We then repeatedly pop unreferenced resources from the stack and check their creators.
                // If a creator pass has no side effects and its reference count drops to zero,
                // we also consider it unreferenced and push any resources it reads from onto the stack.
                while (!unreferencedResources.empty()) {
                    ResourceNode *unreferencedResource = unreferencedResources.top();
                    unreferencedResources.pop();

                    PassNode *creator = unreferencedResource->_creator;

                    // If the creator is null or has side effects, we cannot cull it, so we skip to the next resource.
                    if (creator == nullptr || creator->_hasSideEffects) {
                        continue;
                    }

                    assert(creator->_refCount >= 1);

                    // We decrement the reference count of the creator pass, and if it drops to zero, we consider it unreferenced.
                    if (--creator->_refCount == 0) {
                        for (const auto [resourceId, _] : creator->_reads) {
                            ResourceNode &consumedResource = _resourceNodes[resourceId];

                            if (--consumedResource._refCount == 0) {
                                unreferencedResources.push(&consumedResource);
                            }
                        }
                    }
                }

                // EXECUTION ORDER DETERMINATION
                // Finally, we determine the execution order of the passes based on their dependencies.
                for (PassNode &passNode : _passNodes) {
                    // We only consider passes that have a positive reference count, as they are the ones that will be executed.
                    if (passNode._refCount == 0) {
                        continue;
                    }

                    // For resources that the pass creates, we set the creator pointer to the pass,
                    for (const ResourceID resourceId : passNode._creates) {
                        GetResourceEntry(resourceId).SetCreator(&passNode);
                    }

                    // For resources that the pass writes to, we set the last used by pointer to the pass,
                    // as it is the last pass that modifies the resource.
                    for (const auto [resourceId, _] : passNode._writes) {
                        GetResourceEntry(resourceId).SetLastUsedBy(&passNode);
                    }

                    // For resources that the pass reads from, we also set the last used by pointer to the pass,
                    // as it is the last pass that consumes the resource before it is potentially destroyed.
                    for (const auto [resourceId, _] : passNode._reads) {
                        GetResourceEntry(resourceId).SetLastUsedBy(&passNode);
                    }
                }
            }

            /** @brief Executes the frame graph by executing all passes in the determined execution order.
             * @param context A pointer to any additional context needed for pass execution, such as a rendering context or command buffer.
             * @param allocator A pointer to a memory allocator that can be used for resource creation and management during pass execution. */
            void Execute(void *context, void *allocator) {
                // We execute the passes in the order they were added to the graph,
                // but we only execute those that have a positive reference count or have side effects.
                for (const PassNode &passNode : _passNodes) {
                    if (!passNode.CanExecute()) {
                        continue;
                    }

                    // Before executing the pass, we need to ensure that all resources it creates are created.
                    for (const ResourceID resourceId : passNode._creates) {
                        GetResourceEntry(resourceId).Create(allocator);
                    }

                    // Perform pre-read operations for resources that the pass reads from.
                    for (const auto [resourceId, flags] : passNode._reads) {
                        if (flags != __IGNORE_FLAGS__) {
                            GetResourceEntry(resourceId).PreRead(flags, allocator);
                        }
                    }

                    // Perform pre-write operations for resources that the pass writes to.
                    for (const auto [resourceId, flags] : passNode._writes) {
                        if (flags != __IGNORE_FLAGS__) {
                            GetResourceEntry(resourceId).PreWrite(flags, allocator);
                        }
                    }

                    // Now we can execute the pass itself.
                    auto &executeFunc = passNode._executeFunc;
                    (*executeFunc)(context);

                    // After executing the pass, we can destroy any resources that are last used by this pass.
                    // The "Destroy" method will only be executed for Transient resources,
                    // as Persistent resources are expected to be managed externally and should not be automatically destroyed by the frame graph.
                    for (auto &resource : _resourceRegistry) {
                        if (resource._lastUsedBy == &passNode) {
                            resource.Destroy(allocator);
                        }
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
                        const auto id = _frameGraph._create<T>(ResourceEntry::Type::Transient, name, descriptor, T{});
                        return _passNode._creates.emplace_back(id);
                    }

                    [[nodiscard]] ResourceID Read(const ResourceID resourceID, i32 flags = __IGNORE_FLAGS__) {
                        assert(_frameGraph.IsValid(resourceID));
                        return _passNode.Read(resourceID, flags);
                    }

                    [[nodiscard]] ResourceID Write(const ResourceID resourceID, i32 flags = __IGNORE_FLAGS__) {
                        assert(_frameGraph.IsValid(resourceID));

                        if (_frameGraph.GetResourceEntry(resourceID).IsImported()) {
                            SetSideEffect();
                        }

                        if (_passNode.CreatesResource(resourceID)) {
                            return _passNode.Write(resourceID, flags);
                        } else {
                            // Writing to a texture produces a renamed handle.
                            // This allows us to catch errors when resources are modified in
                            // undefined order (when same resource is written by different passes).
                            // Renaming resources enforces a specific execution order of the render
                            // passes.
                            _passNode.Read(resourceID, __IGNORE_FLAGS__);
                            return _passNode.Write(_frameGraph.Clone(resourceID), flags);
                        }
                    }

                    Builder &SetSideEffect() {
                        _passNode._hasSideEffects = true;

                        return *this;
                    }

                private:
                    Builder(FrameGraph &fg, PassNode &node)
                        : _frameGraph{ fg }
                        , _passNode{ node } {
                    }

                    FrameGraph &_frameGraph;
                    PassNode &_passNode;
            };

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
            static constexpr auto __IGNORE_FLAGS__ = ~0;
            std::vector<PassNode> _passNodes;
            std::vector<ResourceNode> _resourceNodes;
            std::vector<ResourceEntry> _resourceRegistry;

            template <FrameGraphResourceBackend T>
            [[nodiscard]] inline ResourceID
            _create(const ResourceEntry::Type type, const std::string_view name, const typename T::Descriptor &desc, T &&resource) {
                const auto resourceID = _resourceRegistry.size();
                _resourceRegistry.emplace_back(ResourceEntry{ type, resourceID, desc, std::forward<T>(resource) });
                return CreateResourceNode(name, resourceID).GetResourceID();
            }

            [[nodiscard]] PassNode &CreatePassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept> &&base) {
                return _passNodes.emplace_back(PassNode{ name, _passNodes.size(), std::move(base) });
            }

            [[nodiscard]] ResourceNode &
            CreateResourceNode(const std::string_view name, ResourceID resourceId, u32 version = ResourceEntry::__INITIAL_RESOURCE_VERSION__) {
                return _resourceNodes.emplace_back(ResourceNode{ name, _resourceNodes.size(), resourceId, version });
            }

            // *******************************************************************/
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

            [[nodiscard]] ResourceEntry &GetResourceEntry(const ResourceNode &node) {
                return const_cast<ResourceEntry &>(const_cast<const FrameGraph *>(this)->GetResourceEntry(node));
            }

            [[nodiscard]] bool IsValid(ResourceID resourceID) const {
                const auto &node = GetResourceNode(resourceID);
                return node.GetVersion() == GetResourceEntry(node).GetVersion();
            }

            [[nodiscard]] ResourceID Clone(ResourceID resourceID) {
                const auto &node = GetResourceNode(resourceID);
                auto &entry = GetResourceEntry(node);
                entry._version++;

                const auto &clone = CreateResourceNode(node.GetName(), node.GetResourceID(), entry.GetVersion());

                return clone.GetResourceID();
            }
            // *******************************************************************/
    };

} // namespace Vulkyrie::Renderer
