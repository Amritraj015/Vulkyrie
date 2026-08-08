#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/types/static_string.h"
#include "memory/allocators/arena_allocator.h"
#include "renderer/frame_graph/frame_graph_concepts.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/resource_entry.h"
#include "renderer/frame_graph/resource_node.h"

namespace Vulkyrie {

    /** @brief Largest pass payload (`{ PassData, ExecuteFunc }`) the graph will place in the frame arena. Not a
     * hard storage limit - the arena would happily take more - but a guard against a pass capturing large objects
     * by value and quietly multiplying the arena's per-frame footprint. */
    inline constexpr size_t FRAME_GRAPH_PASS_PAYLOAD_LIMIT = 1024;

    /** @brief What the transient aliasing plan achieved for one compiled graph. */
    struct FrameGraphAliasingReport {
    public:
        /** @brief Sum of every planned transient's size, i.e. what the frame would cost with no aliasing. */
        u64 UnaliasedBytes = 0;

        /** @brief Bytes actually required once resources with disjoint lifetimes share storage. */
        u64 AliasedBytes = 0;

        /** @brief The largest total size of the transients live at any one point in the execution order. No packing
         * can do better than this, so it is the yardstick `AliasedBytes` is measured against: equal means the plan
         * is optimal, and the gap between them is what a better packer could still recover. Alignment padding is
         * not counted, so `AliasedBytes` may exceed it by the padding the placements needed. */
        u64 PeakLiveBytes = 0;

        /** @brief Number of transient resources included in the plan. */
        u32 ResourceCount = 0;

        /** @brief Returns the bytes aliasing saved over allocating every transient separately. */
        [[nodiscard]] VE_INLINE u64 SavedBytes() const {
            return UnaliasedBytes - AliasedBytes;
        }
    };

    /** @brief A directed acyclic graph of rendering passes and their resource dependencies.
     *
     * `AddPass` declares passes and the resources they touch; `Compile` culls everything that cannot affect the
     * frame, orders the survivors topologically, and works out resource lifetimes and barriers; `Execute` runs
     * them. `Reset` returns the graph to empty while keeping every buffer and the frame arena, so a graph rebuilt
     * each frame stops allocating entirely once it reaches steady state.
     *
     * Passes hold no containers of their own: the resources a pass creates, reads and writes live in graph-level
     * arrays that each pass indexes with a `(begin, count)` range. This relies on `AddPass` running a pass's setup
     * to completion before the next pass begins, which is asserted rather than assumed. */
    class FrameGraph final {
    public:
        /** @brief Constructs a graph sized for the expected per-frame workload.
         * @param config Reserve hints only; all three affect how many frames it takes to reach an allocation-free
         * steady state, not what the graph can hold. */
        explicit FrameGraph(const FrameGraphConfig &config = {});

        VE_DELETE_MOVE_AND_COPY(FrameGraph);

        ~FrameGraph();

        /** @brief Defines what a single pass does with resources. Handed to a pass's setup function, which uses it
         * to create resources and declare reads and writes; those declarations are the only thing the graph knows
         * about the pass's dependencies. */
        class Builder final {
            friend class FrameGraph;

        public:
            Builder() = delete;

            VE_DELETE_MOVE_AND_COPY(Builder);

            ~Builder() = default;

            /** @brief Creates a new transient resource owned by the graph, and registers this pass as its producer.
             *
             * Creating a resource implies writing it, so a pass that only creates still counts as producing one
             * output. This is what makes the common `builder.Write(builder.Create<T>(...))` spelling behave
             * identically to a bare `Create` rather than double-counting the output and defeating culling.
             *
             * @tparam T The resource type.
             * @param name A human-readable identifier; must be a string literal, or an element of a literal table
             * for a name that varies (`MIP_NAMES[level]`).
             * @param descriptor The descriptor needed to materialize the resource.
             * @param usage How this pass writes the resource; defaults to an unspecified usage, which produces no
             * barrier.
             * @returns A typed handle to the new resource. */
            template <FrameGraphResourceType T>
            [[nodiscard]] FrameGraphHandle<T> Create(StaticString name, const typename T::Descriptor &descriptor, const ResourceUsage &usage = {}) {
                const FrameGraphResourceID id = _frameGraph.createResource<T>(ResourceEntry::Lifetime::Transient, name, descriptor, T{}, _passNode.GetPassID());

                _frameGraph.registerCreate(_passNode, id);

                return FrameGraphHandle<T>{ _frameGraph.registerWrite(_passNode, id, usage) };
            }

