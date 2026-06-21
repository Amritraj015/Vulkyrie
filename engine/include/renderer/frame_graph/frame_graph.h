#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_traits.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/resource_node.h"
#include "renderer/frame_graph/resource_entry.h"
#include "core/asserts.h"

namespace Vulkyrie {
    class ResourceEntry;

    /** @brief The FrameGraph class represents a directed acyclic graph of rendering passes and their resource dependencies.
     * It provides methods for compiling the graph to determine execution order and culling unreferenced passes and resources,
     * as well as executing the passes in the correct order while managing resource lifetimes. */
    class FrameGraph final {
    public:
        FrameGraph() = default;
        ~FrameGraph() = default;

        VE_DELETE_MOVE_AND_COPY(FrameGraph);

        /** @brief Builder class provides an interface for defining the operations of a rendering pass within the frame graph. It allows users to create
         * resources, register read and write accesses to resources, and specify side effects for the pass. The Builder is used within the context of
         * defining a pass, and it interacts with the PassNode to track resource dependencies and manage the execution logic of the pass. */
        class Builder final {
            friend class FrameGraph;

        public:
            Builder() = delete;

            VE_DELETE_MOVE_AND_COPY(Builder);

            ~Builder() = default;

            /** @brief Creates a new resource in the frame graph with the specified name and descriptor, and returns its ResourceID.
             * @tparam T The type of the resource backend, which must satisfy the FrameGraphResourceBackend concept.
             * @param name A human-readable identifier for the resource, which can be used for debugging and profiling purposes.
             * @param descriptor The descriptor containing the necessary information for creating and managing the resource. The specific fields and
             * requirements of the descriptor will depend on the implementation of the resource backend and the requirements of the resource being
             * created.
             * @returns The ResourceID of the newly created resource, which can be used for referencing this resource in subsequent pass definitions. */
            template <FrameGraphResourceBackend T> [[nodiscard]] ResourceID Create(const std::string_view name, const typename T::Descriptor &descriptor) {
                const auto id = _frameGraph.Create<T>(ResourceEntry::Type::Transient, name, descriptor, T{}, _passNode.GetPassID());
                return _passNode._creates.emplace_back(id);
            }

            /** @brief Registers a read access to the specified resource with the given flags, and returns the ResourceID for chaining or further
             * processing.
             * @param resourceID The ID of the resource being read, which must be a valid ResourceID that has been created in the frame graph.
             * @param flags Flags indicating the type of read operation. These flags can be used for optimization or to specify special handling for
             * certain types of reads. The specific meaning and usage of the flags will depend on the implementation of the resource backend and the
             * requirements of the resource being read. By default, this parameter is set to IGNORED_FLAGS, which indicates that no special handling is
             * required for this read operation.
             * @returns The ResourceID of the resource being read, which can be used for chaining calls or for further processing. */
            [[nodiscard]] ResourceID Read(const ResourceID resourceID, i32 flags = IGNORED_FLAGS) {
                VASSERT_EXPR(_frameGraph.IsValid(resourceID), "Resource ID is not valid in the frame graph.");
                return _passNode.Read(resourceID, flags);
            }

            /** @brief Registers a write access to the specified resource with the given flags, and returns the ResourceID for chaining or further
             * processing. If the resource is written to by multiple passes, it will be renamed to enforce a specific execution order of the passes.
             * @param resourceID The ID of the resource being written to, which must be a valid ResourceID that has been created in the frame graph.
             * @param flags Flags indicating the type of write operation. These flags can be used for optimization or to specify special handling for
             * certain types of writes. The specific meaning and usage of the flags will depend on the implementation of the resource backend and the
             * requirements of the resource being written to. By default, this parameter is set to IGNORED_FLAGS, which indicates that no special
             * handling is required for this write operation.
             * @returns The ResourceID of the resource being written to, which can be used for chaining calls or for further processing. */
            [[nodiscard]] ResourceID Write(const ResourceID resourceID, i32 flags = IGNORED_FLAGS) {
                VASSERT_EXPR(_frameGraph.IsValid(resourceID), "Resource ID is not valid in the frame graph.");

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
                    ResourceID rID = _passNode.Read(resourceID, IGNORED_FLAGS);
                    return _passNode.Write(_frameGraph.Clone(rID, _passNode.GetPassID()), flags);
                }
            }

