#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/jobs/parallel_for.h"
#include "core/types/static_string.h"
#include "memory/allocators/arena_allocator.h"
#include "memory/memory_scope.h"
#include "renderer/frame_graph/frame_graph_concepts.h"
#include "renderer/frame_graph/frame_graph_context.h"
#include "renderer/frame_graph/frame_graph_core.h"
#include "renderer/frame_graph/frame_graph_resources.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/frame_graph/resource_entry.h"

namespace Vulkyrie {

    /** @brief A directed acyclic graph of rendering passes and their resource dependencies.
     *
     * `AddPass` declares passes and the resources they touch; `Compile` culls everything that cannot affect the
     * frame, orders the survivors topologically, and works out resource lifetimes and barriers; `Execute` runs
     * them. `Reset` returns the graph to empty while keeping every buffer and the frame arena, so a graph rebuilt
     * each frame stops allocating entirely once it reaches steady state.
     *
     * This is the backend-typed half. Everything that works purely in ids - culling, ordering, lifetimes, the
     * aliasing plan, the barrier batches - lives in `detail::FrameGraphCore`, which is compiled once rather than
     * instantiated per backend. This layer owns the frame arena, the typed resource objects and the pass bodies,
     * and is what turns the core's plan into `Acquire`/`Release` calls and command-list work.
     *
     * @tparam B The renderer backend trait struct this graph acquires resources against. */
    template <RendererBackend B> class FrameGraph final {
    public:
        /** @brief Constructs a graph sized for the expected per-frame workload.
         * @param device The device this graph acquires resources from and sizes its transients against.
         * @param config Reserve hints only; all three affect how many frames it takes to reach an allocation-free
         * steady state, not what the graph can hold. */
        explicit FrameGraph(Vulkyrie::Device<B> &device, const FrameGraphConfig &config = {})
            : mDevice(device)
            , mCore(config)
            , mArena{ config.InitialArenaBytes, MemoryTag::Rendering } {
            VE_MEMORY_SCOPE(MemoryTag::Rendering);

            mEntries.reserve(config.ExpectedResources);
            mTypedPasses.reserve(config.ExpectedPasses);
            mMemoryRequirements.reserve(config.ExpectedResources);
        }

        VE_DELETE_MOVE_AND_COPY(FrameGraph);

        ~FrameGraph() {
            runDestructors();
        }

        /** @brief Defines what a single pass does with resources. Handed to a pass's setup function, which uses it
         * to create resources and declare reads and writes; those declarations are the only thing the graph knows
         * about the pass's dependencies. */
        class Builder final {
            friend class FrameGraph;

        public:
            Builder() = delete;

            VE_DELETE_MOVE_AND_COPY(Builder);

            ~Builder() = default;

            /** @brief Declares a new transient resource owned by the graph, and registers this pass as its
             * producer. The resource is not acquired here - `Compile` decides its lifetime first, and execution
             * acquires it from the transient pool at its first use.
             *
             * Declaring a resource implies writing it, so a pass that only creates still counts as producing one
             * output. This is what makes the common `builder.Write(builder.Create<T>(...))` spelling behave
             * identically to a bare `Create` rather than double-counting the output and defeating culling.
             *
             * @tparam T The resource type.
             * @param name A human-readable identifier; must be a string literal, or an element of a literal table
             * for a name that varies (`MIP_NAMES[level]`).
             * @param descriptor The descriptor the resource is acquired with.
             * @param usage How this pass writes the resource; defaults to an unspecified usage, which produces no
             * barrier.
             * @returns A typed handle to the new resource. */
            template <FrameGraphResourceType<B> T>
            [[nodiscard]] FrameGraphHandle<T> Create(StaticString name, const typename T::Descriptor &descriptor, const ResourceUsage &usage = {}) {
                const FrameGraphResourceID id = mFrameGraph.template declareResource<T>(true, name, descriptor, T{}, mPassNode.GetPassID());

                mFrameGraph.mCore.RegisterCreate(mPassNode, id);

                return FrameGraphHandle<T>{ mFrameGraph.mCore.RegisterWrite(mPassNode, id, usage) };
            }

            /** @brief Registers a read access to a resource.
             * @tparam T The resource type.
             * @param handle The resource to read.
             * @param usage How this pass reads it; the graph turns a change of usage into a barrier.
             * @returns The same handle, for chaining. */
            template <typename T> [[nodiscard]] FrameGraphHandle<T> Read(FrameGraphHandle<T> handle, const ResourceUsage &usage = {}) {
                return FrameGraphHandle<T>{ mFrameGraph.mCore.ReadImpl(mPassNode, handle.ID, usage) };
            }

            /** @brief Registers a write access to a resource. Writing a resource this pass did not create renames
             * it: the pass reads the current version and produces a new one, which is what forces passes that
             * modify the same resource into a defined order.
             * @tparam T The resource type.
             * @param handle The resource to write.
             * @param usage How this pass writes it.
             * @returns A handle to the new version; the old handle must not be used afterwards. */
            template <typename T> [[nodiscard]] FrameGraphHandle<T> Write(FrameGraphHandle<T> handle, const ResourceUsage &usage = {}) {
                return FrameGraphHandle<T>{ mFrameGraph.mCore.WriteImpl(mPassNode, handle.ID, usage) };
            }

            /** @brief Marks the pass as having side effects, exempting it from culling. Needed for passes whose
             * result is not consumed by any other pass in the graph - presenting, read-back, anything touching
             * external state.
             * @returns This builder, for chaining. */
            Builder &MarkSideEffect() {
                mPassNode.MarkSideEffect();

                return *this;
            }

        private:
            Builder(FrameGraph &frameGraph, PassNode &passNode)
                : mFrameGraph{ frameGraph }
                , mPassNode{ passNode } {
            }

            FrameGraph &mFrameGraph;
            PassNode &mPassNode;
        };

        /** @brief The one shape a pass body may take. A plain function pointer, never a closure. */
        template <typename TPassData> using ExecuteFn = void (*)(const TPassData &, FrameGraphPassContext<B> &);

        /** @brief Adds a pass to the graph.
         *
         * The setup function runs immediately and declares the pass's resource dependencies through the `Builder`.
         * The execute function runs during `Execute`/`Record`, and receives the pass data and the pass context -
         * the command list to record into, the device, and the resources the handles in the pass data refer to.
         *
         * The execute function **must not capture**. It has to convert to a plain function pointer, so everything a
         * pass needs must travel in `TPassData` - which is the struct that exists for exactly that. Two reasons,
         * both of which a capture silently defeats: under `RecordParallel` pass bodies run concurrently, and a
         * captured reference to shared state is a data race the graph cannot see; and a pass body outlives the
         * scope that declared it, so a captured reference can dangle by the time the frame executes.
         *
         * @tparam TPassData Data the setup function fills in and the execute function reads.
         * @tparam TSetup Invocable as `setup(Builder &, TPassData &)`. Runs immediately and is never stored, so
         * unlike the execute function it may capture freely.
         * @tparam TExecute Convertible to `void (*)(const TPassData &, FrameGraphPassContext<B> &)`.
         * @param name A human-readable identifier; must be a string literal, or an element of a literal table for
         * a name that varies (`CASCADE_NAMES[cascade]`).
         * @param setup The setup function.
         * @param execute The execute function.
         * @returns A reference to the pass data, valid until the next `Reset`. */
        template <typename TPassData, FrameGraphSetupFn<Builder, TPassData> TSetup, FrameGraphExecuteFn<TPassData, B> TExecute>
        const TPassData &AddPass(StaticString name, TSetup &&setup, TExecute &&execute) {
            VE_MEMORY_SCOPE(MemoryTag::Rendering);

            static_assert(std::is_convertible_v<std::decay_t<TExecute>, ExecuteFn<TPassData>>,
                          "A frame graph execute function must not capture: it has to convert to a plain function pointer. Put everything the pass needs in "
                          "TPassData instead. Captured state is a data race under RecordParallel, and can dangle by the time the frame executes.");

            VASSERT(!mCore.InSetup(), "FrameGraph::AddPass is not re-entrant: a nested pass would interleave the graph-level access ranges.");
            VASSERT(!mCore.IsCompiled(), "FrameGraph::AddPass called after Compile(); call Reset() to start a new frame.");

            using Payload = PassPayload<TPassData>;

            auto *payload = mArena.template Emplace<Payload>(static_cast<ExecuteFn<TPassData>>(execute));

            PassNode &passNode = mCore.CreatePassNode(name);

            TypedPass typed{};
            typed.Payload = payload;
            typed.Invoke = [](void *p, FrameGraphPassContext<B> &passContext) {
                auto *block = static_cast<Payload *>(p);
                block->Execute(std::as_const(block->Data), passContext);
            };

            // The execute function is a pointer and the payload holds nothing else that needs tearing down unless
            // the pass data itself does, which is the only case that costs a destructor.
            if constexpr (!std::is_trivially_destructible_v<TPassData>) {
                typed.Destroy = [](void *p) { std::destroy_at(static_cast<Payload *>(p)); };
            }

            mTypedPasses.push_back(typed);

            mCore.SetInSetup(true);
            Builder builder{ *this, passNode };
            setup(builder, payload->Data);
            mCore.SetInSetup(false);

            // The following function is just used for debug assertions.
#if defined(VE_DEBUG)
            mCore.AssertContiguousRanges(passNode);
#endif

            return payload->Data;
        }

        /** @brief Brings an externally owned resource into the graph. The graph will schedule around it but never
         * acquire or release it, and any pass that writes it is automatically treated as having side effects.
         * @tparam T The resource type.
         * @param name A human-readable identifier. Must be a string literal; see `AddPass` for names that vary.
         * @param descriptor The descriptor describing the resource.
         * @param resource The existing resource object, moved into the graph.
         * @returns A typed handle to the imported resource. */
        template <FrameGraphResourceType<B> T> FrameGraphHandle<T> Import(StaticString name, const typename T::Descriptor &descriptor, T &&resource) {
            return FrameGraphHandle<T>{ declareResource<T>(false, name, descriptor, std::forward<T>(resource), FrameGraphPassID{}) };
        }

        /** @brief Culls, orders and analyses the graph: reference counting, dead-pass removal, a topological sort
         * with cycle detection, resource lifetimes, per-pass release ranges, per-pass barrier batches, and the
         * transient aliasing plan. Call once per frame; `Reset` starts the next one. */
        void Compile() {
            VE_MEMORY_SCOPE(MemoryTag::Rendering);

            // If the graph has already been compiled, then there is nothing to do, return early.
            if (mCore.IsCompiled()) {
                return;
            }

            // The plan is computed on every backend, including those that cannot bind two resources to one
            // allocation: what it publishes in `GetAliasingReport` is what a real packer would save, which is worth
            // knowing even where the offsets cannot be honoured. Whether they are honoured is decided at acquire
            // time, not here.
            //
            // The requirements come from the backend rather than from the descriptor: a packer fed a CPU-side
            // guess that reads low places two resources overlapping.
            mMemoryRequirements.clear();

            for (const ResourceEntry<B> &entry : mEntries) {
                mMemoryRequirements.push_back(entry.getMemoryRequirements(mDevice));
            }

            mCore.Compile(mMemoryRequirements);
        }

        /** @brief Runs the compiled graph on the calling thread, interleaving resource acquisition, barriers, pass
         * execution and resource release in topological order. This is the path a backend without command-buffer
         * recording (OpenGL) wants; see `Record`/`Submit` for the split form.
         * @param frame The frame in flight to run against. */
        void Execute(FrameContext<B> &frame) {
            VASSERT(mCore.IsCompiled(), "FrameGraph::Execute called before Compile().");

            const FrameGraphContext<B> context{ mDevice, frame };
            const FrameGraphResources<B> resources{ mCore, mEntries };

            for (const u32 passIndex : mCore.GetExecutionOrder()) {
                acquireCreates(passIndex, context);
                emitBarriers(passIndex, context, 0);
                runAccessHooks(passIndex, context);
                invokePassBody(passIndex, resources, context, 0);
                releaseAfter(passIndex, context);
            }
        }

        /** @brief Acquires every resource the frame needs and runs each surviving pass's execute function on the
         * calling thread. Pairs with `Submit`.
         * @param frame The frame in flight to record into. */
        void Record(FrameContext<B> &frame) {
            VASSERT(mCore.IsCompiled(), "FrameGraph::Record called before Compile().");

            const FrameGraphContext<B> context{ mDevice, frame };

            prepareResources(context);

            const FrameGraphResources<B> resources{ mCore, mEntries };

            for (const u32 passIndex : mCore.GetExecutionOrder()) {
                runAccessHooks(passIndex, context);
                invokePassBody(passIndex, resources, context, 0);
            }
        }

        /** @brief Same as `Record`, but fans the pass execute functions out across the job system. Recording is
         * order-independent, so all passes may record concurrently; only submission is ordered.
         *
         * Each worker records into its own command list, taken from the frame by worker index - which is why pass
         * bodies cannot capture: everything they touch has to be either their own pass data or something reached
         * through that per-worker context.
         *
         * On a backend whose contexts are thread-affine (`B::kRecordsInParallel == false`) this is `Record`. There
         * is no parallel path to take, and pretending otherwise would hand every worker the same command list.
         * @param frame The frame in flight to record into; it must hold at least one command list per worker. */
        void RecordParallel(FrameContext<B> &frame) {
            VASSERT(mCore.IsCompiled(), "FrameGraph::RecordParallel called before Compile().");

            if constexpr (!B::kRecordsInParallel) {
                Record(frame);
            } else {
                const FrameGraphContext<B> context{ mDevice, frame };

                prepareResources(context);

                const auto passCount = static_cast<u32>(mCore.GetExecutionOrder().size());

                if (passCount == 0) {
                    return;
                }

                VASSERT(frame.GetWorkerCount() >= JobSystem::WorkerCount(),
                        "FrameContext has fewer command lists than the job system has workers; passes would share a list and race.");

                // The access hooks stay on this thread. They are resource-state bookkeeping, not command recording,
                // and running them concurrently would force every resource type to be thread-safe just to be usable
                // from the parallel path - a much wider contract than the one this split is meant to impose.
                for (const u32 passIndex : mCore.GetExecutionOrder()) {
                    runAccessHooks(passIndex, context);
                }

                const FrameGraphResources<B> resources{ mCore, mEntries };

                // Recording is order-independent; only submission is ordered. Every pass therefore records
                // concurrently and Submit replays the topological order afterwards.
                ParallelFor(passCount, 1, [this, &resources, &context](u32 index) {
                    const u32 workerIndex = JobSystem::CurrentWorkerIndex();

                    VASSERT(workerIndex != INVALID_WORKER_INDEX, "A frame graph pass body ran on a thread the job system does not own.");

                    invokePassBody(mCore.GetExecutionOrder()[index], resources, context, workerIndex);
                });
            }
        }

        /** @brief Walks the execution order emitting each pass's batched barriers, then releases every transient
         * resource. Pairs with `Record`/`RecordParallel`.
         * @param frame The frame in flight the barriers are emitted into. */
        void Submit(FrameContext<B> &frame) {
            VASSERT(mCore.IsCompiled(), "FrameGraph::Submit called before Compile().");

            const FrameGraphContext<B> context{ mDevice, frame };

            for (const u32 passIndex : mCore.GetExecutionOrder()) {
                emitBarriers(passIndex, context, 0);
            }

            for (u32 entryIndex = 0; entryIndex < mEntries.size(); ++entryIndex) {
                mEntries[entryIndex].releaseResource(isTransient(entryIndex), context);
            }
        }

        /** @brief Returns the graph to empty while keeping every buffer's capacity and the frame arena's chunks,
         * so the next frame runs without allocating. Runs the destructors of pass data and resource storage.
         *
         * Only valid when no transient is still materialized. A frame that was compiled but never executed leaves
         * some live, and releasing those needs a context - use `Reset(context)` for that case. */
        void Reset() {
            VASSERT(!anyResourceLive(),
                    "FrameGraph::Reset() cannot release transients that an interrupted frame left materialized; call Reset(context) instead.");

            resetInternal();
        }

        /** @brief Resets the graph, releasing any transient resource left materialized by an interrupted frame.
         * @param frame The frame in flight the still-live transients belong to. */
        void Reset(FrameContext<B> &frame) {
            const FrameGraphContext<B> context{ mDevice, frame };

            for (u32 entryIndex = 0; entryIndex < mEntries.size(); ++entryIndex) {
                mEntries[entryIndex].releaseResource(isTransient(entryIndex), context);
            }

            resetInternal();
        }

        /** @brief Resolves a typed handle to the resource object it refers to.
         *
         * Const, and there is deliberately no mutable counterpart: `Acquire` and `Release` are non-const members of
         * every resource type, so nothing outside the graph can drive a resource's lifetime. Pass bodies do not use
         * this at all - they get a `FrameGraphResources<B>` view instead.
         * @tparam T The resource type; validated against the entry's recorded type in Debug builds.
         * @param handle The resource to resolve. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] const T &GetResource(FrameGraphHandle<T> handle) const {
            return entry(handle.ID).template GetResource<T>();
        }

        /** @brief Returns the descriptor a resource was declared with.
         * @tparam T The resource type.
         * @param handle The resource to look up. */
        template <FrameGraphResourceType<B> T> [[nodiscard]] const typename T::Descriptor &GetDescriptor(FrameGraphHandle<T> handle) const {
            return entry(handle.ID).template GetDescriptor<T>();
        }

        /** @brief Returns the compiled execution order as pass indices, empty until `Compile` has run. */
        [[nodiscard]] VE_INLINE std::span<const u32> GetExecutionOrder() const {
            return mCore.GetExecutionOrder();
        }

        /** @brief Returns what the transient aliasing plan achieved, valid after `Compile`. */
        [[nodiscard]] VE_INLINE const FrameGraphAliasingReport &GetAliasingReport() const {
            return mCore.GetAliasingReport();
        }

        /** @brief Returns the graph's per-frame arena for inspection - how much it holds, how often it has grown.
         *
         * Const on purpose. The arena backs pass data and resource storage, and is not synchronized; handing out a
         * mutable reference would let a pass body allocate from it, which is a data race the moment
         * `RecordParallel` runs those bodies concurrently. */
        [[nodiscard]] VE_INLINE const ArenaAllocator &GetFrameArena() const {
            return mArena;
        }

        /** @brief Returns the number of passes declared this frame, culled ones included. */
        [[nodiscard]] VE_INLINE size_t GetPassCount() const {
            return mCore.GetPassCount();
        }

        /** @brief Returns the number of resource versions declared this frame. */
        [[nodiscard]] VE_INLINE size_t GetResourceVersionCount() const {
            return mCore.GetResourceVersionCount();
        }

        /** @brief Returns a pass by index, for tooling and tests.
         * @param passID The pass to look up. */
        [[nodiscard]] VE_INLINE const PassNode &GetPassNode(FrameGraphPassID passID) const {
            return mCore.GetPassNode(passID);
        }

        /** @brief Returns a resource version by id, for tooling and tests.
         * @param resourceID The resource version to look up. */
        [[nodiscard]] VE_INLINE const ResourceNode &GetResourceNode(FrameGraphResourceID resourceID) const {
            return mCore.GetResourceNode(resourceID);
        }

        /** @brief Returns the name a pass was declared with.
         * @param passID The pass to look up. */
        [[nodiscard]] VE_INLINE StaticString GetPassName(FrameGraphPassID passID) const {
            return mCore.GetPassName(passID);
        }

        /** @brief Returns the name a resource version was declared with.
         * @param resourceID The resource version to look up. */
        [[nodiscard]] VE_INLINE StaticString GetResourceName(FrameGraphResourceID resourceID) const {
            return mCore.GetResourceName(resourceID);
        }

        /** @brief Renders the compiled graph as a GraphViz DOT document: passes as boxes, resource versions as
         * ellipses, culled nodes greyed out, edges labelled by access. Feeds `dot -Tpng` and the editor's frame
         * graph view.
         * @returns The DOT source. */
        [[nodiscard]] std::string ToDot() const {
            return mCore.ToDot();
        }

    private:
        friend class Builder;

        /** @brief The arena block a pass owns: its data struct followed by its execute function pointer. */
        template <typename TPassData> struct PassPayload final {
        public:
            explicit PassPayload(ExecuteFn<TPassData> execute)
                : Execute(execute) {
            }

            /** @brief Written by the setup function, read at execute time. */
            TPassData Data{};

            /** @brief The pass body. A pointer, never a closure - see `AddPass`. */
            ExecuteFn<TPassData> Execute;
        };

        /** @brief A pass's typed half, in a side array indexed by pass id. Kept out of `PassNode` so the topology
         * walks - culling, the topological sort, lifetimes - do not drag it through cache for data they never read. */
        struct TypedPass final {
        public:
            void *Payload = nullptr;
            void (*Invoke)(void *payload, FrameGraphPassContext<B> &passContext) = nullptr;

            /** @brief Null unless the pass data itself needs a destructor. */
            void (*Destroy)(void *payload) = nullptr;
        };

        /** @brief Declares a resource entry and its first version node, and constructs the typed resource object
         * in the frame arena beside its descriptor.
         * @param isTransient Whether the graph owns the resource's lifetime.
         * @param name A human-readable identifier, stored by pointer.
         * @param descriptor The descriptor the resource is acquired with.
         * @param resource The resource object, moved into an arena-allocated storage block.
         * @param producer The pass producing the resource, or an invalid id when imported.
         * @returns The id of the first version node. */
        template <FrameGraphResourceType<B> T>
        [[nodiscard]] FrameGraphResourceID
        declareResource(bool isTransient, StaticString name, const typename T::Descriptor &descriptor, T &&resource, FrameGraphPassID producer) {
            VE_MEMORY_SCOPE(MemoryTag::Rendering);

            using Storage = FrameGraphResourceStorage<T>;

            auto *storage = mArena.template Emplace<Storage>(descriptor, std::forward<T>(resource));

            const auto declared = mCore.DeclareResource(isTransient, name, producer);

            VASSERT(declared.EntryID.Get() == mEntries.size(), "Frame graph entry arrays fell out of step with the core's entry states.");

            mEntries.push_back(ResourceEntry<B>::template create<T>(storage));

            return declared.ResourceID;
        }

        /** @brief Whether the graph owns entry `entryIndex`'s lifetime. */
        [[nodiscard]] VE_INLINE bool isTransient(u32 entryIndex) const {
            return mCore.GetEntryState(FrameGraphResourceEntryID{ entryIndex }).IsTransient;
        }

        /** @brief Acquires one resource, handing its type the interval it is live over and - on a backend that can
         * bind two resources to one allocation - the offset the plan gave it.
         *
         * The two aliasing systems meet here. `ResourceLifetime` is what `TransientPool` reuses whole resources by,
         * and works on every backend; `ResourcePlacement` is the graph's own byte packing, and is only meaningful
         * where the backend can honour an offset. A backend without memory aliasing is handed an unplaced
         * placement, so a resource type taking the placed `Acquire` falls back to taking its own storage. */
        VE_INLINE void acquireEntry(FrameGraphResourceEntryID entryID, const FrameGraphContext<B> &context) {
            const detail::EntryState &state = mCore.GetEntryState(entryID);
            const ResourcePlacement placement = B::kHasMemoryAliasing ? state.Placement() : ResourcePlacement{};

            mEntries[entryID.Get()].acquireResource(state.IsTransient, state.Lifetime(), placement, context);
        }

        /** @brief Acquires everything one pass creates. */
        void acquireCreates(u32 passIndex, const FrameGraphContext<B> &context) {
            for (const FrameGraphResourceID resourceID : mCore.GetCreates(passIndex)) {
                acquireEntry(mCore.GetEntryID(resourceID), context);
            }
        }

        /** @brief Releases everything whose last use was this pass. */
        void releaseAfter(u32 passIndex, const FrameGraphContext<B> &context) {
            for (const FrameGraphResourceEntryID entryID : mCore.GetReleases(passIndex)) {
                mEntries[entryID.Get()].releaseResource(isTransient(entryID.Get()), context);
            }
        }

        /** @brief Acquires every resource the frame needs, in first-use order. */
        void prepareResources(const FrameGraphContext<B> &context) {
            for (const u32 passIndex : mCore.GetExecutionOrder()) {
                acquireCreates(passIndex, context);
            }
        }

        /** @brief Notifies the resource types of one pass's declared accesses. Always runs on the calling thread,
         * even under `RecordParallel`: these are resource-state callbacks, so running them concurrently would
         * require every resource type to be thread-safe. */
        void runAccessHooks(u32 passIndex, const FrameGraphContext<B> &context) {
            for (const ResourceAccess &access : mCore.GetReads(passIndex)) {
                // The read a Write registers to order two passes is not an access the resource type should hear
                // about; it would arrive with an empty usage describing work no stage does.
                if (access.OrderingOnly) {
                    continue;
                }

                mEntries[mCore.GetEntryID(access.Resource).Get()].preRead(access.Usage, context);
            }

            for (const ResourceAccess &access : mCore.GetWrites(passIndex)) {
                mEntries[mCore.GetEntryID(access.Resource).Get()].preWrite(access.Usage, context);
            }
        }

        /** @brief Invokes one pass's execute function. This is the only part that runs concurrently under
         * `RecordParallel`, which is why the context it builds is per-pass rather than shared. */
        void invokePassBody(u32 passIndex, const FrameGraphResources<B> &resources, const FrameGraphContext<B> &context, u32 workerIndex) {
            FrameGraphPassContext<B> passContext{ .Device = context.Device,
                                                  .Resources = resources,
                                                  .Commands = context.Frame.AcquireCommandList(workerIndex, QueueType::Graphics),
                                                  .WorkerIndex = workerIndex };

            const TypedPass &pass = mTypedPasses[passIndex];
            pass.Invoke(pass.Payload, passContext);
        }

        /** @brief Emits one pass's batched barriers. One call per pass carrying every transition, rather than one
         * call per resource. */
        void emitBarriers(u32 passIndex, const FrameGraphContext<B> &context, u32 workerIndex) {
            const std::span<const ResourceBarrier> barriers = mCore.GetBarriers(passIndex);

            if (barriers.empty()) {
                return;
            }

            context.Frame.AcquireCommandList(workerIndex, QueueType::Graphics).EmitBarriers(barriers);
        }

        /** @brief Returns the entry backing a resource version. */
        [[nodiscard]] VE_INLINE const ResourceEntry<B> &entry(FrameGraphResourceID resourceID) const {
            return mEntries[mCore.GetEntryID(resourceID).Get()];
        }

        /** @brief Whether any transient is still materialized, which is what makes the no-context `Reset` unsafe. */
        [[nodiscard]] bool anyResourceLive() const {
            return std::ranges::any_of(mEntries, [](const ResourceEntry<B> &e) { return e.isLive(); });
        }

        /** @brief Runs the destructors of every pass payload and resource storage block. The arena memory they
         * occupy is released separately by `ArenaAllocator::Reset`. */
        void runDestructors() {
            for (TypedPass &pass : mTypedPasses) {
                if (nullptr != pass.Destroy) {
                    pass.Destroy(pass.Payload);
                }
            }

            for (ResourceEntry<B> &resourceEntry : mEntries) {
                resourceEntry.destructStorage();
            }
        }

        /** @brief The shared tail of both `Reset` overloads, after any live transient has been dealt with. */
        void resetInternal() {
            runDestructors();

            mCore.Reset();

            // clear() rather than shrinking: every buffer keeps its capacity, and the arena keeps its chunks,
            // which is what makes the next frame allocation-free.
            mEntries.clear();
            mTypedPasses.clear();
            mMemoryRequirements.clear();

            mArena.Reset();
        }

        /** @brief The device every transient is acquired from and sized against. */
        Vulkyrie::Device<B> &mDevice;

        /** @brief Topology, culling, ordering, lifetimes, aliasing and barriers - none of which need a backend. */
        detail::FrameGraphCore mCore;

        /** @brief Per-frame bump allocator backing pass payloads and resource storage. */
        ArenaAllocator mArena;

        /** @brief Every resource entry declared this frame, at the same indices as the core's entry states. */
        std::vector<ResourceEntry<B>> mEntries;

        /** @brief Every pass's data and body, indexed by pass id. */
        std::vector<TypedPass> mTypedPasses;

        /** @brief Scratch: each entry's memory requirements, gathered for `Compile`. Retained across frames so the
         * gather does not allocate in the steady state. */
        std::vector<ResourceMemoryRequirements> mMemoryRequirements;
    };

} // namespace Vulkyrie