            /** @brief Registers a read access to a resource.
             * @tparam T The resource type.
             * @param handle The resource to read.
             * @param usage How this pass reads it; the graph turns a change of usage into a barrier.
             * @returns The same handle, for chaining. */
            template <typename T> [[nodiscard]] FrameGraphHandle<T> Read(FrameGraphHandle<T> handle, const ResourceUsage &usage = {}) {
                return FrameGraphHandle<T>{ readImpl(handle.ID, usage) };
            }

            /** @brief Registers a write access to a resource. Writing a resource this pass did not create renames
             * it: the pass reads the current version and produces a new one, which is what forces passes that
             * modify the same resource into a defined order.
             * @tparam T The resource type.
             * @param handle The resource to write.
             * @param usage How this pass writes it.
             * @returns A handle to the new version; the old handle must not be used afterwards. */
            template <typename T> [[nodiscard]] FrameGraphHandle<T> Write(FrameGraphHandle<T> handle, const ResourceUsage &usage = {}) {
                return FrameGraphHandle<T>{ writeImpl(handle.ID, usage) };
            }

            /** @brief Marks the pass as having side effects, exempting it from culling. Needed for passes whose
             * result is not consumed by any other pass in the graph - presenting, read-back, anything touching
             * external state.
             * @returns This builder, for chaining. */
            Builder &MarkSideEffect() {
                _passNode._hasSideEffects = true;

                return *this;
            }

        private:
            Builder(FrameGraph &frameGraph, PassNode &passNode)
                : _frameGraph{ frameGraph }
                , _passNode{ passNode } {
            }

            /** @brief Type-erased read registration; the typed `Read` is a one-line wrapper over this so nothing
             * beyond the wrapper is instantiated per resource type. */
            [[nodiscard]] FrameGraphResourceID readImpl(FrameGraphResourceID resourceID, const ResourceUsage &usage);

            /** @brief Type-erased write registration; see `readImpl`. */
            [[nodiscard]] FrameGraphResourceID writeImpl(FrameGraphResourceID resourceID, const ResourceUsage &usage);

            FrameGraph &_frameGraph;
            PassNode &_passNode;
        };