            /** @brief Marks the pass as having side effects, which means it performs operations that affect the state of the system or produce visible
             * results. Passes with side effects should not be culled even if they have a reference count of zero. This method can be used to ensure
             * that important passes are not accidentally culled during the compilation process, especially if they interact with external systems or
             * produce results that are not directly consumed by other passes in the graph. */
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

        /** @brief Compiles the frame graph by performing reference count calculation, resource culling, and execution order determination. */
        void Compile() {
            // REFERENCE COUNT CALCULATION
            // First, we calculate the reference counts for each pass and resource node.
            for (PassNode &passNode : _passNodes) {
                // The reference count of a pass is determined by the number of resources it writes to and creates.
                passNode._liveOutputCount = passNode._writes.size() + passNode._creates.size();

                // The reference count of a resource is determined by the number of passes that read from it.
                for (const auto [resourceId, _] : passNode._reads) {
                    _resourceNodes[resourceId]._totalConsumers++;
                }
            }

            // RESOURCE CULLING
            // Next, we identify unreferenced resources and cull any passes that are only referenced by unreferenced resources.
            std::stack<ResourceNode *> unreferencedResources;

            // We start by pushing all resources with a reference count of zero onto the stack.
            for (ResourceNode &resourceNode : _resourceNodes) {
                if (resourceNode._totalConsumers == 0) {
                    unreferencedResources.push(&resourceNode);
                }
            }

            // We then repeatedly pop unreferenced resources from the stack and check their creators.
            // If a creator pass has no side effects and its reference count drops to zero,
            // we also consider it unreferenced and push any resources it reads from onto the stack.
            while (!unreferencedResources.empty()) {
                ResourceNode *unreferencedResource = unreferencedResources.top();
                unreferencedResources.pop();

                // If the resource has no creator (imported resource), skip it.
                if (!unreferencedResource->_createdBy.has_value()) {
                    continue;
                }

                PassNode &creator = _passNodes[unreferencedResource->_createdBy.value()];

                // If the creator has side effects, we cannot cull it, so we skip to the next resource.
                if (creator._hasSideEffects) {
                    continue;
                }

                VASSERT_EXPR(creator._liveOutputCount >= 1, "Pass creator has no live outputs.");

                // We decrement the reference count of the creator pass, and if it drops to zero, we consider it unreferenced.
                if (--creator._liveOutputCount == 0) {
                    for (const auto [resourceId, _] : creator._reads) {
                        ResourceNode &consumedResource = _resourceNodes[resourceId];

                        if (--consumedResource._totalConsumers == 0) {
                            unreferencedResources.push(&consumedResource);
                        }
                    }
                }
            }

            // RESOURCE LIFETIME ANALYSIS
            // Finally, we determine the execution order of the passes based on their dependencies.
            for (PassNode &passNode : _passNodes) {
                // We only consider passes that will be executed (either have outputs or have side effects).
                if (!passNode.CanExecute()) {
                    continue;
                }

                // For resources that the pass creates, we set the creator pointer to the pass,
                for (const ResourceID resourceId : passNode._creates) {
                    GetResourceEntry(resourceId)._creator = &passNode;
                }

                // For resources that the pass writes to, we set the last used by pointer to the pass,
                // as it is the last pass that modifies the resource.
                for (const auto [resourceId, _] : passNode._writes) {
                    GetResourceEntry(resourceId)._lastUsedBy = &passNode;
                }

                // For resources that the pass reads from, we also set the last used by pointer to the pass,
                // as it is the last pass that consumes the resource before it is potentially destroyed.
                for (const auto [resourceId, _] : passNode._reads) {
                    GetResourceEntry(resourceId)._lastUsedBy = &passNode;
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
                    if (flags != IGNORED_FLAGS) {
                        GetResourceEntry(resourceId).PreRead(flags, allocator);
                    }
                }

                // Perform pre-write operations for resources that the pass writes to.
                for (const auto [resourceId, flags] : passNode._writes) {
                    if (flags != IGNORED_FLAGS) {
                        GetResourceEntry(resourceId).PreWrite(flags, allocator);
                    }
                }

                // Now we can execute the pass itself.
                (*passNode._executeFunc)(context);

                // After executing the pass, we can destroy any resources that are last used by this pass.
                // The "Destroy" method will only be executed for Transient resources,
                // as Persistent resources are expected to be managed externally and should not be automatically destroyed by the frame graph.
                //
                // TODO: This can be improved, as we are currently iterating over all resources for each pass, which can be inefficient. We can optimize
                // this by keeping track of which resources are last used by each pass during the compilation phase, so we can directly access and
                // destroy only those resources without having to iterate through the entire resource registry.
                for (auto &resource : _resourceRegistry) {
                    if (resource._lastUsedBy == &passNode) {
                        resource.Destroy(allocator);
                    }
                }
            }
        }

