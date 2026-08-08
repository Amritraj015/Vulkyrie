#include "renderer/frame_graph/frame_graph.h"

#include "core/jobs/parallel_for.h"

namespace Vulkyrie {

    namespace {

        /** @brief Sentinel for "this pass/resource has no position in the execution order". */
        constexpr u32 UNORDERED = std::numeric_limits<u32>::max();

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

    FrameGraph::FrameGraph(const FrameGraphConfig &config)
        : _arena{ config.InitialArenaBytes, MemoryTag::Rendering } {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        // Reserve everything up front so a graph rebuilt every frame stops allocating after frame one. The
        // multipliers are rough shapes of a real graph: most resources are read by more than one pass, and most
        // passes write fewer resources than they read.
        _passNodes.reserve(config.ExpectedPasses);
        _passNames.reserve(config.ExpectedPasses);
        _resourceNodes.reserve(config.ExpectedResources);
        _resourceNames.reserve(config.ExpectedResources);
        _nextVersion.reserve(config.ExpectedResources);
        _resourceEntries.reserve(config.ExpectedResources);
        _latestVersionOfEntry.reserve(config.ExpectedResources);
        _creates.reserve(config.ExpectedResources);
        _reads.reserve(static_cast<size_t>(config.ExpectedResources) * 2);
        _writes.reserve(config.ExpectedResources);
        _releases.reserve(config.ExpectedResources);
        _barriers.reserve(static_cast<size_t>(config.ExpectedResources) * 2);
        _executionOrder.reserve(config.ExpectedPasses);
        _inDegrees.reserve(config.ExpectedPasses);
        _readyPasses.reserve(config.ExpectedPasses);
        _edges.reserve(static_cast<size_t>(config.ExpectedPasses) * 4);
        _cullWorklist.reserve(config.ExpectedResources);
        _currentUsages.reserve(config.ExpectedResources);
        _releaseCounts.reserve(config.ExpectedPasses);
        _aliasCandidates.reserve(config.ExpectedResources);
        _aliasPlacements.reserve(config.ExpectedResources);
        _aliasLiveDelta.reserve(config.ExpectedPasses + 1);
        _aliasPredecessors.reserve(config.ExpectedResources);
        _aliasPredecessorBegin.reserve(config.ExpectedResources);
        _aliasPredecessorCount.reserve(config.ExpectedResources);
    }

    FrameGraph::~FrameGraph() {
        runDestructors();
    }

    void FrameGraph::runDestructors() {
        for (PassNode &pass : _passNodes) {
            if (nullptr != pass._destroyPayload) {
                pass._destroyPayload(pass._payload);
            }
        }

        for (ResourceEntry &entry : _resourceEntries) {
            entry.destructStorage();
        }
    }

    void FrameGraph::Reset() {
        Reset(FrameGraphContext{});
    }

    void FrameGraph::Reset(const FrameGraphContext &context) {
        // A frame that was compiled but never executed can leave transients materialized; release them rather
        // than leaking resource objects.
        for (ResourceEntry &entry : _resourceEntries) {
            entry.destroyResource(context);
        }

        runDestructors();

        // clear() rather than shrinking: every buffer keeps its capacity,
        // and the arena keeps its chunks, which is what makes the next frame allocation-free.
        _passNodes.clear();
        _passNames.clear();
        _resourceNodes.clear();
        _resourceNames.clear();
        _nextVersion.clear();
        _resourceEntries.clear();
        _latestVersionOfEntry.clear();
        _creates.clear();
        _reads.clear();
        _writes.clear();
        _releases.clear();
        _barriers.clear();
        _executionOrder.clear();
        _inDegrees.clear();
        _readyPasses.clear();
        _edges.clear();
        _cullWorklist.clear();
        _currentUsages.clear();
        _releaseCounts.clear();
        _aliasCandidates.clear();
        _aliasPlacements.clear();
        _aliasLiveDelta.clear();
        _aliasPredecessors.clear();
        _aliasPredecessorBegin.clear();
        _aliasPredecessorCount.clear();

        _arena.Reset();

        _aliasingReport = FrameGraphAliasingReport{};
        _compiled = false;
        _inSetup = false;
    }