        /** @brief Adds a pass to the graph.
         *
         * The setup function runs immediately and declares the pass's resource dependencies through the `Builder`.
         * The execute function runs during `Execute`/`Record`, and receives the pass data, the graph (so it can
         * resolve its handles to resource objects via `GetResource`), and the frame context.
         *
         * Both the pass data and the captured execute callable live in the graph's frame arena; the pass itself
         * allocates nothing.
         *
         * Note for `RecordParallel`: execute functions may run concurrently with each other and must not mutate
         * shared state without synchronization. Recording is order-independent - only submission is ordered.
         *
         * @tparam TPassData Data the setup function fills in and the execute function reads.
         * @tparam TSetup Invocable as `setup(Builder &, TPassData &)`.
         * @tparam TExecute Invocable as `execute(const TPassData &, FrameGraph &, const FrameGraphContext &)`.
         * @param name A human-readable identifier; must be a string literal, or an element of a literal table for
         * a name that varies (`CASCADE_NAMES[cascade]`).
         * @param setup The setup function.
         * @param execute The execute function.
         * @returns A reference to the pass data, valid until the next `Reset`. */
        template <typename TPassData, FrameGraphSetupFn<Builder, TPassData> TSetup, FrameGraphExecuteFn<TPassData> TExecute>
        const TPassData &AddPass(StaticString name, TSetup &&setup, TExecute &&execute) {
            using Callable = std::decay_t<TExecute>;
            using Payload = FrameGraphPassPayload<TPassData, Callable>;

            static_assert(sizeof(Payload) <= FRAME_GRAPH_PASS_PAYLOAD_LIMIT, "Frame graph pass payload exceeds FRAME_GRAPH_PASS_PAYLOAD_LIMIT (1024 bytes).");

            VASSERT(!_inSetup, "FrameGraph::AddPass is not re-entrant: a nested pass would interleave the graph-level access ranges.");
            VASSERT(!_compiled, "FrameGraph::AddPass called after Compile(); call Reset() to start a new frame.");

            auto *payload = _arena.Emplace<Payload>(std::forward<TExecute>(execute));

            PassNode::DestroyFn destroyPayload = nullptr;

            if constexpr (!std::is_trivially_destructible_v<Payload>) {
                destroyPayload = [](void *p) { std::destroy_at(static_cast<Payload *>(p)); };
            }

            PassNode &passNode = createPassNode(
                name,
                payload,
                [](void *p, FrameGraph &graph, const FrameGraphContext &context) {
                    auto *typed = static_cast<Payload *>(p);
                    typed->Execute(std::as_const(typed->Data), graph, context);
                },
                destroyPayload);

            _inSetup = true;
            Builder builder{ *this, passNode };
            setup(builder, payload->Data);
            _inSetup = false;

            // Each pass's accesses must be one contiguous run in each graph-level array. They are, because a pass's
            // setup runs to completion before the next begins - this asserts that invariant rather than assuming it.
            VASSERT(passNode._createBegin + passNode._createCount == _creates.size(), "Frame graph create range is not contiguous; was AddPass re-entered?");
            VASSERT(passNode._readBegin + passNode._readCount == _reads.size(), "Frame graph read range is not contiguous; was AddPass re-entered?");
            VASSERT(passNode._writeBegin + passNode._writeCount == _writes.size(), "Frame graph write range is not contiguous; was AddPass re-entered?");

            return payload->Data;
        }

        /** @brief Brings an externally owned resource into the graph. The graph will schedule around it but never
         * create or destroy it, and any pass that writes it is automatically treated as having side effects.
         * @tparam T The resource type.
         * @param name A human-readable identifier. Must be a string literal; see `AddPass` for names that vary.
         * @param descriptor The descriptor describing the resource.
         * @param resource The existing resource object, moved into the graph.
         * @returns A typed handle to the imported resource. */
        template <FrameGraphResourceType T> FrameGraphHandle<T> Import(StaticString name, const typename T::Descriptor &descriptor, T &&resource) {
            return FrameGraphHandle<T>{ createResource<T>(ResourceEntry::Lifetime::Imported, name, descriptor, std::forward<T>(resource), FrameGraphPassID{}) };
        }

        /** @brief Culls, orders and analyses the graph: reference counting, dead-pass removal, a topological sort
         * with cycle detection, resource lifetimes, per-pass release ranges, per-pass barrier batches, and the
         * transient aliasing plan. Call once per frame; `Reset` starts the next one. */
        void Compile();

        /** @brief Runs the compiled graph on the calling thread, interleaving resource creation, barriers, pass
         * execution and resource release in topological order. This is the path a backend without command-buffer
         * recording (OpenGL) wants; see `Record`/`Submit` for the split form.
         * @param context The frame context, forwarded unchanged to every pass and resource type. */
        void Execute(const FrameGraphContext &context);

        /** @brief Materializes every resource the frame needs and runs each surviving pass's execute function on
         * the calling thread. Pairs with `Submit`.
         * @param context The frame context, forwarded unchanged to every pass and resource type. */
        void Record(const FrameGraphContext &context);