        /** @brief Adds a new pass to the frame graph with the specified name, setup function, and execution function, and returns a reference to the pass
         * data associated with the newly added pass.
         *
         * @tparam PassData The type of the data associated with this pass, which can be used to store any necessary information for the pass during setup
         * and execution.
         *
         * @tparam SetupFunc The type of the setup function, which must be invocable with a Builder reference and a PassData reference. The setup function
         * is responsible for defining the resources that the pass creates, reads from, and writes to, as well as any side effects it may have. This
         * function is called during the pass addition process to configure the pass's dependencies and behavior within the frame graph.
         *
         * @tparam ExecuteFunc The type of the execution function, which must be invocable with a const reference to PassData and a void pointer for
         * context. The execution function contains the actual logic that will be executed when this pass is executed as part of the frame graph. This
         * function is called during the execution phase of the frame graph, and it should perform the necessary operations for this pass based on the data
         * provided in PassData and any additional context passed in as a void pointer. The execution function should be designed to work with the resources
         * that have been defined in the setup function, and it should ensure that it correctly handles any dependencies and side effects associated with
         * this pass. Additionally, the size of the execution function must be less than or equal to 1024 bytes, which is a constraint to ensure efficient
         * storage and invocation of the execution logic within the frame graph.
         *
         * @param name A human-readable identifier for the pass, which can be used for debugging and profiling purposes.
         * @param setupFunc The setup function that defines the resources and behavior of the pass.
         * @param executeFunc The execution function that contains the logic to be executed for this pass during the execution phase of the frame graph.
         *
         * @returns A reference to the PassData associated with the newly added pass.
         * */
        template <typename PassData, typename SetupFunc, typename ExecuteFunc>
            requires std::is_invocable_v<SetupFunc, Builder &, PassData &> && std::is_invocable_v<ExecuteFunc, const PassData &, void *> &&
                     (sizeof(ExecuteFunc) <= 1024)
        const PassData &AddPass(const std::string_view name, SetupFunc &&setupFunc, ExecuteFunc &&executeFunc) {
            auto pass = std::make_unique<FrameGraphPass<PassData, ExecuteFunc>>(std::forward<ExecuteFunc>(executeFunc));
            auto &passDataRef = pass->data;
            auto &passNode = CreatePassNode(name, std::move(pass));

            Builder builder{ *this, passNode };
            setupFunc(builder, passDataRef);

            return passDataRef;
        }

