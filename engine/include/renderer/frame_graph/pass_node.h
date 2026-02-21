#pragma once

#include "renderer/frame_graph/resource_node.h"

namespace Vulkyrie::Renderer {

    using PassID = size_t;

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

    template <typename Data, typename Execute> struct FrameGraphPass final : FrameGraphPassConcept {
        public:
            explicit FrameGraphPass(Execute &&exec)
                : execFunction{ std::forward<Execute>(exec) } {
            }

            void operator()(void *context) override {
                execFunction(data, context);
            }

            Execute execFunction;
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

            PassNode(PassNode &&) = delete;
            PassNode &operator=(PassNode &&) = delete;

            /** @brief Retrieves the name of the pass, which is a human-readable identifier for the pass. */
            [[nodiscard]] inline std::string_view GetName() const {
                return _name;
            }

            /** @brief Retrieves the current reference count of the pass, which indicates how many other passes or resources depend on this pass. */
            [[nodiscard]] inline size_t GetRefCount() const {
                return _refCount;
            }

            /** @brief Increments the reference count of the pass, indicating that it is being used by another pass or resource. */
            [[nodiscard]] inline PassID GetPassID() const {
                return _passID;
            }

            /** @brief Checks if the pass has side effects, which means it performs operations that affect the state of the system or produce visible results.
             */
            [[nodiscard]] inline bool HasSideEffects() const {
                return _hasSideEffects;
            }

            /** @brief Checks if the pass can be executed, which is true if it has a positive reference count or if it has side effects. */
            [[nodiscard]] inline bool CanExecute() const {
                return _refCount > 0 || HasSideEffects();
            }

            /** @brief Checks if the pass creates the specified resource.
             * @param resourceId The ID of the resource to check.
             * @return `true` if the pass creates the resource; otherwise, `false`.
             */
            [[nodiscard]] bool CreatesResource(ResourceID resourceId) const {
                return std::ranges::find(_creates, resourceId) != _creates.cend();
            }

            /** @brief Checks if the pass reads from the specified resource.
             * @param resourceId The ID of the resource to check.
             * @return `true` if the pass reads from the resource; otherwise, `false`.
             */
            [[nodiscard]] bool ReadsResource(ResourceID resourceId) const {
                return std::ranges::find(_reads, resourceId) != _reads.cend();
            }

            /** @brief Checks if the pass writes to the specified resource.
             * @param resourceId The ID of the resource to check.
             * @return `true` if the pass writes to the resource; otherwise, `false`.
             */
            [[nodiscard]] bool WritesToResource(ResourceID resourceId) const {
                return std::ranges::find(_writes, resourceId) != _writes.cend();
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
                , _refCount(0)
                , _hasSideEffects(false)
                , _executeFunc(std::move(executeFunc)) {
                _reads.reserve(20);
                _writes.reserve(20);
                _creates.reserve(20);
            }

            /** @brief The name of the pass, which is a human-readable identifier for the pass. */
            const std::string_view _name;

            /** @brief The unique index of the pass node in the graph. This ID is used for tracking dependencies and execution order. */
            const PassID _passID;

            /** @brief The reference count of the pass, which indicates how many other passes or resources depend on this pass.
             * A pass with a reference count of zero may be considered for culling if it has no side effects.
             */
            size_t _refCount;

            /** @brief Indicates whether the pass has side effects,
             * which means it performs operations that affect the state of the system or produce visible results.
             * Passes with side effects should not be culled even if they have a reference count of zero.
             */
            bool _hasSideEffects;

            /** @brief The function to execute when this pass is executed. It should be invocable with a const reference to PassData. */
            std::unique_ptr<FrameGraphPassConcept> _executeFunc;

            /** @brief List of resource IDs that this pass reads from. These list are used for dependency tracking. */
            std::vector<ResourceID> _reads;

            /** @brief List of resource IDs that this pass writes to. These list are used for dependency tracking. */
            std::vector<ResourceID> _writes;

            /** @brief List of resource IDs that this pass creates. These list are used for dependency tracking. */
            std::vector<ResourceID> _creates;
    };

} // namespace Vulkyrie::Renderer