        /** @brief Same as `Record`, but fans the pass execute functions out across the job system. Recording is
         * order-independent, so all passes may record concurrently; only submission is ordered. Execute functions
         * must not mutate shared state without synchronization.
         *
         * Anything a pass needs to allocate while recording must come from per-worker storage the backend owns and
         * reaches through `FrameGraphContext::RenderContext` - the same place its per-thread command pool lives.
         * The graph deliberately exposes no allocator here, because it does not know the worker count and cannot
         * partition scratch correctly.
         * @param context The frame context, forwarded unchanged to every pass and resource type. */
        void RecordParallel(const FrameGraphContext &context);

        /** @brief Walks the execution order emitting each pass's batched barriers, then releases every transient
         * resource. Pairs with `Record`/`RecordParallel`.
         * @param context The frame context, forwarded unchanged to every pass and resource type. */
        void Submit(const FrameGraphContext &context);

        /** @brief Returns the graph to empty while keeping every buffer's capacity and the frame arena's chunks,
         * so the next frame runs without allocating. Runs the destructors of pass payloads and resource storage.
         *
         * Any transient resource still materialized is released first, which requires a context; use the
         * `Reset(context)` overload if a frame was compiled but never executed. */
        void Reset();

        /** @brief Resets the graph, releasing any transient resource left materialized by an interrupted frame.
         * @param context The frame context used to release still-live transients. */
        void Reset(const FrameGraphContext &context);

        /** @brief Resolves a typed handle to the resource object it refers to. Available during pass execution,
         * which is what lets a pass actually bind the resources it declared.
         * @tparam T The resource type; validated against the entry's recorded type in Debug builds. */
        template <FrameGraphResourceType T> [[nodiscard]] T &GetResource(FrameGraphHandle<T> handle) {
            return getResourceEntry(handle.ID).template GetResource<T>();
        }

        /** @brief Resolves a typed handle to the resource object it refers to.
         * @tparam T The resource type. */
        template <FrameGraphResourceType T> [[nodiscard]] const T &GetResource(FrameGraphHandle<T> handle) const {
            return getResourceEntry(handle.ID).template GetResource<T>();
        }

        /** @brief Returns the descriptor a resource was created with.
         * @tparam T The resource type. */
        template <FrameGraphResourceType T> [[nodiscard]] const typename T::Descriptor &GetDescriptor(FrameGraphHandle<T> handle) const {
            return getResourceEntry(handle.ID).template GetDescriptor<T>();
        }

        /** @brief Returns the compiled execution order as pass indices, empty until `Compile` has run. */
        [[nodiscard]] VE_INLINE std::span<const u32> GetExecutionOrder() const {
            return _executionOrder;
        }

        /** @brief Returns what the transient aliasing plan achieved, valid after `Compile`. */
        [[nodiscard]] VE_INLINE const FrameGraphAliasingReport &GetAliasingReport() const {
            return _aliasingReport;
        }

        /** @brief Returns the graph's per-frame arena for inspection - how much it holds, how often it has grown.
         *
         * Const on purpose. The arena backs pass payloads and resource storage, and is not synchronized; handing
         * out a mutable reference would let a pass body allocate from it, which is a data race the moment
         * `RecordParallel` runs those bodies concurrently. Allocation stays internal to the graph, and the compiler
         * enforces it rather than a comment asking nicely. */
        [[nodiscard]] VE_INLINE const ArenaAllocator &GetFrameArena() const {
            return _arena;
        }

        /** @brief Returns the number of passes declared this frame, culled ones included. */
        [[nodiscard]] VE_INLINE size_t GetPassCount() const {
            return _passNodes.size();
        }

        /** @brief Returns the number of resource versions declared this frame. */
        [[nodiscard]] VE_INLINE size_t GetResourceVersionCount() const {
            return _resourceNodes.size();
        }

        /** @brief Returns a pass by index, for tooling and tests.
         * @param passID The pass to look up. */
        [[nodiscard]] VE_INLINE const PassNode &GetPassNode(FrameGraphPassID passID) const {
            VASSERT(passID.IsValid() && passID.Get() < _passNodes.size(), "Pass ID is out of range.");

            return _passNodes[passID.Get()];
        }

