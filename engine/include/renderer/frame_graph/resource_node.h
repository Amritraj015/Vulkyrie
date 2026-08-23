#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    namespace detail {
        class FrameGraphCore;
    }

    /** @brief One version of a resource as seen by the graph. Writing a resource produces a new `ResourceNode`
     * pointing at the same `ResourceEntry`, which is what lets the graph order passes that modify the same
     * underlying resource. The node's own id identifies the version; `FrameGraphCore` tracks which id is current
     * for an entry, so detecting a stale handle is a comparison against that rather than a counter.
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

        /** @brief Returns the pass that produced this version, or an invalid id for an imported resource no pass
         * has written yet. */
        [[nodiscard]] VE_INLINE FrameGraphPassID GetProducer() const {
            return mProducer;
        }

    private:
        /** @param resourceID This node's index in the graph, and its index into the graph's name array.
         * @param resourceEntryID The entry holding the resource this node is a version of.
         * @param producer The pass producing this version, or an invalid id when imported. */
        ResourceNode(FrameGraphResourceID resourceID, FrameGraphResourceEntryID resourceEntryID, FrameGraphPassID producer)
            : mResourceID(resourceID)
            , mResourceEntryID(resourceEntryID)
            , mProducer{ producer } {
        }

        /** @brief The ID (index) assigned to this resource by the frame graph. */
        FrameGraphResourceID mResourceID{};

        /** @brief The ID (index) of the associated "realized resource" assigned to this resource node by the frame graph. */
        FrameGraphResourceEntryID mResourceEntryID{};

        /** @brief Passes that consume this version. Drives the cull: a version nothing consumes cannot keep its producer alive. */
        u32 mTotalConsumers = 0;

        /** @brief The ID of the pass node that created this resource. */
        FrameGraphPassID mProducer{};
    };

    static_assert(std::is_move_assignable_v<ResourceNode>, "ResourceNode must be move-assignable so the graph can reuse its node array across frames.");

    // 16 bytes: four u32s. The compile walks scan this array linearly, so a field added here is paid for every
    // resource of every frame.
    static_assert(sizeof(ResourceNode) <= 16, "ResourceNode exceeded its 16-byte budget; see the note above before raising it.");

} // namespace Vulkyrie
