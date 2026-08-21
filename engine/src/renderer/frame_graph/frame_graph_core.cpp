#include "renderer/frame_graph/frame_graph_core.h"

#include "core/logger.h"
#include "memory/memory_scope.h"

#include <sstream>

namespace Vulkyrie::detail {

    namespace {

        /** @brief Sentinel for "this pass/resource has no position in the execution order". */
        constexpr u32 UNORDERED = std::numeric_limits<u32>::max();

        /** @brief Version every resource starts at; must match `EntryState::Version`'s default. */
        constexpr u32 INITIAL_RESOURCE_VERSION = 1U;

        /** @brief Folds a second access to the same resource within one pass into the first. Stage and access masks
         * union; layout and queue take the last explicitly specified value, since a pass cannot have a resource in
         * two layouts at once and the later declaration is the more specific one. */
        void MergeUsage(ResourceUsage &target, const ResourceUsage &addition) {
            target.Stages |= addition.Stages;
            target.Access |= addition.Access;

            if (addition.Layout != 0) {
                target.Layout = addition.Layout;
            }

            if (addition.QueueType != 0) {
                target.QueueType = addition.QueueType;
            }
        }

        /** @brief Rounds an offset up to the next multiple of a power-of-two alignment. */
        [[nodiscard]] u64 AlignUp(u64 value, u64 alignment) {
            return (value + alignment - 1) & ~(alignment - 1);
        }

    } // namespace

    // ===========================================================================================
    // Construction and lifetime
    // ===========================================================================================