        /** @brief Returns a resource version by id, for tooling and tests.
         * @param resourceID The resource version to look up. */
        [[nodiscard]] VE_INLINE const ResourceNode &GetResourceNode(FrameGraphResourceID resourceID) const {
            VASSERT(resourceID.IsValid() && resourceID.Get() < _resourceNodes.size(), "Resource ID is out of range.");

            return _resourceNodes[resourceID.Get()];
        }

        /** @brief Returns the name a pass was declared with.
         * @param passID The pass to look up. */
        [[nodiscard]] VE_INLINE StaticString GetPassName(FrameGraphPassID passID) const {
            VASSERT(passID.IsValid() && passID.Get() < _passNames.size(), "Pass ID is out of range.");

            return _passNames[passID.Get()];
        }

        /** @brief Returns the name a resource version was declared with. Every version of a resource shares the
         * name the first one was given.
         * @param resourceID The resource version to look up. */
        [[nodiscard]] VE_INLINE StaticString GetResourceName(FrameGraphResourceID resourceID) const {
            VASSERT(resourceID.IsValid() && resourceID.Get() < _resourceNames.size(), "Resource ID is out of range.");

            return _resourceNames[resourceID.Get()];
        }

        /** @brief Renders the compiled graph as a GraphViz DOT document: passes as boxes, resource versions as
         * ellipses, culled nodes greyed out, edges labelled by access. Feeds `dot -Tpng` and the editor's frame
         * graph view.
         * @returns The DOT source. */
        [[nodiscard]] std::string ToDot() const;

    private:
        friend class Builder;

        /** @brief Creates a resource entry and its first version node.
         * @param lifetime Whether the graph owns the resource's lifetime.
         * @param name A human-readable identifier, stored by pointer.
         * @param descriptor The descriptor needed to materialize the resource.
         * @param resource The resource object, moved into an arena-allocated storage block.
         * @param producer The pass producing the resource, or an invalid id when imported.
         * @returns The id of the first version node. */
        template <FrameGraphResourceType T>
        [[nodiscard]] FrameGraphResourceID
        createResource(ResourceEntry::Lifetime lifetime, StaticString name, const typename T::Descriptor &descriptor, T &&resource, FrameGraphPassID producer) {
            using Storage = FrameGraphResourceStorage<T>;

            VASSERT(!_compiled, "FrameGraph resources cannot be declared after Compile(); call Reset() to start a new frame.");

            auto *storage = _arena.Emplace<Storage>(descriptor, std::forward<T>(resource));

            const auto entryID = FrameGraphResourceEntryID{ static_cast<u32>(_resourceEntries.size()) };
            _resourceEntries.push_back(ResourceEntry::create<T>(lifetime, entryID, storage));

            return createResourceNode(name, entryID, ResourceEntry::INITIAL_RESOURCE_VERSION, producer);
        }

        /** @brief Appends a pass node whose access ranges start at the current end of the graph-level arrays. */
        [[nodiscard]] PassNode &createPassNode(StaticString name, void *payload, PassNode::InvokeFn invoke, PassNode::DestroyFn destroy);

        /** @brief Appends a resource version node and links it to the previous version of the same entry. */
        [[nodiscard]] FrameGraphResourceID createResourceNode(StaticString name, FrameGraphResourceEntryID entryID, u32 version, FrameGraphPassID producer);

        /** @brief Records that a pass materializes a resource, adding it to the pass's create range. */
        void registerCreate(PassNode &passNode, FrameGraphResourceID resourceID);

        /** @brief Records a read, merging into an existing access if the pass already reads the resource. */
        FrameGraphResourceID registerRead(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage);

        /** @brief Records a write, merging into an existing access if the pass already writes the resource. */
        FrameGraphResourceID registerWrite(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage);

        /** @brief Produces the next version of a resource, so two passes writing it are forced into an order. */
        [[nodiscard]] FrameGraphResourceID createNextVersion(FrameGraphResourceID resourceID, FrameGraphPassID producer);

