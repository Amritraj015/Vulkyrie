#pragma once

#include "vlkypch.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/frame_graph/frame_graph_concepts.h"

namespace Vulkyrie {

    class FrameGraph;

    /** @brief The arena block a pass owns: its data struct followed by the captured execute callable. One block per
     * pass, reached through the type-erased function pointers on `PassNode`. */
    template <typename TPassData, FrameGraphExecuteFn<TPassData> TExecute> struct FrameGraphPassPayload final {
    public:
        template <typename TFunc>
        explicit FrameGraphPassPayload(TFunc &&execute)
            : Execute(std::forward<TFunc>(execute)) {
        }

        /** @brief Written by the setup function, read at execute time. */
        TPassData Data{};

        /** @brief The pass execution function. */
        TExecute Execute;
    };

    // TODO: This will be a cache-line miss, fix this.
    /** @brief A rendering pass: its dispatch pointers, its dependency ranges, and the refcount used to cull it.
     *
     * Holds no containers. A pass's created/read/written resources live in graph-level arrays, and the node stores
     * a `(begin, count)` range into each - a pass's setup runs to completion before the next pass begins, so every
     * append for a pass is contiguous. That removes three heap allocations per pass and turns the compile and
     * execute walks into linear scans over packed arrays.
     *
     * Dispatch mirrors `core/jobs/job.h`: an arena-allocated payload plus `void (*)(void *, ...)` trampolines,
     * with the destructor pointer left null for trivially-destructible captures. */
    class PassNode final {
        friend class FrameGraph;

    public:
        /** @brief Invokes a pass's captured callable against its payload. */
        using InvokeFn = void (*)(void *payload, FrameGraph &graph, const FrameGraphContext &context);

        /** @brief Destroys a pass's payload; null when the payload is trivially destructible. */
        using DestroyFn = void (*)(void *payload);

        PassNode() = default;

        [[nodiscard]] VE_INLINE FrameGraphPassID GetPassID() const {
            return _passID;
        }

        /** @brief Whether the pass survived culling: it still has live outputs, or it has side effects. */
        [[nodiscard]] VE_INLINE bool ShouldExecute() const {
            return _liveOutputCount > 0 || _hasSideEffects;
        }

        /** @brief Whether the pass was marked as having side effects, which exempts it from culling. */
        [[nodiscard]] VE_INLINE bool HasSideEffects() const {
            return _hasSideEffects;
        }

    private:
        /** @brief Constructs a pass with no accesses yet; the ranges are filled as the builder runs.
         * @param passID This pass's index in the graph, and its index into the graph's name array.
         * @param payload The arena block holding the pass data and execute callable.
         * @param invoke Trampoline invoking the callable.
         * @param destroy Trampoline destroying the payload, or null when trivially destructible.
         * @param createBegin Current end of the graph's create array, where this pass's create range starts.
         * @param readBegin Current end of the graph's read array.
         * @param writeBegin Current end of the graph's write array. */
        PassNode(FrameGraphPassID passID, void *payload, InvokeFn invoke, DestroyFn destroy, u32 createBegin, u32 readBegin, u32 writeBegin)
            : _payload{ payload }
            , _invoke{ invoke }
            , _destroyPayload{ destroy }
            , _passID{ passID }
            , _createBegin{ createBegin }
            , _readBegin{ readBegin }
            , _writeBegin{ writeBegin } {
        }

        /** @brief Arena block holding `{ PassData, ExecuteFunc }`. Not owned - the arena owns the storage, this
         * node owns only the destructor call. */
        void *_payload = nullptr;

        InvokeFn _invoke = nullptr;

        /** @brief Null when the payload is trivially destructible. */
        DestroyFn _destroyPayload = nullptr;

        /** @brief The ID (index) assigned by the frame graph to this pass node. */
        FrameGraphPassID _passID{};

        /** @brief Distinct resources this pass produces that are still consumed. The pass is culled when this
         * reaches zero and it has no side effects. */
        u32 _liveOutputCount = 0;

        /** @brief Range into the graph's create array. */
        u32 _createBegin = 0;
        u32 _createCount = 0;

        /** @brief Range into the graph's read array. */
        u32 _readBegin = 0;
        u32 _readCount = 0;

        /** @brief Range into the graph's write array. */
        u32 _writeBegin = 0;
        u32 _writeCount = 0;

        /** @brief Range into the graph's release array, filled by `Compile`. Retires the old scan of every resource
         * entry per pass. */
        u32 _releaseBegin = 0;
        u32 _releaseCount = 0;

        /** @brief Range into the graph's barrier array, filled by `Compile`. */
        u32 _barrierBegin = 0;
        u32 _barrierCount = 0;

        /** @brief Position of this pass in the compiled execution order, or `~0U` when culled. */
        u32 _executionIndex = std::numeric_limits<u32>::max();

        /** @brief Passes with side effects are never culled. */
        bool _hasSideEffects = false;
    };

    static_assert(std::is_move_assignable_v<PassNode>, "PassNode must be move-assignable so the graph can reuse its node array across frames.");

    // 80 bytes: three dispatch pointers, eleven u32s of ranges and counters, and a flag. Names are deliberately not
    // here - they are debug-only data that `Compile` and `Execute` never read, so a 16-byte view on the node would
    // be dragged through cache by every hot walk for nothing; they live in a side array indexed by `_passID`. A
    // single cache line is still not reachable, but the node holds no containers and costs no allocation, which is
    // what the layout is for. Growing past this should be a deliberate decision, not a side effect of adding a field.
    static_assert(sizeof(PassNode) <= 80, "PassNode exceeded its 80-byte budget; see the note above before raising it.");

} // namespace Vulkyrie