    FrameGraphCore::FrameGraphCore(const FrameGraphConfig &config) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        // Reserve everything up front so a graph rebuilt every frame stops allocating after frame one. The
        // multipliers are rough shapes of a real graph: most resources are read by more than one pass, and most
        // passes write fewer resources than they read.
        mPassNodes.reserve(config.ExpectedPasses);
        mPassNames.reserve(config.ExpectedPasses);
        mResourceNodes.reserve(config.ExpectedResources);
        mResourceNames.reserve(config.ExpectedResources);
        mNextVersion.reserve(config.ExpectedResources);
        mEntryStates.reserve(config.ExpectedResources);
        mLatestVersionOfEntry.reserve(config.ExpectedResources);
        mCreates.reserve(config.ExpectedResources);
        mReads.reserve(static_cast<size_t>(config.ExpectedResources) * 2);
        mWrites.reserve(config.ExpectedResources);
        mReleases.reserve(config.ExpectedResources);
        mBarriers.reserve(static_cast<size_t>(config.ExpectedResources) * 2);
        mExecutionOrder.reserve(config.ExpectedPasses);
        mInDegrees.reserve(config.ExpectedPasses);
        mReadyPasses.reserve(config.ExpectedPasses);
        mEdges.reserve(static_cast<size_t>(config.ExpectedPasses) * 4);
        mCullWorklist.reserve(config.ExpectedResources);
        mCurrentUsages.reserve(config.ExpectedResources);
        mReleaseCounts.reserve(config.ExpectedPasses);
        mAliasCandidates.reserve(config.ExpectedResources);
        mAliasPlacements.reserve(config.ExpectedResources);
        mAliasLiveDelta.reserve(config.ExpectedPasses + 1);
        mAliasPredecessors.reserve(config.ExpectedResources);
        mAliasPredecessorBegin.reserve(config.ExpectedResources);
        mAliasPredecessorCount.reserve(config.ExpectedResources);
    }

    void FrameGraphCore::Reset() {
        // clear() rather than shrinking: every buffer keeps its capacity, which is what makes the next frame
        // allocation-free.
        mPassNodes.clear();
        mPassNames.clear();
        mResourceNodes.clear();
        mResourceNames.clear();
        mNextVersion.clear();
        mEntryStates.clear();
        mLatestVersionOfEntry.clear();
        mCreates.clear();
        mReads.clear();
        mWrites.clear();
        mReleases.clear();
        mBarriers.clear();
        mExecutionOrder.clear();
        mInDegrees.clear();
        mReadyPasses.clear();
        mEdges.clear();
        mCullWorklist.clear();
        mCurrentUsages.clear();
        mReleaseCounts.clear();
        mAliasCandidates.clear();
        mAliasPlacements.clear();
        mAliasLiveDelta.clear();
        mAliasPredecessors.clear();
        mAliasPredecessorBegin.clear();
        mAliasPredecessorCount.clear();

        mAliasingReport = FrameGraphAliasingReport{};
        mCompiled = false;
        mInSetup = false;
    }

    // ===========================================================================================
    // Graph construction
    // ===========================================================================================

    PassNode &FrameGraphCore::CreatePassNode(StaticString name) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        const auto passID = FrameGraphPassID{ static_cast<u32>(mPassNodes.size()) };

        mPassNames.push_back(name);

        return mPassNodes.emplace_back(
            PassNode{ passID, static_cast<u32>(mCreates.size()), static_cast<u32>(mReads.size()), static_cast<u32>(mWrites.size()) });
    }

    FrameGraphCore::DeclaredResource FrameGraphCore::DeclareResource(bool isTransient, StaticString name, FrameGraphPassID producer) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        VASSERT(!mCompiled, "FrameGraph resources cannot be declared after Compile(); call Reset() to start a new frame.");

        const auto entryID = FrameGraphResourceEntryID{ static_cast<u32>(mEntryStates.size()) };

        mEntryStates.push_back(EntryState{ .IsTransient = isTransient });

        return DeclaredResource{ .EntryID = entryID, .ResourceID = createResourceNode(name, entryID, INITIAL_RESOURCE_VERSION, producer) };
    }

    void FrameGraphCore::AssertContiguousRanges([[maybe_unused]] const PassNode &passNode) const {
        // Each pass's accesses must be one contiguous run in each graph-level array. They are, because a pass's
        // setup runs to completion before the next begins - this asserts that invariant rather than assuming it.
        VASSERT(passNode.mCreateBegin + passNode.mCreateCount == mCreates.size(), "Frame graph create range is not contiguous; was AddPass re-entered?");
        VASSERT(passNode.mReadBegin + passNode.mReadCount == mReads.size(), "Frame graph read range is not contiguous; was AddPass re-entered?");
        VASSERT(passNode.mWriteBegin + passNode.mWriteCount == mWrites.size(), "Frame graph write range is not contiguous; was AddPass re-entered?");
    }

    FrameGraphResourceID FrameGraphCore::createResourceNode(StaticString name, FrameGraphResourceEntryID entryID, u32 version, FrameGraphPassID producer) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        const auto resourceID = FrameGraphResourceID{ static_cast<u32>(mResourceNodes.size()) };

        mResourceNames.push_back(name);
        mResourceNodes.push_back(ResourceNode{ resourceID, entryID, version, producer });
        mNextVersion.push_back(FrameGraphResourceID{});

        if (mLatestVersionOfEntry.size() <= entryID.Get()) {
            mLatestVersionOfEntry.resize(entryID.Get() + 1, FrameGraphResourceID{});
        }

        // Link the previous version to this one so Compile can derive write-after-read edges in O(1) per read.
        const FrameGraphResourceID previous = mLatestVersionOfEntry[entryID.Get()];

        if (previous.IsValid()) {
            mNextVersion[previous.Get()] = resourceID;
        }

        mLatestVersionOfEntry[entryID.Get()] = resourceID;

        return resourceID;
    }

    void FrameGraphCore::RegisterCreate(PassNode &passNode, FrameGraphResourceID resourceID) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        mCreates.push_back(resourceID);
        ++passNode.mCreateCount;
    }

    FrameGraphResourceID FrameGraphCore::registerRead(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        VASSERT(!passCreatesResource(passNode, resourceID), "A pass cannot read a resource it creates in the same pass.");

        for (u32 i = 0; i < passNode.mReadCount; ++i) {
            ResourceAccess &access = mReads[passNode.mReadBegin + i];

            if (access.Resource == resourceID) {
                MergeUsage(access.Usage, usage);
                return resourceID;
            }
        }

        mReads.push_back(ResourceAccess{ .Resource = resourceID, .Usage = usage });
        ++passNode.mReadCount;

        return resourceID;
    }

    FrameGraphResourceID FrameGraphCore::RegisterWrite(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        for (u32 i = 0; i < passNode.mWriteCount; ++i) {
            ResourceAccess &access = mWrites[passNode.mWriteBegin + i];

            if (access.Resource == resourceID) {
                MergeUsage(access.Usage, usage);
                return resourceID;
            }
        }

        mWrites.push_back(ResourceAccess{ .Resource = resourceID, .Usage = usage });
        ++passNode.mWriteCount;

        return resourceID;
    }

    FrameGraphResourceID FrameGraphCore::createNextVersion(FrameGraphResourceID resourceID, FrameGraphPassID producer) {
        const StaticString name = mResourceNames[resourceID.Get()];
        const FrameGraphResourceEntryID entryID = mResourceNodes[resourceID.Get()].mResourceEntryID;

        EntryState &state = mEntryStates[entryID.Get()];
        ++state.Version;

        // Every version shares the original's name.
        return createResourceNode(name, entryID, state.Version, producer);
    }

    bool FrameGraphCore::isCurrentVersion(FrameGraphResourceID resourceID) const {
        if (!resourceID.IsValid() || resourceID.Get() >= mResourceNodes.size()) {
            return false;
        }

        const ResourceNode &node = mResourceNodes[resourceID.Get()];

        return node.mVersion == mEntryStates[node.mResourceEntryID.Get()].Version;
    }

    bool FrameGraphCore::passCreatesResource(const PassNode &passNode, FrameGraphResourceID resourceID) const {
        for (u32 i = 0; i < passNode.mCreateCount; ++i) {
            if (mCreates[passNode.mCreateBegin + i] == resourceID) {
                return true;
            }
        }

        return false;
    }

    FrameGraphResourceID FrameGraphCore::ReadImpl(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VASSERT(isCurrentVersion(resourceID), "Resource handle is stale: another pass has written this resource since the handle was obtained.");

        return registerRead(passNode, resourceID, usage);
    }

    FrameGraphResourceID FrameGraphCore::WriteImpl(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VASSERT(isCurrentVersion(resourceID), "Resource handle is stale: another pass has written this resource since the handle was obtained.");

        if (!mEntryStates[GetEntryID(resourceID).Get()].IsTransient) {
            // An imported resource is observable outside the graph, so writing one is a side effect by definition.
            passNode.MarkSideEffect();
        }

        if (passCreatesResource(passNode, resourceID)) {
            return RegisterWrite(passNode, resourceID, usage);
        }

        // Writing a resource the pass did not create renames it: the pass consumes the current version and
        // produces the next one. That is what forces two passes modifying the same resource into a defined order
        // and makes a stale handle detectable.
        const FrameGraphResourceID previous = registerRead(passNode, resourceID, ResourceUsage{});

        return RegisterWrite(passNode, createNextVersion(previous, passNode.GetPassID()), usage);
    }

    // ===========================================================================================
    // Compilation
    // ===========================================================================================

    void FrameGraphCore::Compile(std::span<const ResourceMemoryRequirements> requirements) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        computeReferenceCounts();
        cullUnreferencedPasses();
        buildExecutionOrder();
        computeResourceLifetimes();

        // Aliasing before barriers, not after: a transient that inherits another's bytes needs a discard at its
        // first use, and only the plan knows which resources those are.
        buildAliasingPlan(requirements);
        buildBarriers();

        mCompiled = true;
    }

    void FrameGraphCore::computeReferenceCounts() {
        for (PassNode &pass : mPassNodes) {
            // Creating a resource registers a write for it, so the write list already holds each distinct output
            // exactly once. Counting "creates" separately is what used to seed 2 for a single `Write(Create(...))`
            // output and floor the refcount at 1, silently making every such pass uncullable.
            pass.mLiveOutputCount = pass.mWriteCount;

            for (u32 i = 0; i < pass.mReadCount; ++i) {
                ++mResourceNodes[mReads[pass.mReadBegin + i].Resource.Get()].mTotalConsumers;
            }
        }
    }

    void FrameGraphCore::cullUnreferencedPasses() {
        for (const ResourceNode &node : mResourceNodes) {
            if (node.mTotalConsumers == 0) {
                mCullWorklist.push_back(node.mResourceID);
            }
        }

        while (!mCullWorklist.empty()) {
            const FrameGraphResourceID resourceID = mCullWorklist.back();
            mCullWorklist.pop_back();

            const FrameGraphPassID producerID = mResourceNodes[resourceID.Get()].mProducer;

            // Imported resources have no producer in the graph.
            if (!producerID.IsValid()) {
                continue;
            }

            PassNode &producer = mPassNodes[producerID.Get()];

            if (producer.mHasSideEffects) {
                continue;
            }

            VASSERT(producer.mLiveOutputCount >= 1, "Pass producer has no live outputs left to decrement.");

            // Release-safe floor: the assertion above compiles out, and an unguarded decrement would wrap to
            // 0xFFFFFFFF and resurrect the pass instead of trapping.
            if (producer.mLiveOutputCount == 0) {
                continue;
            }

            if (--producer.mLiveOutputCount == 0) {
                for (u32 i = 0; i < producer.mReadCount; ++i) {
                    ResourceNode &consumed = mResourceNodes[mReads[producer.mReadBegin + i].Resource.Get()];

                    if (consumed.mTotalConsumers > 0 && --consumed.mTotalConsumers == 0) {
                        mCullWorklist.push_back(consumed.mResourceID);
                    }
                }
            }
        }
    }

    void FrameGraphCore::buildExecutionOrder() {
        mInDegrees.assign(mPassNodes.size(), 0);

        const auto addEdge = [this](FrameGraphPassID predecessor, FrameGraphPassID successor) {
            if (!predecessor.IsValid() || !successor.IsValid() || predecessor == successor) {
                return;
            }

            if (!mPassNodes[predecessor.Get()].ShouldExecute() || !mPassNodes[successor.Get()].ShouldExecute()) {
                return;
            }

            mEdges.emplace_back(predecessor.Get(), successor.Get());
        };

        for (const PassNode &pass : mPassNodes) {
            if (!pass.ShouldExecute()) {
                continue;
            }

            for (u32 i = 0; i < pass.mReadCount; ++i) {
                const FrameGraphResourceID readID = mReads[pass.mReadBegin + i].Resource;

                // Read-after-write: whoever produced this version must run first.
                addEdge(mResourceNodes[readID.Get()].mProducer, pass.mPassID);

                // Write-after-read: whoever produces the *next* version must run after this reader,
                // or the reader would observe data that has already been overwritten.
                const FrameGraphResourceID nextID = mNextVersion[readID.Get()];

                if (nextID.IsValid()) {
                    addEdge(pass.mPassID, mResourceNodes[nextID.Get()].mProducer);
                }
            }
        }

        std::ranges::sort(mEdges);

        for (const auto &[predecessor, successor] : mEdges) {
            ++mInDegrees[successor];
        }

        for (u32 passIndex = 0; passIndex < mPassNodes.size(); ++passIndex) {
            if (mPassNodes[passIndex].ShouldExecute() && mInDegrees[passIndex] == 0) {
                mReadyPasses.push_back(passIndex);
            }
        }

        // A min-heap rather than a queue: among all valid topological orders this picks the one closest to
        // declaration order, so a graph already declared in dependency order compiles to exactly that order.
        const auto lowestFirst = std::greater<u32>{};
        std::ranges::make_heap(mReadyPasses, lowestFirst);

        while (!mReadyPasses.empty()) {
            std::ranges::pop_heap(mReadyPasses, lowestFirst);
            const u32 passIndex = mReadyPasses.back();
            mReadyPasses.pop_back();

            mPassNodes[passIndex].mExecutionIndex = static_cast<u32>(mExecutionOrder.size());
            mExecutionOrder.push_back(passIndex);

            const auto successors = std::ranges::equal_range(mEdges, passIndex, std::less<u32>{}, &std::pair<u32, u32>::first);

            for (const auto &[predecessor, successor] : successors) {
                if (--mInDegrees[successor] == 0) {
                    mReadyPasses.push_back(successor);
                    std::ranges::push_heap(mReadyPasses, lowestFirst);
                }
            }
        }

        const auto executableCount = static_cast<size_t>(std::ranges::count_if(mPassNodes, [](const PassNode &pass) { return pass.ShouldExecute(); }));

        if (mExecutionOrder.size() != executableCount) {
            // Not reachable through the Builder - handles only flow forward, so derived edges cannot form a cycle -
            // but a future graph source could produce one, and silently dropping passes would be worse.
            VERROR("Frame graph contains a dependency cycle: {} of {} executable passes could not be ordered. Appending them in declaration order.",
                   executableCount - mExecutionOrder.size(),
                   executableCount);

            for (u32 passIndex = 0; passIndex < mPassNodes.size(); ++passIndex) {
                if (mPassNodes[passIndex].ShouldExecute() && mPassNodes[passIndex].mExecutionIndex == UNORDERED) {
                    mPassNodes[passIndex].mExecutionIndex = static_cast<u32>(mExecutionOrder.size());
                    mExecutionOrder.push_back(passIndex);
                }
            }
        }
    }

    void FrameGraphCore::computeResourceLifetimes() {
        const auto touch = [this](FrameGraphResourceID resourceID, u32 order) {
            EntryState &state = mEntryStates[mResourceNodes[resourceID.Get()].mResourceEntryID.Get()];

            if (state.FirstUse == UNORDERED) {
                state.FirstUse = order;
            }

            state.LastUse = order;
        };

        for (u32 order = 0; order < mExecutionOrder.size(); ++order) {
            const PassNode &pass = mPassNodes[mExecutionOrder[order]];

            for (u32 i = 0; i < pass.mCreateCount; ++i) {
                touch(mCreates[pass.mCreateBegin + i], order);
            }

            for (u32 i = 0; i < pass.mWriteCount; ++i) {
                touch(mWrites[pass.mWriteBegin + i].Resource, order);
            }

            for (u32 i = 0; i < pass.mReadCount; ++i) {
                touch(mReads[pass.mReadBegin + i].Resource, order);
            }
        }

        // Bucket the releases by last-use position in two linear passes. The previous implementation scanned the
        // whole entry array once per pass, which is O(passes x resources) on the hot path.
        mReleaseCounts.assign(mExecutionOrder.size(), 0);

        for (const EntryState &state : mEntryStates) {
            if (state.IsTransient && state.LastUse != UNORDERED) {
                ++mReleaseCounts[state.LastUse];
            }
        }

        u32 running = 0;

        for (u32 order = 0; order < mExecutionOrder.size(); ++order) {
            PassNode &pass = mPassNodes[mExecutionOrder[order]];
            pass.mReleaseBegin = running;
            pass.mReleaseCount = 0;
            running += mReleaseCounts[order];
        }

        mReleases.resize(running);

        for (u32 entryIndex = 0; entryIndex < mEntryStates.size(); ++entryIndex) {
            const EntryState &state = mEntryStates[entryIndex];

            if (!state.IsTransient || state.LastUse == UNORDERED) {
                continue;
            }

            PassNode &pass = mPassNodes[mExecutionOrder[state.LastUse]];
            mReleases[pass.mReleaseBegin + pass.mReleaseCount] = FrameGraphResourceEntryID{ entryIndex };
            ++pass.mReleaseCount;
        }
    }

    void FrameGraphCore::buildBarriers() {
        mCurrentUsages.assign(mEntryStates.size(), ResourceUsage{});

        const auto transition = [this](FrameGraphResourceID resourceID, const ResourceUsage &desired) {
            const u32 entryIndex = mResourceNodes[resourceID.Get()].mResourceEntryID.Get();
            ResourceUsage &current = mCurrentUsages[entryIndex];

            ResourceUsage before = current;
            bool aliasing = false;

            // The first access to a transient that inherited another's bytes has to discard them. Only the stage
            // and access masks are carried over - they are what the discard must wait on - while the layout stays
            // zero, because the contents being replaced were never this resource's to transition from.
            if (mAliasPredecessorCount[entryIndex] > 0) {
                const u32 begin = mAliasPredecessorBegin[entryIndex];

                for (u32 i = 0; i < mAliasPredecessorCount[entryIndex]; ++i) {
                    const ResourceUsage &predecessor = mCurrentUsages[mAliasPredecessors[begin + i].Get()];

                    before.Stages |= predecessor.Stages;
                    before.Access |= predecessor.Access;
                }

                // Consumed, so every later access to this resource is an ordinary transition.
                mAliasPredecessorCount[entryIndex] = 0;
                aliasing = true;
            }

            // Nothing to transition when the resource is already in the required state - which is every access in
            // a graph that does not specify usages at all, so an OpenGL-style backend pays nothing here. A discard
            // is emitted regardless: "these bytes now hold something else" is not something the backend can derive.
            if (!aliasing && current == desired) {
                return;
            }

            mBarriers.push_back(
                ResourceBarrier{ .Entry = FrameGraphResourceEntryID{ entryIndex }, .Before = before, .After = desired, .AliasingTransition = aliasing });

            current = desired;
        };

        for (const u32 passIndex : mExecutionOrder) {
            PassNode &pass = mPassNodes[passIndex];
            pass.mBarrierBegin = static_cast<u32>(mBarriers.size());

            for (u32 i = 0; i < pass.mReadCount; ++i) {
                const ResourceAccess &access = mReads[pass.mReadBegin + i];
                transition(access.Resource, access.Usage);
            }

            for (u32 i = 0; i < pass.mWriteCount; ++i) {
                const ResourceAccess &access = mWrites[pass.mWriteBegin + i];
                transition(access.Resource, access.Usage);
            }

            pass.mBarrierCount = static_cast<u32>(mBarriers.size()) - pass.mBarrierBegin;
        }
    }

    void FrameGraphCore::buildAliasingPlan(std::span<const ResourceMemoryRequirements> requirements) {
        // Sized up front rather than alongside the predecessor scan below, so `buildBarriers` can index them even
        // when nothing is eligible for aliasing and this returns early.
        mAliasPredecessorBegin.assign(mEntryStates.size(), 0);
        mAliasPredecessorCount.assign(mEntryStates.size(), 0);

        VASSERT(requirements.size() == mEntryStates.size(), "Frame graph memory requirements must carry one entry per resource entry.");

        for (u32 entryIndex = 0; entryIndex < mEntryStates.size(); ++entryIndex) {
            const EntryState &state = mEntryStates[entryIndex];

            if (!state.IsTransient || state.FirstUse == UNORDERED) {
                continue;
            }

            // A resource type that does not report its requirements is handed a zero size and simply stays out of
            // the plan - as is every resource on a backend that cannot bind two of them to one allocation.
            const ResourceMemoryRequirements &entryRequirements = requirements[entryIndex];

            if (entryRequirements.Size == 0) {
                continue;
            }

            mAliasCandidates.push_back(AliasCandidate{ .EntryIndex = entryIndex,
                                                       .Size = entryRequirements.Size,
                                                       .Alignment = std::max<u64>(entryRequirements.Alignment, 1),
                                                       .FirstUse = state.FirstUse,
                                                       .LastUse = state.LastUse });
        }

        if (mAliasCandidates.empty()) {
            return;
        }

        // Largest first. Every transient is placed at an offset into one shared block, so a big resource laid down
        // early leaves gaps beside it that the smaller ones can then fill. The alternative - walking candidates by
        // first use and handing each a region reused whole - sizes every region to the largest thing that ever
        // occupies it, which on a frame mixing 32 MB targets with 4 KB scratch costs multiples of the peak demand.
        std::ranges::sort(mAliasCandidates, [](const AliasCandidate &lhs, const AliasCandidate &rhs) {
            // First use breaks size ties so the plan does not depend on the sort's stability.
            return lhs.Size != rhs.Size ? lhs.Size > rhs.Size : lhs.FirstUse < rhs.FirstUse;
        });

        u64 highWater = 0;

        for (u32 index = 0; index < mAliasCandidates.size(); ++index) {
            AliasCandidate &candidate = mAliasCandidates[index];
            u64 offset = 0;

            // Sweep the placements in offset order, pushing past everything whose lifetime overlaps this candidate
            // until a wide enough gap opens up. Because they are ordered, the first such gap is the lowest one, and
            // anything at a higher offset than the end of it cannot conflict - hence the early exit.
            for (const u32 placedIndex : mAliasPlacements) {
                const AliasCandidate &placed = mAliasCandidates[placedIndex];

                // Tested before the lifetime check, not after: everything from here on sits at this offset or
                // higher, so nothing left can reach back into the gap and the answer is already settled.
                if (placed.Offset >= offset + candidate.Size) {
                    break;
                }

                // Disjoint lifetimes may share bytes, which is the whole point of the plan.
                if (placed.LastUse < candidate.FirstUse || candidate.LastUse < placed.FirstUse) {
                    continue;
                }

                offset = std::max(offset, AlignUp(placed.Offset + placed.Size, candidate.Alignment));
            }

            candidate.Offset = offset;
            highWater = std::max(highWater, offset + candidate.Size);

            const auto position = std::ranges::upper_bound(mAliasPlacements, offset, {}, [this](u32 i) { return mAliasCandidates[i].Offset; });
            mAliasPlacements.insert(position, index);
        }

        // Peak live bytes, as a running sum over a per-position delta: the floor `highWater` is measured against.
        u64 totalBytes = 0;

        mAliasLiveDelta.assign(mExecutionOrder.size() + 1, 0);

        for (const AliasCandidate &candidate : mAliasCandidates) {
            totalBytes += candidate.Size;

            mAliasLiveDelta[candidate.FirstUse] += static_cast<i64>(candidate.Size);
            mAliasLiveDelta[candidate.LastUse + 1] -= static_cast<i64>(candidate.Size);

            EntryState &state = mEntryStates[candidate.EntryIndex];
            state.AliasOffset = candidate.Offset;
            state.IsAliased = true;
        }

        i64 liveBytes = 0;
        i64 peakLiveBytes = 0;

        for (const i64 delta : mAliasLiveDelta) {
            liveBytes += delta;
            peakLiveBytes = std::max(peakLiveBytes, liveBytes);
        }

        // Record, for every candidate, the earlier occupants of the bytes it is taking over, so `buildBarriers` can
        // source its discard from all of them. Every one is needed, not just the most recent: when a large resource
        // takes over the space of several smaller ones that never overlapped each other, nothing has ordered those
        // smaller ones against one another, so naming only the last of them leaves the rest without a source scope.
        for (const AliasCandidate &candidate : mAliasCandidates) {
            const auto begin = static_cast<u32>(mAliasPredecessors.size());
            const u64 candidateEnd = candidate.Offset + candidate.Size;

            // Walked through the offset-ordered placement list rather than the size-ordered candidate list, so the
            // scan stops at the first placement starting past this candidate's last byte instead of running to the
            // end every time.
            for (const u32 placedIndex : mAliasPlacements) {
                const AliasCandidate &other = mAliasCandidates[placedIndex];

                if (other.Offset >= candidateEnd) {
                    break;
                }

                // A candidate never matches itself: it shares all its own bytes, but its own last use is never
                // before its own first use.
                if (candidate.Offset < other.Offset + other.Size && other.LastUse < candidate.FirstUse) {
                    mAliasPredecessors.push_back(FrameGraphResourceEntryID{ other.EntryIndex });
                }
            }

            mAliasPredecessorBegin[candidate.EntryIndex] = begin;
            mAliasPredecessorCount[candidate.EntryIndex] = static_cast<u32>(mAliasPredecessors.size()) - begin;
        }

        mAliasingReport = FrameGraphAliasingReport{ .UnaliasedBytes = totalBytes,
                                                    .AliasedBytes = highWater,
                                                    .PeakLiveBytes = static_cast<u64>(peakLiveBytes),
                                                    .ResourceCount = static_cast<u32>(mAliasCandidates.size()) };
    }


    std::string FrameGraphCore::ToDot() const {
        std::ostringstream out;

        out << "digraph FrameGraph {\n";
        out << "    rankdir=LR;\n";
        out << "    node [fontname=\"Helvetica\", fontsize=10];\n";
        out << "    edge [fontname=\"Helvetica\", fontsize=8];\n\n";

        for (const PassNode &pass : mPassNodes) {
            const bool culled = !pass.ShouldExecute();

            out << "    P" << pass.mPassID.Get() << " [shape=box, style=\"filled,rounded\", fillcolor=\"" << (culled ? "#f0f0f0" : "#cfe2ff") << "\", label=\""
                << mPassNames[pass.mPassID.Get()].View();

            if (culled) {
                out << "\\n(culled)";
            } else {
                out << "\\n#" << pass.mExecutionIndex;
            }

            out << "\"];\n";
        }

        out << "\n";

        for (const ResourceNode &node : mResourceNodes) {
            const EntryState &state = mEntryStates[node.mResourceEntryID.Get()];

            out << "    R" << node.mResourceID.Get() << " [shape=ellipse, style=filled, fillcolor=\"" << (state.IsTransient ? "#d7f2d7" : "#ffe5b4")
                << "\", label=\"" << mResourceNames[node.mResourceID.Get()].View() << "\\nv" << node.mVersion << "\"];\n";
        }

        out << "\n";

        for (const PassNode &pass : mPassNodes) {
            for (u32 i = 0; i < pass.mCreateCount; ++i) {
                out << "    P" << pass.mPassID.Get() << " -> R" << mCreates[pass.mCreateBegin + i].Get() << " [style=dashed, label=\"create\"];\n";
            }

            for (u32 i = 0; i < pass.mWriteCount; ++i) {
                const FrameGraphResourceID written = mWrites[pass.mWriteBegin + i].Resource;

                // A created resource already has a dashed "create" arrow; the implied write would duplicate it.
                if (!passCreatesResource(pass, written)) {
                    out << "    P" << pass.mPassID.Get() << " -> R" << written.Get() << " [label=\"write\"];\n";
                }
            }

            for (u32 i = 0; i < pass.mReadCount; ++i) {
                out << "    R" << mReads[pass.mReadBegin + i].Resource.Get() << " -> P" << pass.mPassID.Get() << " [label=\"read\"];\n";
            }
        }

        out << "}\n";

        return out.str();
    }

} // namespace Vulkyrie::detail
