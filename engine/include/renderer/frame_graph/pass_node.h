#pragma once

#include "renderer/frame_graph/resource_node.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "core/asserts.h"

namespace Vulkyrie {

    /** @brief An abstract base class that defines the interface for executing a frame graph pass.
     * It serves as a type-erased wrapper for different pass implementations, allowing them to be stored and executed in a uniform manner.
     */
    struct FrameGraphPassConcept {
    public:
        FrameGraphPassConcept() = default;
        virtual ~FrameGraphPassConcept() = default;

        FrameGraphPassConcept(const FrameGraphPassConcept &) = delete;
        FrameGraphPassConcept &operator=(const FrameGraphPassConcept &) = delete;

        FrameGraphPassConcept(FrameGraphPassConcept &&) noexcept = delete;
        FrameGraphPassConcept &operator=(FrameGraphPassConcept &&) noexcept = delete;

        virtual void operator()(void *) = 0;
    };

    /** @brief A template class that implements the FrameGraphPassConcept interface,
     * allowing for type-erased execution of frame graph passes with associated data.
     */
    template <typename Data, typename Execute> struct FrameGraphPass final : FrameGraphPassConcept {
    public:
        explicit FrameGraphPass(Execute &&exec)
            : execFunction{ std::forward<Execute>(exec) } {
        }

        void operator()(void *context) override {
            execFunction(data, context);
        }

        /** @brief The function to execute when this pass is executed.
         * It should be invocable with a reference to the pass data and a pointer to any additional context needed for execution.
         */
        Execute execFunction;

        /** @brief The data associated with the pass, which can be used to store any information needed for the execution of the pass.
         * This data is passed to the execution function when the pass is executed.
         */
        Data data{};
    };

    /** @brief Represents a pass node in the frame graph. Each pass
     * node encapsulates the execution logic of a rendering pass,
     * along with its resource dependencies.
     */
    class PassNode final {
        friend class FrameGraph;

    public:
        PassNode(const PassNode &) = delete;
        PassNode &operator=(const PassNode &) = delete;

        PassNode(PassNode &&) = default;
        PassNode &operator=(PassNode &&) = delete;

        /** @brief Represents an access to a resource by a pass, including the resource ID
         * and any associated flags that indicate the type of access (e.g., read or write).
         */
        struct ResourceAccess {
        public:
            /** @brief The ID of the resource being accessed. */
            Vulkyrie::ResourceID ResourceID;

            /** @brief Flags indicating the type of access (e.g., read or write) and any additional information about the access. */
            i32 Flags;

            /** @brief Constructs a ResourceAccess with the specified resource ID and flags.
             * @param resourceID The ID of the resource being accessed.
             * @param flags Flags indicating the type of access and any additional information about the access.
             */
            explicit ResourceAccess(::Vulkyrie::ResourceID resourceID, i32 flags)
                : ResourceID(resourceID)
                , Flags(flags) {
            }

            bool operator==(const ResourceAccess &) const = default;
        };

        /** @brief Retrieves the name of the pass, which is a human-readable identifier for the pass. */
        [[nodiscard]] inline std::string_view GetName() const {
            return _name;
        }

        /** @brief Increments the reference count of the pass, indicating that it is being used by another pass or resource. */
        [[nodiscard]] inline PassID GetPassID() const {
            return _passID;
        }

        /** @brief Checks if the pass can be executed, which is true if it has a positive reference count or if it has side effects. */
        [[nodiscard]] inline bool CanExecute() const {
            return _liveOutputCount > 0 || _hasSideEffects;
        }

        /** @brief Checks if the pass creates the specified resource.
         * @param resourceID The ID of the resource to check.
         * @returns `true` if the pass creates the resource; otherwise, `false`.
         */
        [[nodiscard]] bool CreatesResource(const ResourceID resourceID) const {
            return std::ranges::find(_creates, resourceID) != _creates.cend();
        }

