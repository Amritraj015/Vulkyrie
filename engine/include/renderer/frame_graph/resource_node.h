#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    namespace detail {
        class FrameGraphCore;
    }

    /** @brief One version of a resource as seen by the graph. Writing a resource produces a new `ResourceNode`
     * pointing at the same `ResourceEntry` with a bumped version, which is what lets the graph order passes that
     * modify the same underlying resource.
     *
     * Deliberately holds no `const` members: the graph reuses its node array across frames, which requires move
     * assignment. */
    class ResourceNode final {
        friend class detail::FrameGraphCore;

    public:
        ResourceNode() = default;

        /** @brief Gets the ID (index) assigned to this resource by the frame graph. */
        [[nodiscard]] VE_INLINE FrameGraphResourceID GetResourceID() const {
            return mResourceID;
        }

        /** @brief Returns the entry this node is a version of. */
        [[nodiscard]] VE_INLINE FrameGraphResourceEntryID GetResourceEntryID() const {
            return mResourceEntryID;
        }

        /** @brief Returns the version this node represents. Version 1 is the resource as created. */
        [[nodiscard]] VE_INLINE u32 GetVersion() const {
            return mVersion;
        }

        /** @brief Returns the pass that produced this version, or an invalid id for an imported resource no pass
         * has written yet. */
        [[nodiscard]] VE_INLINE FrameGraphPassID GetProducer() const {
            return mProducer;
        }

    private:
        /** @param resourceID This node's index in the graph, and its index into the graph's name array.
         * @param resourceEntryID The entry holding the resource this node is a version of.
         * @param version The version this node represents.
         * @param producer The pass producing this version, or an invalid id when imported. */
        ResourceNode(FrameGraphResourceID resourceID, FrameGraphResourceEntryID resourceEntryID, u32 version, FrameGraphPassID producer)
            : mResourceID(resourceID)
            , mResourceEntryID(resourceEntryID)
            , mVersion{ version }
            , mProducer{ producer } {
        }

        /** @brief The ID (index) assigned to this resource by the frame graph. */
        FrameGraphResourceID mResourceID{};

        /** @brief The ID (index) of the associated "realized resource" assigned to this resource node by the frame graph. */
        FrameGraphResourceEntryID mResourceEntryID{};

        /** @brief Passes that consume this version. Drives the cull: a version nothing consumes cannot keep its producer alive. */
        u32 mTotalConsumers = 0;

        /** @brief Represents the current resource version. */
        u32 mVersion = 0;

        /** @brief The ID of the pass node that created this resource. */
        FrameGraphPassID mProducer{};
    };

    static_assert(std::is_move_assignable_v<ResourceNode>, "ResourceNode must be move-assignable so the graph can reuse its node array across frames.");

} // namespace Vulkyrie