        /** @brief Imports an externally managed resource into the frame graph with the specified name, descriptor, and resource instance, and returns its
         * ResourceID.
         * @tparam T The type of the resource backend, which must satisfy the FrameGraphResourceBackend concept.
         * @param name A human-readable identifier for the resource, which can be used for debugging and profiling purposes.
         * @param desc The descriptor containing the necessary information for creating and managing the resource. The specific fields and requirements of
         * the descriptor will depend on the implementation of the resource backend and the requirements of the resource being imported.
         * @param resource The actual resource instance that is being imported into the frame graph. This should be an instance of the type that satisfies
         * the FrameGraphResourceBackend concept, and it should be initialized with any necessary information for managing the resource. Imported resources
         * are expected to be managed externally and will not be automatically destroyed by the frame graph. */
        template <FrameGraphResourceBackend T> ResourceID Import(const std::string_view name, const typename T::Descriptor &descriptor, T &&resource) {
            return Create<T>(ResourceEntry::Type::Imported, name, descriptor, std::forward<T>(resource), std::nullopt);
        }

    private:
        /** @brief A constant representing flags that indicate that no special handling is required for a read or write operation. This can be used as a
         * default value for the flags parameter in the Read and Write methods, indicating that the operation should be treated as a standard read or write
         * without any special optimizations or handling. The specific meaning and usage of this constant will depend on the implementation of the resource
         * backend and the requirements of the resources being read or written to. */
        static constexpr auto IGNORED_FLAGS = ~0;

        /** @brief The pass nodes vector holds all the pass nodes in the frame graph. Each pass node represents a specific rendering pass that will be
         * executed as part of the frame graph. The pass nodes are used for tracking dependencies and determining execution order during the compilation
         * process. Each pass node contains information about the pass it represents, such as its name, unique identifier, reference count, side effect
         * flag, execution function, and lists of resources it creates, reads from, and writes to. This information is crucial for determining execution
         * order, culling unreferenced passes, and ensuring that passes are executed correctly based on their dependencies and side effects. */
        std::vector<PassNode> _passNodes;

        /** @brief The resource nodes vector holds all the resource nodes in the frame graph. Each resource node represents a specific instance of a
         * resource that is used by the passes in the graph. The resource nodes are used for tracking dependencies and managing resource lifetimes during
         * pass execution. Each resource node contains information about the resource it represents, such as its name, unique identifier, associated
         * resource entry ID, reference count, version number, and pointers to the creator pass and any passes that consume it. This information is crucial
         * for determining execution order, culling unreferenced resources, and ensuring that resources are correctly created and destroyed during pass
         * execution. */
        std::vector<ResourceNode> _resourceNodes;

        /** @brief The resource registry is a vector that holds all the resource entries in the frame graph. Each resource entry contains information about
         * a specific resource, such as its type (Transient or Imported), version number, and associated resource entry ID. The resource registry is used
         * for tracking and managing resources within the frame graph, allowing us to efficiently create, update, and destroy resources as needed during
         * pass execution. */
        std::vector<ResourceEntry> _resourceRegistry;

        /** @brief Creates a new resource entry in the frame graph with the specified type, name, descriptor, and resource, and returns its ResourceID.
         * @tparam T The type of the resource backend, which must satisfy the FrameGraphResourceBackend concept.
         * @param type The type of the resource (Transient or Imported), which indicates how the resource should be managed within the frame graph.
         * Transient resources are created and destroyed automatically by the frame graph based on their usage, while Imported resources are expected to be
         * managed externally and should not be automatically destroyed by the frame graph.
         * @param name A human-readable identifier for the resource, which can be used for debugging and profiling purposes.
         * @param desc The descriptor containing the necessary information for creating and managing the resource. The specific fields and requirements of
         * the descriptor will depend on the implementation of the resource backend and the requirements of the resource being created.
         * @param resource The actual resource associated with the backend. This should be an instance of the type that satisfies the
         * FrameGraphResourceBackend concept, and it should be initialized with any necessary information for creating and managing the resource.
         * @param creatorID Optional PassID of the pass that creates this resource. This is set at creation time for proper dependency tracking. */
        template <FrameGraphResourceBackend T>
        [[nodiscard]] ResourceID Create(const ResourceEntry::Type type,
                                        const std::string_view name,
                                        const typename T::Descriptor &desc,
                                        T &&resource,
                                        std::optional<PassID> creatorID = std::nullopt) {
            const ResourceEntryID resourceEntryID = _resourceRegistry.size();
            _resourceRegistry.push_back(ResourceEntry{ type, resourceEntryID, desc, std::forward<T>(resource) });

            return CreateResourceNode(name, resourceEntryID, ResourceEntry::INITIAL_RESOURCE_VERSION, creatorID).GetResourceID();
        }