    // ===========================================================================================
    // Graph construction
    // ===========================================================================================

    PassNode &FrameGraph::createPassNode(StaticString name, void *payload, PassNode::InvokeFn invoke, PassNode::DestroyFn destroy) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        const auto passID = FrameGraphPassID{ static_cast<u32>(_passNodes.size()) };

        _passNames.push_back(name);

        return _passNodes.emplace_back(
            PassNode{ passID, payload, invoke, destroy, static_cast<u32>(_creates.size()), static_cast<u32>(_reads.size()), static_cast<u32>(_writes.size()) });
    }

    FrameGraphResourceID FrameGraph::createResourceNode(StaticString name, FrameGraphResourceEntryID entryID, u32 version, FrameGraphPassID producer) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        const auto resourceID = FrameGraphResourceID{ static_cast<u32>(_resourceNodes.size()) };

        _resourceNames.push_back(name);
        _resourceNodes.push_back(ResourceNode{ resourceID, entryID, version, producer });
        _nextVersion.push_back(FrameGraphResourceID{});

        if (_latestVersionOfEntry.size() <= entryID.Get()) {
            _latestVersionOfEntry.resize(entryID.Get() + 1, FrameGraphResourceID{});
        }

        // Link the previous version to this one so Compile can derive write-after-read edges in O(1) per read.
        const FrameGraphResourceID previous = _latestVersionOfEntry[entryID.Get()];

        if (previous.IsValid()) {
            _nextVersion[previous.Get()] = resourceID;
        }

        _latestVersionOfEntry[entryID.Get()] = resourceID;

