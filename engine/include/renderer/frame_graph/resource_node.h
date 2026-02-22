#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie::Renderer {
    class PassNode;

    /** @brief Represents a resource node in the frame graph. Each resource node encapsulates information about a resource, such as its name, reference count,
     * and the pass that creates it. Resource nodes are used for tracking dependencies and managing resource lifetimes within the frame graph. */
    class ResourceNode final {
            friend class FrameGraph;

        public:
            ResourceNode(const ResourceNode &) = delete;
            ResourceNode &operator=(const ResourceNode &) = delete;

            ResourceNode(ResourceNode &&) = default;
            ResourceNode &operator=(ResourceNode &&) = delete;

            /** @brief Retrieves the name of the resource, which is a human-readable identifier for the resource. */
            [[nodiscard]] inline std::string_view GetName() const {
                return _name;
            }

            /** @brief Retrieves the identifier associated with this resource node. */
            [[nodiscard]] inline ResourceID GetResourceID() const {
                return _resourceID;
            }

            /** @brief Retrieves the identifier of the resource entry associated with this resource node. */
            [[nodiscard]] inline ResourceEntryID GetResourceEntryID() const {
                return _resourceEntryID;
            }

        private:
            /** @brief Constructs a ResourceNode with the specified name, resource ID, resource entry ID, and version.
             * @param name The human-readable identifier for the resource.
             * @param resourceID The unique identifier for this resource node in the graph, which is used for tracking dependencies and management within the
             * frame graph.
             * @param resourceEntryID The identifier of the resource entry associated with this resource node, which is used for tracking and management within
             * the frame graph.
             * @param version The version number of the resource, which is used for tracking changes and ensuring that resources are correctly updated and
             * managed within the frame graph. */
            explicit ResourceNode(const std::string_view name, ResourceID resourceID, ResourceEntryID resourceEntryID, u32 version)
                : _name(name)
                , _resourceID(resourceID)
                , _resourceEntryID(resourceEntryID)
                , _totalConsumers(0)
                , _version{ version } {
            }

            /** @brief The name of the resource, which is a human-readable identifier for the resource. */
            const std::string_view _name;

            /** @brief The unique identifier for this resource node in the graph. */
            const ResourceID _resourceID;

            /** @brief The identifier of the resource entry associated with this resource node. */
            const ResourceEntryID _resourceEntryID;

            /** @brief The total number of passes that consume this resource.
             * This is used for reference counting and determining when a resource can be culled. */
            size_t _totalConsumers;

            /** @brief The version number of the resource. */
            u32 _version;

            /** @brief A pointer to the pass node that creates this resource.
             * This is used for tracking dependencies and determining execution order in the frame graph. */
            PassNode *_creator{ nullptr };
    };
} // namespace Vulkyrie::Renderer