        /** @brief Creates a new pass node in the frame graph with the specified name and execution function,
         * and returns a reference to the newly created pass node.
         * @param name A human-readable identifier for the pass, which can be used for debugging and profiling purposes.
         * @param base A unique pointer to a FrameGraphPassConcept that encapsulates the execution function for this pass. The execution function should be
         * invocable with a const reference to PassData, which is the data associated with this pass. The specific implementation of the execution function
         * will depend on the requirements of the pass being defined and the data it needs to operate on. */
        [[nodiscard]] PassNode &CreatePassNode(const std::string_view name, std::unique_ptr<FrameGraphPassConcept> &&base) {
            return _passNodes.emplace_back(PassNode{ name, _passNodes.size(), std::move(base) });
        }

        /** @brief Creates a new resource node in the frame graph with the specified name and resource entry ID, and returns a reference to the newly
         * created resource node. The resource node is associated with the given resource entry ID, which is used for tracking and management within the
         * frame graph. The version number of the resource node is initialized to the provided version parameter, which can be used for tracking changes and
         * ensuring that resources are correctly updated and managed within the frame graph. By default, the version number is set to
         * ResourceEntry::INITIAL_RESOURCE_VERSION, which indicates that this is the initial version of the resource.
         * @param creatorID Optional PassID of the pass that creates this resource. This is set at creation time rather than during compilation. */
        [[nodiscard]] ResourceNode &CreateResourceNode(const std::string_view name,
                                                       ResourceEntryID resourceEntryId,
                                                       u32 version = ResourceEntry::INITIAL_RESOURCE_VERSION,
                                                       std::optional<PassID> creatorID = std::nullopt) {
            const ResourceID resourceID = _resourceNodes.size();
            return _resourceNodes.emplace_back(ResourceNode{ name, resourceID, resourceEntryId, version, creatorID });
        }

        /** @brief Retrieves a non-const reference to the ResourceEntry associated with the specified ResourceID.
         * @param id The ResourceID of the resource entry to retrieve, which must be a valid ResourceID that has been created in the frame graph.
         * @returns A non-const reference to the ResourceEntry associated with the specified ResourceID.
         * */
        [[nodiscard]] ResourceEntry &GetResourceEntry(ResourceID id) {
            return const_cast<ResourceEntry &>(const_cast<const FrameGraph *>(this)->GetResourceEntry(id));
        }

        /** @brief Retrieves a const reference to the ResourceEntry associated with the specified ResourceNode.
         * @param node The ResourceNode for which to retrieve the associated ResourceEntry. The ResourceNode must have a valid resource entry ID that
         * corresponds to an entry in the resource registry.
         * @returns A const reference to the ResourceEntry associated with the specified ResourceNode.
         * */
        [[nodiscard]] const ResourceEntry &GetResourceEntry(const ResourceNode &node) const {
            VASSERT_EXPR(node._resourceEntryID < _resourceRegistry.size(), "Resource entry ID is out of range.");
            return _resourceRegistry[node._resourceEntryID];
        }