        /** @brief Checks that a handle refers to the current version of its resource; a stale handle means two
         * passes were declared writing the same resource in an undefined order. */
        [[nodiscard]] bool isCurrentVersion(FrameGraphResourceID resourceID) const;

        /** @brief Returns whether a pass creates the given resource, scanning only that pass's create range. */
        [[nodiscard]] bool passCreatesResource(const PassNode &passNode, FrameGraphResourceID resourceID) const;

        // --- Compile stages -------------------------------------------------------------------------------

        /** @brief Seeds pass output counts and resource consumer counts. */
        void computeReferenceCounts();

        /** @brief Removes passes whose outputs nothing consumes, and everything that only fed them. */
        void cullUnreferencedPasses();

        /** @brief Orders the surviving passes with Kahn's algorithm over the derived dependency edges. */
        void buildExecutionOrder();

        /** @brief Records first/last use of every resource, in execution-order positions, and fills each pass's
         * release range. */
        void computeResourceLifetimes();

        /** @brief Packs transients with disjoint lifetimes into shared storage, and records which resources each
         * one takes its bytes over from. Runs before `buildBarriers`, which needs those predecessors. */
        void buildAliasingPlan();

        /** @brief Walks the execution order turning usage changes into per-pass barrier batches, and marks the
         * first use of every aliased transient so the backend discards the previous occupant's contents. */
        void buildBarriers();

        /** @brief Materializes every resource the frame needs, in first-use order. */
        void prepareResources(const FrameGraphContext &context);

        /** @brief Notifies the resource types of one pass's declared accesses. Always runs on the calling thread,
         * even under `RecordParallel`: these are resource-state callbacks, so running them concurrently would
         * require every resource type to be thread-safe. */
        void runAccessHooks(u32 passIndex, const FrameGraphContext &context);

        /** @brief Invokes one pass's execute function. This is the only part that runs concurrently under
         * `RecordParallel`. */
        void invokePassBody(u32 passIndex, const FrameGraphContext &context);

        /** @brief Emits one pass's batched barriers, if the context provides a hook. */
        void emitBarriers(u32 passIndex, const FrameGraphContext &context) const;

        /** @brief Returns the entry backing a resource version. */
        [[nodiscard]] VE_INLINE ResourceEntry &getResourceEntry(FrameGraphResourceID resourceID) {
            VASSERT(resourceID.IsValid() && resourceID.Get() < _resourceNodes.size(), "Resource ID is out of range.");

            return _resourceEntries[_resourceNodes[resourceID.Get()]._resourceEntryID.Get()];
        }

        /** @brief Returns the entry backing a resource version. */
        [[nodiscard]] VE_INLINE const ResourceEntry &getResourceEntry(FrameGraphResourceID resourceID) const {
            VASSERT(resourceID.IsValid() && resourceID.Get() < _resourceNodes.size(), "Resource ID is out of range.");

            return _resourceEntries[_resourceNodes[resourceID.Get()]._resourceEntryID.Get()];
        }

        /** @brief Runs the destructors of every pass payload and resource storage block. The arena memory they
         * occupy is released separately by `ArenaAllocator::Reset`. */
        void runDestructors();

        /** @brief One transient resource competing for storage in the aliasing plan, and the offset it was given. */
        struct AliasCandidate {
        public:
            u32 EntryIndex = 0;
            u64 Size = 0;
            u64 Alignment = 1;
            u32 FirstUse = 0;
            u32 LastUse = 0;
            u64 Offset = 0;
        };

        /** @brief Per-frame bump allocator backing pass payloads and resource storage. */
        ArenaAllocator _arena;

        /** @brief Every pass declared this frame, in declaration order. */
        std::vector<PassNode> _passNodes;

        /** @brief Every resource version declared this frame. */
        std::vector<ResourceNode> _resourceNodes;

        /** @brief Every resource entry declared this frame. */
        std::vector<ResourceEntry> _resourceEntries;

        /** @brief For each resource version, the node holding the next version of the same entry, or an invalid
         * id. Lets `Compile` derive write-after-read edges without a per-entry search. */
        std::vector<FrameGraphResourceID> _nextVersion;