        /** @brief Checks if the pass reads from the specified resource.
         * @param resourceID The ID of the resource to check.
         * @returns `true` if the pass reads from the resource; otherwise, `false`.
         */
        [[nodiscard]] bool ReadsResource(const ResourceID resourceID) const {
            return std::ranges::find_if(_reads, [resourceID](const ResourceAccess &a) { return a.ResourceID == resourceID; }) != _reads.cend();
        }

        /** @brief Checks if the pass writes to the specified resource.
         * @param resourceID The ID of the resource to check.
         * @returns `true` if the pass writes to the resource; otherwise, `false`.
         */
        [[nodiscard]] bool WritesToResource(const ResourceID resourceID) const {
            return std::ranges::find_if(_writes, [resourceID](const ResourceAccess &a) { return a.ResourceID == resourceID; }) != _writes.cend();
        }

    private:
        /** @brief Constructs a PassNode with the specified name, pass ID, and execution function.
         * @param name The human-readable identifier for the pass.
         * @param passId The unique index of the pass node in the graph.
         * @param executeFunc The function to execute when this pass is executed. It should be invocable with a const reference to PassData.
         */
        PassNode(const std::string_view name, PassID passId, std::unique_ptr<FrameGraphPassConcept> &&executeFunc)
            : _name(name)
            , _passID(passId)
            , _liveOutputCount(0)
            , _hasSideEffects(false)
            , _executeFunc(std::move(executeFunc)) {
            _creates.reserve(10);
            _reads.reserve(10);
            _writes.reserve(10);
        }

        /** @brief The name of the pass, which is a human-readable identifier for the pass. */
        const std::string_view _name;

        /** @brief The unique index of the pass node in the graph. This ID is used for tracking dependencies and execution order. */
        const PassID _passID;

        /** @brief Total number of live outputs produced by this pass.
         * This is used for reference counting and determining when a pass can be culled. */
        size_t _liveOutputCount;

        /** @brief Indicates whether the pass has side effects,
         * which means it performs operations that affect the state of the system or produce visible results.
         * Passes with side effects should not be culled even if they have a reference count of zero.
         */
        bool _hasSideEffects;

        /** @brief The function to execute when this pass is executed. It should be invocable with a const reference to PassData. */
        std::unique_ptr<FrameGraphPassConcept> _executeFunc;

        /** @brief List of resource IDs that this pass creates. These list are used for dependency tracking. */
        std::vector<ResourceID> _creates;

        /** @brief List of resource IDs that this pass reads from. These list are used for dependency tracking. */
        std::vector<ResourceAccess> _reads;

        /** @brief List of resource IDs that this pass writes to. These list are used for dependency tracking. */
        std::vector<ResourceAccess> _writes;

        /** @brief Registers a read access to the specified resource with the given flags.
         * If the resource is not already registered as a read access, it is added to the list of reads.
         * @param resourceID The ID of the resource being read.
         * @param flags Flags indicating the type of read operation. These flags can be used for optimization or to specify special handling for certain
         * types of reads.
         * @returns The ID of the resource being read, which can be used for chaining calls or for further processing. */
        [[nodiscard]] ResourceID Read(const ResourceID resourceID, i32 flags) {
            VASSERT_EXPR(!CreatesResource(resourceID) && !WritesToResource(resourceID), "Resource is already being created or written to by this pass.");

            ResourceAccess access{ resourceID, flags };

            if (std::ranges::find(_reads, access) == _reads.cend()) {
                _reads.push_back(access);
            }

            return resourceID;
        }

        /** @brief Registers a write access to the specified resource with the given flags.
         * If the resource is not already registered as a write access, it is added to the list of writes.
         * @param resourceID The ID of the resource being written to.
         * @param flags Flags indicating the type of write operation. These flags can be used for optimization or to specify special handling for certain
         * types of writes.
         * @returns The ID of the resource being written to, which can be used for chaining calls or for further processing. */
        [[nodiscard]] ResourceID Write(const ResourceID resourceID, i32 flags) {
            ResourceAccess access{ resourceID, flags };

            if (std::ranges::find(_writes, access) == _writes.cend()) {
                _writes.push_back(access);
            }

            return resourceID;
        }
    };

} // namespace Vulkyrie