        return resourceID;
    }

    void FrameGraph::registerCreate(PassNode &passNode, FrameGraphResourceID resourceID) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        _creates.push_back(resourceID);
        ++passNode._createCount;
    }

    FrameGraphResourceID FrameGraph::registerRead(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        VASSERT(!passCreatesResource(passNode, resourceID), "A pass cannot read a resource it creates in the same pass.");

        for (u32 i = 0; i < passNode._readCount; ++i) {
            ResourceAccess &access = _reads[passNode._readBegin + i];

            if (access.Resource == resourceID) {
                MergeUsage(access.Usage, usage);
                return resourceID;
            }
        }

        _reads.push_back(ResourceAccess{ .Resource = resourceID, .Usage = usage });
        ++passNode._readCount;

        return resourceID;
    }

    FrameGraphResourceID FrameGraph::registerWrite(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        for (u32 i = 0; i < passNode._writeCount; ++i) {
            ResourceAccess &access = _writes[passNode._writeBegin + i];

            if (access.Resource == resourceID) {
                MergeUsage(access.Usage, usage);
                return resourceID;
            }
        }

        _writes.push_back(ResourceAccess{ .Resource = resourceID, .Usage = usage });
        ++passNode._writeCount;

        return resourceID;
    }

    FrameGraphResourceID FrameGraph::createNextVersion(FrameGraphResourceID resourceID, FrameGraphPassID producer) {
        const StaticString name = _resourceNames[resourceID.Get()];
        const FrameGraphResourceEntryID entryID = _resourceNodes[resourceID.Get()]._resourceEntryID;

        ResourceEntry &entry = _resourceEntries[entryID.Get()];
        ++entry._version;

        // Every version shares the original's name.
        return createResourceNode(name, entryID, entry._version, producer);
    }

    bool FrameGraph::isCurrentVersion(FrameGraphResourceID resourceID) const {
        if (!resourceID.IsValid() || resourceID.Get() >= _resourceNodes.size()) {
            return false;
        }

        const ResourceNode &node = _resourceNodes[resourceID.Get()];

        return node._version == _resourceEntries[node._resourceEntryID.Get()]._version;
    }

    bool FrameGraph::passCreatesResource(const PassNode &passNode, FrameGraphResourceID resourceID) const {
        for (u32 i = 0; i < passNode._createCount; ++i) {
            if (_creates[passNode._createBegin + i] == resourceID) {
                return true;
            }
        }

        return false;
    }

    FrameGraphResourceID FrameGraph::Builder::readImpl(FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VASSERT(_frameGraph.isCurrentVersion(resourceID), "Resource handle is stale: another pass has written this resource since the handle was obtained.");

        return _frameGraph.registerRead(_passNode, resourceID, usage);
    }

    FrameGraphResourceID FrameGraph::Builder::writeImpl(FrameGraphResourceID resourceID, const ResourceUsage &usage) {
        VASSERT(_frameGraph.isCurrentVersion(resourceID), "Resource handle is stale: another pass has written this resource since the handle was obtained.");

        if (_frameGraph.getResourceEntry(resourceID).IsImported()) {
            // An imported resource is observable outside the graph, so writing one is a side effect by definition.
            MarkSideEffect();
        }

        if (_frameGraph.passCreatesResource(_passNode, resourceID)) {
            return _frameGraph.registerWrite(_passNode, resourceID, usage);
        }

        // Writing a resource the pass did not create renames it: the pass consumes the current version and
        // produces the next one. That is what forces two passes modifying the same resource into a defined order
        // and makes a stale handle detectable.
        const FrameGraphResourceID previous = _frameGraph.registerRead(_passNode, resourceID, ResourceUsage{});

        return _frameGraph.registerWrite(_passNode, _frameGraph.createNextVersion(previous, _passNode.GetPassID()), usage);
    }

    // ===========================================================================================
    // Compilation
    // ===========================================================================================

    void FrameGraph::Compile() {
        VE_MEMORY_SCOPE(MemoryTag::Rendering);

        // If the graph has already been compiled,
        // then there is nothing to do, return early.
        if (_compiled)
            return;

        computeReferenceCounts();
        cullUnreferencedPasses();
        buildExecutionOrder();
        computeResourceLifetimes();

        // Aliasing before barriers, not after: a transient that inherits another's bytes needs a discard at its
        // first use, and only the plan knows which resources those are.
        buildAliasingPlan();
        buildBarriers();

        _compiled = true;
    }

    void FrameGraph::computeReferenceCounts() {
        for (PassNode &pass : _passNodes) {
            // Creating a resource registers a write for it, so the write list already holds each distinct output
            // exactly once. Counting "creates" separately is what used to seed 2 for a single `Write(Create(...))`
            // output and floor the refcount at 1, silently making every such pass uncullable.
            pass._liveOutputCount = pass._writeCount;

            for (u32 i = 0; i < pass._readCount; ++i) {
                ++_resourceNodes[_reads[pass._readBegin + i].Resource.Get()]._totalConsumers;
            }
        }
    }

    void FrameGraph::cullUnreferencedPasses() {
        for (const ResourceNode &node : _resourceNodes) {
            if (node._totalConsumers == 0) {
                _cullWorklist.push_back(node._resourceID);
            }
        }

        while (!_cullWorklist.empty()) {
            const FrameGraphResourceID resourceID = _cullWorklist.back();
            _cullWorklist.pop_back();

            const FrameGraphPassID producerID = _resourceNodes[resourceID.Get()]._producer;

            // Imported resources have no producer in the graph.
            if (!producerID.IsValid()) {
                continue;
            }

            PassNode &producer = _passNodes[producerID.Get()];

            if (producer._hasSideEffects) {
                continue;
            }

            VASSERT(producer._liveOutputCount >= 1, "Pass producer has no live outputs left to decrement.");

            // Release-safe floor: the assertion above compiles out, and an unguarded decrement would wrap to
            // 0xFFFFFFFF and resurrect the pass instead of trapping.
            if (producer._liveOutputCount == 0) {
                continue;
            }

            if (--producer._liveOutputCount == 0) {
                for (u32 i = 0; i < producer._readCount; ++i) {
                    ResourceNode &consumed = _resourceNodes[_reads[producer._readBegin + i].Resource.Get()];

                    if (consumed._totalConsumers > 0 && --consumed._totalConsumers == 0) {
                        _cullWorklist.push_back(consumed._resourceID);
                    }
                }
            }
        }
    }

    void FrameGraph::buildExecutionOrder() {
        _inDegrees.assign(_passNodes.size(), 0);

        const auto addEdge = [this](FrameGraphPassID predecessor, FrameGraphPassID successor) {
            if (!predecessor.IsValid() || !successor.IsValid() || predecessor == successor) {
                return;
            }

            if (!_passNodes[predecessor.Get()].ShouldExecute() || !_passNodes[successor.Get()].ShouldExecute()) {
                return;
            }

            _edges.emplace_back(predecessor.Get(), successor.Get());
        };

        for (const PassNode &pass : _passNodes) {
            if (!pass.ShouldExecute()) {
                continue;
            }

            for (u32 i = 0; i < pass._readCount; ++i) {
                const FrameGraphResourceID readID = _reads[pass._readBegin + i].Resource;

                // Read-after-write: whoever produced this version must run first.
                addEdge(_resourceNodes[readID.Get()]._producer, pass._passID);

                // Write-after-read: whoever produces the *next* version must run after this reader,
                // or the reader would observe data that has already been overwritten.
                const FrameGraphResourceID nextID = _nextVersion[readID.Get()];

                if (nextID.IsValid()) {
                    addEdge(pass._passID, _resourceNodes[nextID.Get()]._producer);
                }
            }
        }

        std::ranges::sort(_edges);

        for (const auto &[predecessor, successor] : _edges) {
            ++_inDegrees[successor];
        }

        for (u32 passIndex = 0; passIndex < _passNodes.size(); ++passIndex) {
            if (_passNodes[passIndex].ShouldExecute() && _inDegrees[passIndex] == 0) {
                _readyPasses.push_back(passIndex);
            }
        }

        // A min-heap rather than a queue: among all valid topological orders this picks the one closest to
        // declaration order, so a graph already declared in dependency order compiles to exactly that order.
        const auto lowestFirst = std::greater<u32>{};
        std::ranges::make_heap(_readyPasses, lowestFirst);

        while (!_readyPasses.empty()) {
            std::ranges::pop_heap(_readyPasses, lowestFirst);
            const u32 passIndex = _readyPasses.back();
            _readyPasses.pop_back();

            _passNodes[passIndex]._executionIndex = static_cast<u32>(_executionOrder.size());
            _executionOrder.push_back(passIndex);

            const auto successors = std::ranges::equal_range(_edges, passIndex, std::less<u32>{}, &std::pair<u32, u32>::first);

            for (const auto &[predecessor, successor] : successors) {
                if (--_inDegrees[successor] == 0) {
                    _readyPasses.push_back(successor);
                    std::ranges::push_heap(_readyPasses, lowestFirst);
                }
            }
        }

        const auto executableCount = static_cast<size_t>(std::ranges::count_if(_passNodes, [](const PassNode &pass) { return pass.ShouldExecute(); }));

        if (_executionOrder.size() != executableCount) {
            // Not reachable through the Builder - handles only flow forward, so derived edges cannot form a cycle -
            // but a future graph source could produce one, and silently dropping passes would be worse.
            VERROR("Frame graph contains a dependency cycle: {} of {} executable passes could not be ordered. Appending them in declaration order.",
                   executableCount - _executionOrder.size(),
                   executableCount);

            for (u32 passIndex = 0; passIndex < _passNodes.size(); ++passIndex) {
                if (_passNodes[passIndex].ShouldExecute() && _passNodes[passIndex]._executionIndex == UNORDERED) {
                    _passNodes[passIndex]._executionIndex = static_cast<u32>(_executionOrder.size());
                    _executionOrder.push_back(passIndex);
                }
            }
        }
    }

    void FrameGraph::computeResourceLifetimes() {
        const auto touch = [this](FrameGraphResourceID resourceID, u32 order) {
            ResourceEntry &entry = _resourceEntries[_resourceNodes[resourceID.Get()]._resourceEntryID.Get()];

            if (entry._firstUseIndex == UNORDERED) {
                entry._firstUseIndex = order;
            }

            entry._lastUseIndex = order;
        };

        for (u32 order = 0; order < _executionOrder.size(); ++order) {
            const PassNode &pass = _passNodes[_executionOrder[order]];

            for (u32 i = 0; i < pass._createCount; ++i) {
                touch(_creates[pass._createBegin + i], order);
            }

            for (u32 i = 0; i < pass._writeCount; ++i) {
                touch(_writes[pass._writeBegin + i].Resource, order);
            }

            for (u32 i = 0; i < pass._readCount; ++i) {
                touch(_reads[pass._readBegin + i].Resource, order);
            }
        }

        // Bucket the releases by last-use position in two linear passes. The previous implementation scanned the
        // whole entry array once per pass, which is O(passes x resources) on the hot path.
        _releaseCounts.assign(_executionOrder.size(), 0);

        for (const ResourceEntry &entry : _resourceEntries) {
            if (entry.IsTransient() && entry._lastUseIndex != UNORDERED) {
                ++_releaseCounts[entry._lastUseIndex];
            }
        }

        u32 running = 0;

        for (u32 order = 0; order < _executionOrder.size(); ++order) {
            PassNode &pass = _passNodes[_executionOrder[order]];
            pass._releaseBegin = running;
            pass._releaseCount = 0;
            running += _releaseCounts[order];
        }

        _releases.resize(running);

        for (const ResourceEntry &entry : _resourceEntries) {
            if (!entry.IsTransient() || entry._lastUseIndex == UNORDERED) {
                continue;
            }

            PassNode &pass = _passNodes[_executionOrder[entry._lastUseIndex]];
            _releases[pass._releaseBegin + pass._releaseCount] = entry._resourceEntryID;
            ++pass._releaseCount;
        }
    }

    void FrameGraph::buildBarriers() {
        _currentUsages.assign(_resourceEntries.size(), ResourceUsage{});

        const auto transition = [this](FrameGraphResourceID resourceID, const ResourceUsage &desired) {
            const u32 entryIndex = _resourceNodes[resourceID.Get()]._resourceEntryID.Get();
            ResourceUsage &current = _currentUsages[entryIndex];

            ResourceUsage before = current;
            bool aliasing = false;

            // The first access to a transient that inherited another's bytes has to discard them. Only the stage
            // and access masks are carried over - they are what the discard must wait on - while the layout stays
            // zero, because the contents being replaced were never this resource's to transition from.
            if (_aliasPredecessorCount[entryIndex] > 0) {
                const u32 begin = _aliasPredecessorBegin[entryIndex];

                for (u32 i = 0; i < _aliasPredecessorCount[entryIndex]; ++i) {
                    const ResourceUsage &predecessor = _currentUsages[_aliasPredecessors[begin + i].Get()];

                    before.Stages |= predecessor.Stages;
                    before.Access |= predecessor.Access;
                }

                // Consumed, so every later access to this resource is an ordinary transition.
                _aliasPredecessorCount[entryIndex] = 0;
                aliasing = true;
            }

            // Nothing to transition when the resource is already in the required state - which is every access in
            // a graph that does not specify usages at all, so an OpenGL-style backend pays nothing here. A discard
            // is emitted regardless: "these bytes now hold something else" is not something the backend can derive.
            if (!aliasing && current == desired) {
                return;
            }

            _barriers.push_back(
                ResourceBarrier{ .Entry = FrameGraphResourceEntryID{ entryIndex }, .Before = before, .After = desired, .AliasingTransition = aliasing });

            current = desired;
        };

        for (const u32 passIndex : _executionOrder) {
            PassNode &pass = _passNodes[passIndex];
            pass._barrierBegin = static_cast<u32>(_barriers.size());

            for (u32 i = 0; i < pass._readCount; ++i) {
                const ResourceAccess &access = _reads[pass._readBegin + i];
                transition(access.Resource, access.Usage);
            }

            for (u32 i = 0; i < pass._writeCount; ++i) {
                const ResourceAccess &access = _writes[pass._writeBegin + i];
                transition(access.Resource, access.Usage);
            }

            pass._barrierCount = static_cast<u32>(_barriers.size()) - pass._barrierBegin;
        }
    }

    void FrameGraph::buildAliasingPlan() {
        // Sized up front rather than alongside the predecessor scan below, so `buildBarriers` can index them even
        // when nothing is eligible for aliasing and this returns early.
        _aliasPredecessorBegin.assign(_resourceEntries.size(), 0);
        _aliasPredecessorCount.assign(_resourceEntries.size(), 0);

        for (const ResourceEntry &entry : _resourceEntries) {
            if (!entry.IsTransient() || entry._firstUseIndex == UNORDERED) {
                continue;
            }

            const std::optional<ResourceMemoryRequirements> requirements = entry.getMemoryRequirements();

            // A resource type that does not report its requirements simply stays out of the plan.
            if (!requirements.has_value() || requirements->Size == 0) {
                continue;
            }

            _aliasCandidates.push_back(AliasCandidate{ .EntryIndex = entry._resourceEntryID.Get(),
                                                       .Size = requirements->Size,
                                                       .Alignment = std::max<u64>(requirements->Alignment, 1),
                                                       .FirstUse = entry._firstUseIndex,
                                                       .LastUse = entry._lastUseIndex });
        }

        if (_aliasCandidates.empty()) {
            return;
        }

        // Largest first. Every transient is placed at an offset into one shared block, so a big resource laid down
        // early leaves gaps beside it that the smaller ones can then fill. The alternative - walking candidates by
        // first use and handing each a region reused whole - sizes every region to the largest thing that ever
        // occupies it, which on a frame mixing 32 MB targets with 4 KB scratch costs multiples of the peak demand.
        std::ranges::sort(_aliasCandidates, [](const AliasCandidate &lhs, const AliasCandidate &rhs) {
            // First use breaks size ties so the plan does not depend on the sort's stability.
            return lhs.Size != rhs.Size ? lhs.Size > rhs.Size : lhs.FirstUse < rhs.FirstUse;
        });

        u64 highWater = 0;

        for (u32 index = 0; index < _aliasCandidates.size(); ++index) {
            AliasCandidate &candidate = _aliasCandidates[index];
            u64 offset = 0;

            // Sweep the placements in offset order, pushing past everything whose lifetime overlaps this candidate
            // until a wide enough gap opens up. Because they are ordered, the first such gap is the lowest one, and
            // anything at a higher offset than the end of it cannot conflict - hence the early exit.
            for (const u32 placedIndex : _aliasPlacements) {
                const AliasCandidate &placed = _aliasCandidates[placedIndex];

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

            const auto position = std::ranges::upper_bound(_aliasPlacements, offset, {}, [this](u32 i) { return _aliasCandidates[i].Offset; });
            _aliasPlacements.insert(position, index);
        }

        // Peak live bytes, as a running sum over a per-position delta: the floor `highWater` is measured against.
        u64 totalBytes = 0;

        _aliasLiveDelta.assign(_executionOrder.size() + 1, 0);

        for (const AliasCandidate &candidate : _aliasCandidates) {
            totalBytes += candidate.Size;

            _aliasLiveDelta[candidate.FirstUse] += static_cast<i64>(candidate.Size);
            _aliasLiveDelta[candidate.LastUse + 1] -= static_cast<i64>(candidate.Size);

            ResourceEntry &entry = _resourceEntries[candidate.EntryIndex];
            entry._aliasOffset = candidate.Offset;
            entry._isAliased = true;
        }

        i64 liveBytes = 0;
        i64 peakLiveBytes = 0;

        for (const i64 delta : _aliasLiveDelta) {
            liveBytes += delta;
            peakLiveBytes = std::max(peakLiveBytes, liveBytes);
        }

        // Record, for every candidate, the earlier occupants of the bytes it is taking over, so `buildBarriers` can
        // source its discard from all of them. Every one is needed, not just the most recent: when a large resource
        // takes over the space of several smaller ones that never overlapped each other, nothing has ordered those
        // smaller ones against one another, so naming only the last of them leaves the rest without a source scope.
        for (const AliasCandidate &candidate : _aliasCandidates) {
            const auto begin = static_cast<u32>(_aliasPredecessors.size());
            const u64 candidateEnd = candidate.Offset + candidate.Size;

            // Walked through the offset-ordered placement list rather than the size-ordered candidate list, so the
            // scan stops at the first placement starting past this candidate's last byte instead of running to the
            // end every time.
            for (const u32 placedIndex : _aliasPlacements) {
                const AliasCandidate &other = _aliasCandidates[placedIndex];

                if (other.Offset >= candidateEnd) {
                    break;
                }

                // A candidate never matches itself: it shares all its own bytes, but its own last use is never
                // before its own first use.
                if (candidate.Offset < other.Offset + other.Size && other.LastUse < candidate.FirstUse) {
                    _aliasPredecessors.push_back(FrameGraphResourceEntryID{ other.EntryIndex });
                }
            }

            _aliasPredecessorBegin[candidate.EntryIndex] = begin;
            _aliasPredecessorCount[candidate.EntryIndex] = static_cast<u32>(_aliasPredecessors.size()) - begin;
        }

        _aliasingReport = FrameGraphAliasingReport{ .UnaliasedBytes = totalBytes,
                                                    .AliasedBytes = highWater,
                                                    .PeakLiveBytes = static_cast<u64>(peakLiveBytes),
                                                    .ResourceCount = static_cast<u32>(_aliasCandidates.size()) };
    }

    // ===========================================================================================
    // Execution
    // ===========================================================================================

    void FrameGraph::Execute(const FrameGraphContext &context) {
        VASSERT(_compiled, "FrameGraph::Execute called before Compile().");

        for (const u32 passIndex : _executionOrder) {
            const PassNode &pass = _passNodes[passIndex];

            for (u32 i = 0; i < pass._createCount; ++i) {
                getResourceEntry(_creates[pass._createBegin + i]).createResource(context);
            }

            emitBarriers(passIndex, context);
            runAccessHooks(passIndex, context);
            invokePassBody(passIndex, context);

            for (u32 i = 0; i < pass._releaseCount; ++i) {
                _resourceEntries[_releases[pass._releaseBegin + i].Get()].destroyResource(context);
            }
        }
    }

    void FrameGraph::Record(const FrameGraphContext &context) {
        VASSERT(_compiled, "FrameGraph::Record called before Compile().");

        prepareResources(context);

        for (const u32 passIndex : _executionOrder) {
            runAccessHooks(passIndex, context);
            invokePassBody(passIndex, context);
        }
    }

    void FrameGraph::RecordParallel(const FrameGraphContext &context) {
        VASSERT(_compiled, "FrameGraph::RecordParallel called before Compile().");

        prepareResources(context);

        const auto passCount = static_cast<u32>(_executionOrder.size());

        if (passCount == 0) {
            return;
        }

        // The access hooks stay on this thread. They are resource-state bookkeeping, not command recording, and
        // running them concurrently would force every resource type to be thread-safe just to be usable from the
        // parallel path - a much wider contract than the one this split is meant to impose.
        for (const u32 passIndex : _executionOrder) {
            runAccessHooks(passIndex, context);
        }

        // Recording is order-independent; only submission is ordered. Every pass therefore records concurrently
        // and Submit replays the topological order afterwards.
        ParallelFor(passCount, 1, [this, &context](u32 index) { invokePassBody(_executionOrder[index], context); });
    }

    void FrameGraph::Submit(const FrameGraphContext &context) {
        VASSERT(_compiled, "FrameGraph::Submit called before Compile().");

        for (const u32 passIndex : _executionOrder) {
            emitBarriers(passIndex, context);
        }

        for (ResourceEntry &entry : _resourceEntries) {
            entry.destroyResource(context);
        }
    }

    void FrameGraph::prepareResources(const FrameGraphContext &context) {
        for (const u32 passIndex : _executionOrder) {
            const PassNode &pass = _passNodes[passIndex];

            for (u32 i = 0; i < pass._createCount; ++i) {
                getResourceEntry(_creates[pass._createBegin + i]).createResource(context);
            }
        }
    }

    void FrameGraph::runAccessHooks(u32 passIndex, const FrameGraphContext &context) {
        const PassNode &pass = _passNodes[passIndex];

        for (u32 i = 0; i < pass._readCount; ++i) {
            const ResourceAccess &access = _reads[pass._readBegin + i];
            getResourceEntry(access.Resource).preRead(access.Usage, context);
        }

        for (u32 i = 0; i < pass._writeCount; ++i) {
            const ResourceAccess &access = _writes[pass._writeBegin + i];
            getResourceEntry(access.Resource).preWrite(access.Usage, context);
        }
    }

    void FrameGraph::invokePassBody(u32 passIndex, const FrameGraphContext &context) {
        const PassNode &pass = _passNodes[passIndex];

        pass._invoke(pass._payload, *this, context);
    }

    void FrameGraph::emitBarriers(u32 passIndex, const FrameGraphContext &context) const {
        const PassNode &pass = _passNodes[passIndex];

        if (context.EmitBarriers == nullptr || pass._barrierCount == 0) {
            return;
        }

        // One call per pass carrying every transition, rather than one call per resource.
        context.EmitBarriers(context, std::span<const ResourceBarrier>{ _barriers.data() + pass._barrierBegin, pass._barrierCount });
    }

    // ===========================================================================================
    // Tooling
    // ===========================================================================================

    std::string FrameGraph::ToDot() const {
        std::ostringstream out;

        out << "digraph FrameGraph {\n";
        out << "    rankdir=LR;\n";
        out << "    node [fontname=\"Helvetica\", fontsize=10];\n";
        out << "    edge [fontname=\"Helvetica\", fontsize=8];\n\n";

        for (const PassNode &pass : _passNodes) {
            const bool culled = !pass.ShouldExecute();

            out << "    P" << pass._passID.Get() << " [shape=box, style=\"filled,rounded\", fillcolor=\"" << (culled ? "#f0f0f0" : "#cfe2ff") << "\", label=\""
                << _passNames[pass._passID.Get()].View();

            if (culled) {
                out << "\\n(culled)";
            } else {
                out << "\\n#" << pass._executionIndex;
            }

            out << "\"];\n";
        }

        out << "\n";

        for (const ResourceNode &node : _resourceNodes) {
            const ResourceEntry &entry = _resourceEntries[node._resourceEntryID.Get()];

            out << "    R" << node._resourceID.Get() << " [shape=ellipse, style=filled, fillcolor=\"" << (entry.IsImported() ? "#ffe5b4" : "#d7f2d7")
                << "\", label=\"" << _resourceNames[node._resourceID.Get()].View() << "\\nv" << node._version << "\"];\n";
        }

        out << "\n";

        for (const PassNode &pass : _passNodes) {
            for (u32 i = 0; i < pass._createCount; ++i) {
                out << "    P" << pass._passID.Get() << " -> R" << _creates[pass._createBegin + i].Get() << " [style=dashed, label=\"create\"];\n";
            }

            for (u32 i = 0; i < pass._writeCount; ++i) {
                const FrameGraphResourceID written = _writes[pass._writeBegin + i].Resource;

                // A created resource already has a dashed "create" arrow; the implied write would duplicate it.
                if (!passCreatesResource(pass, written)) {
                    out << "    P" << pass._passID.Get() << " -> R" << written.Get() << " [label=\"write\"];\n";
                }
            }

            for (u32 i = 0; i < pass._readCount; ++i) {
                out << "    R" << _reads[pass._readBegin + i].Resource.Get() << " -> P" << pass._passID.Get() << " [label=\"read\"];\n";
            }
        }

        out << "}\n";

        return out.str();
    }

} // namespace Vulkyrie
