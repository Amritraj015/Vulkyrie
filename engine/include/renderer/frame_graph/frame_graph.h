#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_traits.h"

namespace Vulkyrie::Renderer {

    using ResourceID = i32;

    class GraphNode {
        public:
            GraphNode() = delete;
            virtual ~GraphNode() = default;

            GraphNode(const GraphNode &) = delete;
            GraphNode &operator=(const GraphNode &) = delete;

            GraphNode(GraphNode &&) = default;
            GraphNode &operator=(GraphNode &&) = delete;

            [[nodiscard]] inline std::string_view GetName() const {
                return _name;
            }

            [[nodiscard]] inline u32 GetRefCount() const {
                return _refCount;
            }

            [[nodiscard]] inline size_t GetNodeIndex() const {
                return _index;
            }

        protected:
            GraphNode(const std::string_view name, size_t index)
                : _name(name)
                , _index(index)
                , _refCount(0) {
            }

        private:
            const std::string_view _name;
            const size_t _index;
            u32 _refCount;
    };

    class PassNode;
    class FrameGraphPassResources;

    class FrameGraphResource final {
        public:
            static constexpr u8 INITIAL_VERSION = 1U;

            FrameGraphResource() = delete;
            ~FrameGraphResource() = default;

            FrameGraphResource(const FrameGraphResource &) = delete;
            FrameGraphResource &operator=(const FrameGraphResource) = delete;

            FrameGraphResource(FrameGraphResource &&) = default;
            FrameGraphResource &operator=(FrameGraphResource &&) = delete;

            void Create(void *allocator);
            void Destroy(void *allocator);
            void PreRead(u32 flags, void *context);
            void PreWrite(u32 flags, void *context);

            [[nodiscard]] inline u32 GetId() const {
                return _id;
            }

            [[nodiscard]] inline u32 GetVersion() const {
                return _version;
            }

            [[nodiscard]] inline bool IsImported() const {
                return _type == Type::Imported;
            }

            [[nodiscard]] inline bool IsTransient() const {
                return _type == Type::Transient;
            }

        private:
            enum class Type : u32 { Transient, Imported };

            const Type _type;
            const u32 _id;
            u32 _version; // Incremented on each (unique) write declaration.
            // std::unique_ptr<Concept> m_concept;

            PassNode *_producer{ nullptr };
            PassNode *_last{ nullptr };
    };

    struct FrameGraphPassConcept {
        public:
            FrameGraphPassConcept() = default;
            virtual ~FrameGraphPassConcept() = default;

            FrameGraphPassConcept(const FrameGraphPassConcept &) = delete;
            FrameGraphPassConcept &operator=(const FrameGraphPassConcept &) = delete;

            FrameGraphPassConcept(FrameGraphPassConcept &&) noexcept = delete;
            FrameGraphPassConcept &operator=(FrameGraphPassConcept &&) noexcept = delete;

            virtual void operator()(FrameGraphPassResources &, void *) = 0;
    };

    template <typename Data, typename Execute> struct FrameGraphPass final : FrameGraphPassConcept {
        public:
            explicit FrameGraphPass(Execute &&exec)
                : execFunction{ std::forward<Execute>(exec) } {
            }

            void operator()(FrameGraphPassResources &resources, void *context) override {
                execFunction(data, resources, context);
            }

            Execute execFunction;
            Data data{};
    };

    /** @brief Represents a pass node in the frame graph. Each pass
     * node encapsulates the execution logic of a rendering pass,
     * along with its resource dependencies.
     */
    class PassNode : public GraphNode {
            friend class FrameGraph;

        public:
            /** @brief Constructs a PassNode with the given name, index, and execution function.
             * @param name The name of the pass node.
             * @param index The unique index of the pass node in the graph.
             * @param executeFunc The function to execute when this pass is executed. It should be invocable with a const reference to PassData.
             */
            PassNode(const std::string_view name, u32 index, std::unique_ptr<FrameGraphPassConcept> &&executeFunc)
                : GraphNode(name, index)
                , _executeFunc(std::move(executeFunc)) {
                _readResources.reserve(20);
                _writeResources.reserve(20);
                _createdResources.reserve(20);
            }

            /** @brief Checks if the pass creates the specified resource.
             * @param resource The ID of the resource to check.
             * @return `true` if the pass creates the resource; otherwise, `false`.
             */
            [[nodiscard]] inline bool CreatesResource(ResourceID resource) const {
                return std::ranges::find(_createdResources, resource) != _createdResources.cend();
            }

