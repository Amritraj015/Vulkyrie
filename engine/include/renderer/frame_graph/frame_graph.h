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

    // template <typename PassData, typename ExecuteFunc>
    //     requires std::is_invocable_v<ExecuteFunc, const PassData &>
    class PassNode : public GraphNode {
        public:
            /** @brief Constructs a PassNode with the given name, index, and execution function.
             * @param name The name of the pass node.
             * @param index The unique index of the pass node in the graph.
             * @param executeFunc The function to execute when this pass is executed. It should be invocable with a const reference to PassData.
             */
            PassNode(const std::string_view name, u32 index, ExecuteFuncWrapper &&executeFunc)
                : GraphNode(name, index)
                , _executeFunc(std::forward<ExecuteFuncWrapper>(executeFunc)) {
                _readResources.reserve(20);
                _writeResources.reserve(20);
                _createdResources.reserve(20);
            }

            /** @brief Executes the pass by invoking the stored execution function with the pass data. */
            void Execute() {
                _executeFunc(Data);
            }

            PassData Data;

            /** @brief Checks if the pass creates the specified resource.
             * @param resource The ID of the resource to check.
             * @return `true` if the pass creates the resource; otherwise, `false`.
             */
            [[nodiscard]] bool CreatesResource(ResourceID resource) const {
                return std::ranges::find(_createdResources, resource) != _createdResources.cend();
            }

            /** @brief Checks if the pass reads from the specified resource.
             * @param resource The ID of the resource to check.
             * @return `true` if the pass reads from the resource; otherwise, `false`.
             */
            [[nodiscard]] bool ReadsResource(ResourceID resource) const {
                return std::ranges::find(_readResources, resource) != _readResources.cend();
            }

            /** @brief Checks if the pass writes to the specified resource.
             * @param resource The ID of the resource to check.
             * @return `true` if the pass writes to the resource; otherwise, `false`.
             */
            [[nodiscard]] bool WritesToResource(ResourceID resource) const {
                return std::ranges::find(_writeResources, resource) != _writeResources.cend();
            }

        private:
            /** @brief The function to execute when this pass is executed. It should be invocable with a const reference to PassData. */
            ExecuteFuncWrapper _executeFunc;

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

            [[nodiscard]] u32 GetResourceId() const {
                return _resourceId;
            }

            [[nodiscard]] u32 GetVersion() const {
                return _version;
            }

        private:
            // Index to virtual resource (FrameGraph::m_resourceRegistry).
            const u32 _resourceId;
            const u32 _version;

            // PassNode *_producer{ nullptr };
            // PassNode *_last{ nullptr };
    };

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

            [[nodiscard]] auto GetId() const {
                return _id;
            }

            [[nodiscard]] auto GetVersion() const {
                return _version;
            }

            [[nodiscard]] auto IsImported() const {
                return _type == Type::Imported;
            }

            [[nodiscard]] auto IsTransient() const {
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

    class FrameGraph {
        public:
            FrameGraph() = default;

            FrameGraph(const FrameGraph &) = delete;
            FrameGraph(FrameGraph &&) = delete;

            FrameGraph &operator=(const FrameGraph &) = delete;
            FrameGraph &operator=(FrameGraph &&) = delete;

            void Compile();

            void Execute() {
                for (auto &passNode : _passNodes) {
                    passNode.Execute();
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
                requires std::is_invocable_v<SetupFunc, Builder &, PassData &> && std::is_invocable_v<ExecuteFunc, const PassData &>
            const PassData AddPass(const std::string_view name, SetupFunc &&setupFunc, ExecuteFunc &&executeFunc) {
                const u32 insertIndex = static_cast<u32>(_passNodes.size());

                auto passNode = CreateScope<PassNode<PassData, ExecuteFunc>>(name, insertIndex, std::forward<ExecuteFunc>(executeFunc));

                Builder builder(*this, *passNode);

                setupFunc(builder, passNode->Data);

                _passNodes.emplace_back(std::move(passNode));

                return passNode->Data;
            }

        private:
            std::vector<PassNode> _passNodes;
            std::vector<ResourceNode> _resourceNodes;
            std::vector<FrameGraphResource> _resourceRegistry;

            [[nodiscard]] PassNode &_createPassNode(const std::string_view name) {
                return _passNodes.emplace_back(name, _passNodes.size(), std::move(executionFunction));
            }

            [[nodiscard]] ResourceNode &_createResourceNode(const std::string_view name, u32 resourceId, u32 version = FrameGraphResource::INITIAL_VERSION) {
                return _resourceNodes.emplace_back(name, _resourceNodes.size(), resourceId, version);
            }

            [[nodiscard]] ResourceID _clone(ResourceID resourse);
    };

} // namespace Vulkyrie::Renderer
