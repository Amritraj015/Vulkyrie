#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_types.h"

namespace Vulkyrie {

    namespace detail {
        class FrameGraphCore;
    }

    /** @brief A rendering pass as the graph's topology sees it: its dependency ranges and the refcount used to cull
     * it. Holds nothing typed - neither the pass data nor the execute function, which live in the backend-typed
     * facade and are reached by pass index.
     *
     * Holds no containers either. A pass's created/read/written resources live in graph-level arrays, and the node
     * stores a `(begin, count)` range into each - a pass's setup runs to completion before the next pass begins, so
     * every append for a pass is contiguous. That removes three heap allocations per pass and turns the compile and
     * execute walks into linear scans over packed arrays. */
    class PassNode final {
        friend class detail::FrameGraphCore;

    public:
        PassNode() = default;

        [[nodiscard]] VE_INLINE FrameGraphPassID GetPassID() const {
            return mPassID;
        }

        /** @brief Whether the pass survived culling: it still has live outputs, or it has side effects. */
        [[nodiscard]] VE_INLINE bool ShouldExecute() const {
            return mLiveOutputCount > 0 || mHasSideEffects;
        }

        /** @brief Whether the pass was marked as having side effects, which exempts it from culling. */
        [[nodiscard]] VE_INLINE bool HasSideEffects() const {
            return mHasSideEffects;
        }

        /** @brief Marks the pass as having side effects, exempting it from culling. */
        VE_INLINE void MarkSideEffect() {
            mHasSideEffects = true;
        }

    private:
        /** @brief Constructs a pass with no accesses yet; the ranges are filled as the builder runs.
         * @param passID This pass's index in the graph, and its index into the graph's name array.
         * @param createBegin Current end of the graph's create array, where this pass's create range starts.
         * @param readBegin Current end of the graph's read array.
         * @param writeBegin Current end of the graph's write array. */
        PassNode(FrameGraphPassID passID, u32 createBegin, u32 readBegin, u32 writeBegin)
            : mPassID{ passID }
            , mCreateBegin{ createBegin }
            , mReadBegin{ readBegin }
            , mWriteBegin{ writeBegin } {
        }

        /** @brief The ID (index) assigned by the frame graph to this pass node. */
        FrameGraphPassID mPassID{};

        /** @brief Distinct resources this pass produces that are still consumed. The pass is culled when this
         * reaches zero and it has no side effects. */
        u32 mLiveOutputCount = 0;

        /** @brief Range into the graph's create array. */
        u32 mCreateBegin = 0;
        u32 mCreateCount = 0;

        /** @brief Range into the graph's read array. */
        u32 mReadBegin = 0;
        u32 mReadCount = 0;

        /** @brief Range into the graph's write array. */
        u32 mWriteBegin = 0;
        u32 mWriteCount = 0;

        /** @brief Range into the graph's release array, filled by `Compile`. */
        u32 mReleaseBegin = 0;
        u32 mReleaseCount = 0;

        /** @brief Range into the graph's barrier array, filled by `Compile`. */
        u32 mBarrierBegin = 0;
        u32 mBarrierCount = 0;

        /** @brief Position of this pass in the compiled execution order, or `~0U` when culled. */
        u32 mExecutionIndex = std::numeric_limits<u32>::max();

        /** @brief Passes with side effects are never culled. */
        bool mHasSideEffects = false;
    };

    static_assert(std::is_move_assignable_v<PassNode>, "PassNode must be move-assignable so the graph can reuse its node array across frames.");

    // 56 bytes: fourteen u32s of ranges and counters, and a flag. The pass's payload and dispatch pointers live in
    // the typed facade rather than here, because the topology walks never read them; names live in a side array for
    // the same reason, being debug-only data that `Compile` and `Execute` never touch. Growing past this should be
    // a deliberate decision, not a side effect of adding a field.
    static_assert(sizeof(PassNode) <= 56, "PassNode exceeded its 56-byte budget; see the note above before raising it.");

} // namespace Vulkyrie