            /** @brief Checks if the pass reads from the specified resource.
             * @param resource The ID of the resource to check.
             * @return `true` if the pass reads from the resource; otherwise, `false`.
             */
            [[nodiscard]] inline bool ReadsResource(ResourceID resource) const {
                return std::ranges::find(_readResources, resource) != _readResources.cend();
            }

            /** @brief Checks if the pass writes to the specified resource.
             * @param resource The ID of the resource to check.
             * @return `true` if the pass writes to the resource; otherwise, `false`.
             */
            [[nodiscard]] inline bool WritesToResource(ResourceID resource) const {
                return std::ranges::find(_writeResources, resource) != _writeResources.cend();
            }

        private:
            /** @brief The function to execute when this pass is executed. It should be invocable with a const reference to PassData. */
            std::unique_ptr<FrameGraphPassConcept> _executeFunc;

            /** @brief Lists of resource IDs that this pass reads from. These lists are used for dependency tracking and scheduling. */
            std::vector<ResourceID> _readResources;

            /** @brief Lists of resource IDs that this pass writes to. These lists are used for dependency tracking and scheduling. */
            std::vector<ResourceID> _writeResources;

            /** @brief Lists of resource IDs that this pass creates. These lists are used for dependency tracking and scheduling. */
            std::vector<ResourceID> _createdResources;
    };

    class ResourceNode final : public GraphNode {
            friend class FrameGraph;

        public:
            ResourceNode(const std::string_view name, size_t index, u32 resourceId, u32 version)
                : GraphNode{ name, index }
                , _resourceId{ resourceId }
                , _version{ version } {
            }

            ResourceNode(const ResourceNode &) = delete;
            ResourceNode &operator=(const ResourceNode &) = delete;

            ResourceNode(ResourceNode &&) = default;
            ResourceNode &operator=(ResourceNode &&) = delete;

            [[nodiscard]] inline u32 GetResourceId() const {
                return _resourceId;
            }

            [[nodiscard]] inline u32 GetVersion() const {
                return _version;
            }

        private:
            // Index to virtual resource (FrameGraph::m_resourceRegistry).
            const u32 _resourceId;
            const u32 _version;

            PassNode *_producer{ nullptr };
            PassNode *_last{ nullptr };
    };

    class FrameGraphPassResources {
            friend class FrameGraph;

        public:
            FrameGraphPassResources() = delete;
            ~FrameGraphPassResources() = default;

            FrameGraphPassResources(const FrameGraphPassResources &) = delete;
            FrameGraphPassResources(FrameGraphPassResources &&) noexcept = delete;

            FrameGraphPassResources &operator=(const FrameGraphPassResources &) = delete;
            FrameGraphPassResources &operator=(FrameGraphPassResources &&) noexcept = delete;

            /**
             * @note Causes runtime-error with:
             * - Attempt to use obsolete handle (the one that has been renamed before)
             * - Incorrect resource type T
             */
            template <typename T>
                requires FrameGraphResourceBackend<T>
            [[nodiscard]] T &Get(FrameGraphResource id);

            template <typename T>
                requires FrameGraphResourceBackend<T>
            [[nodiscard]] const typename T::Desc &GetDescriptor(FrameGraphResource id) const;

        private:
            FrameGraphPassResources(FrameGraph &fg, const PassNode &node)
                : _frameGraph{ fg }
                , _passNode{ node } {
            }

        private:
            FrameGraph &_frameGraph;
            const PassNode &_passNode;
    };

    class FrameGraph {
        public:
            FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph(FrameGraph &&) = delete;

            FrameGraph &operator=(const FrameGraph &) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;