        /** @brief Resources created by each pass, indexed by the pass's create range. */
        std::vector<FrameGraphResourceID> _creates;

        /** @brief Resources read by each pass, indexed by the pass's read range. */
        std::vector<ResourceAccess> _reads;

        /** @brief Resources written by each pass, indexed by the pass's write range. */
        std::vector<ResourceAccess> _writes;

        /** @brief Resources released after each pass, indexed by the pass's release range. Filled by `Compile`. */
        std::vector<FrameGraphResourceEntryID> _releases;

        /** @brief Transitions emitted before each pass, indexed by the pass's barrier range. */
        std::vector<ResourceBarrier> _barriers;

        /** @brief Surviving passes in topological order, as indices into `_passNodes`. */
        std::vector<u32> _executionOrder;

        /** @brief Scratch: unmet predecessor count per pass, used by the topological sort. */
        std::vector<u32> _inDegrees;

        /** @brief Scratch: the sort's ready set, kept as a min-heap on pass index so the resulting order stays as
         * close to declaration order as the dependencies allow. */
        std::vector<u32> _readyPasses;

        /** @brief Scratch: dependency edges as `(predecessor, successor)` pairs. */
        std::vector<std::pair<u32, u32>> _edges;

        /** @brief Scratch: resources whose consumer count has dropped to zero, driving the cull. */
        std::vector<FrameGraphResourceID> _cullWorklist;

        /** @brief Scratch: per-entry current usage, used to derive barriers. */
        std::vector<ResourceUsage> _currentUsages;

        /** @brief Scratch: number of resources released after each execution-order position, used to size the
         * release ranges in one counting pass rather than re-scanning every entry per pass. */
        std::vector<u32> _releaseCounts;

        /** @brief For each entry, the most recent version node declared for it, used to link `_nextVersion`. */
        std::vector<FrameGraphResourceID> _latestVersionOfEntry;

        /** @brief Scratch: transients eligible for aliasing, sorted largest first by the plan. */
        std::vector<AliasCandidate> _aliasCandidates;

        /** @brief Scratch: indices into `_aliasCandidates` of everything already placed, ordered by assigned
         * offset. Keeping it ordered is what lets the placement sweep stop at the first gap it finds. */
        std::vector<u32> _aliasPlacements;

        /** @brief Scratch: per-execution-position change in live transient bytes, summed into the plan's
         * `PeakLiveBytes`. */
        std::vector<i64> _aliasLiveDelta;

        /** @brief Scratch: for each transient, the entries that previously occupied bytes it now takes over,
         * indexed by the ranges below. Kept in side arrays rather than on `ResourceEntry` because only `Compile`
         * reads them - `Execute` would otherwise drag them through cache for nothing. */
        std::vector<FrameGraphResourceEntryID> _aliasPredecessors;

        /** @brief Scratch: start of each entry's predecessor range, indexed by `FrameGraphResourceEntryID`. */
        std::vector<u32> _aliasPredecessorBegin;

        /** @brief Scratch: length of each entry's predecessor range, zeroed once `buildBarriers` has emitted that
         * entry's discard so later accesses to it are ordinary transitions. */
        std::vector<u32> _aliasPredecessorCount;

        /** @brief Pass names, indexed by `FrameGraphPassID`. Kept out of `PassNode` so the compile and execute
         * walks do not pull debug-only data through cache. */
        std::vector<StaticString> _passNames;

        /** @brief Resource version names, indexed by `FrameGraphResourceID`. */
        std::vector<StaticString> _resourceNames;

        /** @brief What the aliasing plan achieved this frame. */
        FrameGraphAliasingReport _aliasingReport;

        /** @brief Whether `Compile` has run since the last `Reset`. */
        bool _compiled = false;

        /** @brief Whether a pass's setup function is currently running, guarding against re-entrant `AddPass`. */
        bool _inSetup = false;
    };

} // namespace Vulkyrie