        /** @brief Retrieves a const reference to the ResourceEntry associated with the specified ResourceID.
         * @param resourceID The ResourceID of the resource entry to retrieve, which must be a valid ResourceID that has been created in the frame graph.
         * @returns A const reference to the ResourceEntry associated with the specified ResourceID.
         * */
        [[nodiscard]] const ResourceEntry &GetResourceEntry(ResourceID resourceID) const {
            return GetResourceEntry(GetResourceNode(resourceID));
        }

        /** @brief Retrieves a const reference to the ResourceNode associated with the specified ResourceID.
         * @param resourceID The ResourceID of the resource node to retrieve, which must be a valid ResourceID that has been created in the frame graph.
         * @returns A const reference to the ResourceNode associated with the specified ResourceID.
         * */
        [[nodiscard]] const ResourceNode &GetResourceNode(ResourceID resourceID) const {
            VASSERT_EXPR(resourceID < _resourceNodes.size(), "Resource ID is out of range.");
            return _resourceNodes[resourceID];
        }

        /** @brief Retrieves a non-const reference to the ResourceEntry associated with the specified ResourceNode.
         * @param node The ResourceNode for which to retrieve the associated ResourceEntry.
         * The ResourceNode must have a valid resource entry ID that corresponds to an entry in the resource registry.
         * @returns A non-const reference to the ResourceEntry associated with the specified ResourceNode.
         * */
        [[nodiscard]] ResourceEntry &GetResourceEntry(const ResourceNode &node) {
            return const_cast<ResourceEntry &>(const_cast<const FrameGraph *>(this)->GetResourceEntry(node));
        }

        /** @brief Checks if the specified resource ID is valid by comparing the version number of the resource node with the version number of the
         * corresponding resource entry.
         * This is used to ensure that resources are correctly tracked and managed within the frame graph, and to catch errors
         * when resources are modified in undefined order. A resource ID is considered valid if the version number of the resource node matches the version
         * number of the corresponding resource entry, which indicates that the resource has not been modified since it was last read or written to. If a
         * resource ID is found to be invalid, it may indicate that there is an error in the pass definitions or in the way resources are being accessed,
         * and it can help catch issues early in the development process.
         *
         * @param resourceID The ResourceID of the resource to check for validity, which must be a valid ResourceID that has been created in the frame
         * graph.
         * @returns `true` if the specified resource ID is valid; otherwise, `false
         * */
        [[nodiscard]] bool IsValid(ResourceID resourceID) const {
            const ResourceNode &resourceNode = GetResourceNode(resourceID);
            const ResourceEntry &resourceEntry = GetResourceEntry(resourceNode);

            return resourceNode._version == resourceEntry._version;
        }

        /** @brief Clones the specified resource by creating a new resource node with the same name and resource entry ID,
         * but with an incremented version number.
         * This is used to enforce a specific execution order of passes that write to the same resource, as it allows us to catch errors when
         * resources are modified in undefined order. By cloning a resource, we create a new version of it that can be written to by a different pass
         * without affecting the original version that may still be read by other passes. The new resource node will have the same name and resource entry
         * ID as the original, but its version number will be incremented to indicate that it is a new version of the resource.
         *
         * @param resourceID The ResourceID of the resource to clone, which must be a valid ResourceID that has been created in the frame graph.
         * @param creatorID Optional PassID of the pass that creates this cloned resource. This is set at creation time for proper dependency tracking.
         * @returns The ResourceID of the newly cloned resource, which can be used for referencing the new version of the resource in subsequent pass
         * definitions. The cloned resource will have the same name and resource entry ID as the original, but with an incremented version number to
         * indicate that it is a new version of the resource.
         * */
        [[nodiscard]] ResourceID Clone(ResourceID resourceID, std::optional<PassID> creatorID) {
            const ResourceNode &node = GetResourceNode(resourceID);
            ResourceEntry &entry = GetResourceEntry(node);
            entry._version++;

            const ResourceNode &clone = CreateResourceNode(node._name, node._resourceEntryID, entry._version, creatorID);

            return clone.GetResourceID();
        }
    };

} // namespace Vulkyrie