            void Compile() {
                for (auto &pass : _passNodes) {
                    pass._refCount = static_cast<int32_t>(pass._writeResources.size());

                    for (const auto [id, _] : pass._readResources) {
                        auto &consumed = _resourceNodes[id];
                        consumed.m_refCount++;
                    }

                    for (const auto [id, _] : pass._writeResources) {
                        auto &written = _resourceNodes[id];
                        written.m_producer = &pass;
                    }
                }

                // -- Culling:

                std::stack<ResourceNode *> unreferencedResources;

                for (auto &node : _resourceNodes) {
                    if (node._refCount == 0) unreferencedResources.push(&node);
                }

                while (!unreferencedResources.empty()) {
                    auto *unreferencedResource = unreferencedResources.top();
                    unreferencedResources.pop();
                    PassNode *producer{ unreferencedResource->_producer };
                    if (producer == nullptr || producer->hasSideEffect()) continue;

                    assert(producer->_refCount >= 1);

                    if (--producer->_refCount == 0) {
                        for (const auto [id, _] : producer->_readResources) {
                            auto &node = _resourceNodes[id];
                            if (--node._refCount == 0) unreferencedResources.push(&node);
                        }
                    }
                }

                // -- Calculate resources lifetime:

                for (auto &pass : _passNodes) {
                    if (pass._refCount == 0) continue;

                    for (const auto id : pass._createdResources) _getResourceEntry(id).m_producer = &pass;
                    for (const auto [id, _] : pass._writeResources) _getResourceEntry(id).m_last = &pass;
                    for (const auto [id, _] : pass._readResources) _getResourceEntry(id).m_last = &pass;
                }
            }

            void Execute(void *context) {
                for (const auto &pass : _passNodes) {
                    if (!pass.CanExecute()) continue;

                    for (const auto id : pass._createdResources) _getResourceEntry(id).create(allocator);

                    for (const auto [id, flags] : pass._readResources) {
                        if (flags != kFlagsIgnored) {
                            _getResourceEntry(id).preRead(flags, context);
                        }
                    }

                    for (const auto [id, flags] : pass._writeResources) {
                        if (flags != kFlagsIgnored) {
                            _getResourceEntry(id).preWrite(flags, context);
                        }
                    }

                    FrameGraphPassResources resources{ *this, pass };
                    std::invoke(*pass._executeFunc, resources, context);

                    for (auto &entry : _resourceRegistry) {
                        if (entry._last == &pass && entry.IsTransient()) entry.Destroy(allocator);
                    }
                }
            }

            class Builder {
                    friend class FrameGraph;

                public:
                    Builder() = delete;
                    Builder(const Builder &) = delete;
                    Builder(Builder &&) = delete;

                    Builder &operator=(const Builder &) = delete;
                    Builder &operator=(Builder &&) = delete;

                    template <FrameGraphResourceBackend T>
                    [[nodiscard]] ResourceID CreateResource(const std::string_view name, const typename T::Descriptor &descriptor);

                    [[nodiscard]] ResourceID Read(const ResourceID resource);

                    [[nodiscard]] ResourceID Write(const ResourceID resource);

                private:
                    Builder(FrameGraph &fg, GraphNode &node)
                        : _frameGraph{ fg }
                        , _passNode{ node } {
                    }

                    FrameGraph &_frameGraph;
                    GraphNode &_passNode;
            };

            [[nodiscard]] bool IsValid() const;

            template <typename PassData, typename SetupFunc, typename ExecuteFunc>
                requires std::is_invocable_v<SetupFunc, Builder &, PassData &> && std::is_invocable_v<ExecuteFunc, const PassData &> &&
                         (sizeof(ExecuteFunc) <= 1024)
            const PassData &AddPass(const std::string_view name, SetupFunc &&setupFunc, ExecuteFunc &&executeFunc) {
                auto *pass = new FrameGraphPass<PassData, ExecuteFunc>(std::forward<Execute>(executeFunc));
                auto &passNode = CreatePassNode(name, std::unique_ptr<FrameGraphPass<PassData, ExecuteFunc>>(pass));
                Builder builder{ *this, passNode };
                setupFunc(builder, pass->data);

                return passNode->Data;
            }

        private:
            std::vector<PassNode> _passNodes;
            std::vector<ResourceNode> _resourceNodes;
            std::vector<FrameGraphResource> _resourceRegistry;

            [[nodiscard]] PassNode &CreatePassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept> &&executionFunction) {
                return _passNodes.emplace_back(name, _passNodes.size(), std::move(executionFunction));
            }

            [[nodiscard]] ResourceNode &CreateResourceNode(const std::string_view name, u32 resourceId, u32 version = FrameGraphResource::INITIAL_VERSION) {
                return _resourceNodes.emplace_back(name, _resourceNodes.size(), resourceId, version);
            }

            [[nodiscard]] ResourceID Clone(ResourceID resourse);
    };

} // namespace Vulkyrie::Renderer
