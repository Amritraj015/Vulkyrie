#pragma once

#include "vlkypch.h"
#include "core/asserts.h"
#include "core/types/static_string.h"
#include "renderer/frame_graph/frame_graph_types.h"
#include "renderer/frame_graph/pass_node.h"
#include "renderer/frame_graph/resource_node.h"
#include "renderer/rhi/resource_types.h"

namespace Vulkyrie {

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

    namespace detail {

        /** @brief Everything `Compile` needs to know about one resource entry, with none of the type erasure.
         *
         * Split out of the entry itself so the compile walks scan a packed 24-byte POD instead of dragging six
         * trampoline pointers through cache for data they never read - and, more importantly, so the whole compile
         * pipeline can stay non-template while the resource objects it plans for are backend-typed. */
        struct EntryState final {
            /** @brief Execution-order position of the first pass that touches this resource, or `~0U` if unused.
             * Together with `LastUse` this is the interval the aliasing allocator packs. */
            u32 FirstUse = std::numeric_limits<u32>::max();

            /** @brief Execution-order position of the last pass that touches this resource, or `~0U` if unused.
             * The resource is released after that pass. */
            u32 LastUse = std::numeric_limits<u32>::max();

            /** @brief Offset the byte-packing aliasing plan assigned; meaningful only when `IsAliased` is set. */
            u64 AliasOffset = 0;

            /** @brief Bumped by each write, so stale handles can be detected. */
            u32 Version = 1;

            /** @brief Whether the graph owns this resource's lifetime, as opposed to it being imported. */
            bool IsTransient = true;

            /** @brief Whether the aliasing plan placed this resource. */
            bool IsAliased = false;

            /** @brief Returns the interval this resource is live over, for the transient pool. */
            [[nodiscard]] VE_INLINE ResourceLifetime Lifetime() const noexcept {
                return ResourceLifetime{ .FirstUse = FirstUse, .LastUse = LastUse };
            }

            /** @brief Returns where the byte-packing plan put this resource, for types taking the placed `Acquire`. */
            [[nodiscard]] VE_INLINE ResourcePlacement Placement() const noexcept {
                return ResourcePlacement{ .Offset = AliasOffset, .IsAliased = IsAliased };
            }
        };

        /** @brief The backend-agnostic half of a frame graph: topology, culling, ordering, lifetimes, the aliasing
         * plan and the barrier batches. Works entirely in ids - it never sees a resource object, a pass body or a
         * backend, which is what lets it stay a single non-template translation unit instead of being instantiated
         * once per backend.
         *
         * Not a public API. `FrameGraph<B>` owns one of these and is the only thing that talks to it. */
        class FrameGraphCore final {
        public:
            /** @brief One newly declared resource: the entry backing it and its first version node. */
            struct DeclaredResource final {
                FrameGraphResourceEntryID EntryID{};
                FrameGraphResourceID ResourceID{};
            };

            explicit FrameGraphCore(const FrameGraphConfig &config);

            VE_DELETE_MOVE_AND_COPY(FrameGraphCore);

            ~FrameGraphCore() = default;

            // --- Construction ---------------------------------------------------------------------------------

            /** @brief Appends a pass node whose access ranges start at the current end of the graph-level arrays. */
            [[nodiscard]] PassNode &CreatePassNode(StaticString name);

            /** @brief Creates a resource entry and its first version node.
             * @param isTransient Whether the graph owns the resource's lifetime.
             * @param name A human-readable identifier, stored by pointer.
             * @param producer The pass producing the resource, or an invalid id when imported. */
            [[nodiscard]] DeclaredResource DeclareResource(bool isTransient, StaticString name, FrameGraphPassID producer);

            /** @brief Records that a pass materializes a resource, adding it to the pass's create range. */
            void RegisterCreate(PassNode &passNode, FrameGraphResourceID resourceID);

            /** @brief Records a write, merging into an existing access if the pass already writes the resource. */
            FrameGraphResourceID RegisterWrite(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage);

            /** @brief Registers a read for a pass, asserting the handle is not stale. */
            [[nodiscard]] FrameGraphResourceID ReadImpl(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage);

            /** @brief Registers a write for a pass, renaming the resource when the pass did not create it. */
            [[nodiscard]] FrameGraphResourceID WriteImpl(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage);

            // --- Compilation ----------------------------------------------------------------------------------

            /** @brief Culls, orders and analyses the graph.
             * @param requirements One entry per resource entry, in entry-id order, as reported by the resource
             * types. A `Size` of zero means the resource stays out of the byte-packing plan - which is what a type
             * that does not implement `GetMemoryRequirements` reports, and what every resource reports on a backend
             * that cannot bind two resources to one allocation. */
            void Compile(std::span<const ResourceMemoryRequirements> requirements);

            /** @brief Returns the graph to empty while keeping every buffer's capacity. */
            void Reset();

            // --- Queries --------------------------------------------------------------------------------------

            [[nodiscard]] VE_INLINE bool IsCompiled() const noexcept {
                return mCompiled;
            }

            [[nodiscard]] VE_INLINE bool InSetup() const noexcept {
                return mInSetup;
            }

            VE_INLINE void SetInSetup(bool inSetup) noexcept {
                mInSetup = inSetup;
            }

            [[nodiscard]] VE_INLINE std::span<const u32> GetExecutionOrder() const noexcept {
                return mExecutionOrder;
            }

            [[nodiscard]] VE_INLINE const FrameGraphAliasingReport &GetAliasingReport() const noexcept {
                return mAliasingReport;
            }

            [[nodiscard]] VE_INLINE size_t GetPassCount() const noexcept {
                return mPassNodes.size();
            }

            [[nodiscard]] VE_INLINE size_t GetResourceVersionCount() const noexcept {
                return mResourceNodes.size();
            }

            [[nodiscard]] VE_INLINE size_t GetEntryCount() const noexcept {
                return mEntryStates.size();
            }

            /** @brief Returns the compile-time state of one resource entry. */
            [[nodiscard]] VE_INLINE const EntryState &GetEntryState(FrameGraphResourceEntryID entryID) const {
                VASSERT(entryID.IsValid() && entryID.Get() < mEntryStates.size(), "Resource entry ID is out of range.");

                return mEntryStates[entryID.Get()];
            }

            /** @brief Returns the entry backing a resource version. */
            [[nodiscard]] VE_INLINE FrameGraphResourceEntryID GetEntryID(FrameGraphResourceID resourceID) const {
                VASSERT(resourceID.IsValid() && resourceID.Get() < mResourceNodes.size(), "Resource ID is out of range.");

                return mResourceNodes[resourceID.Get()].mResourceEntryID;
            }

            /** @brief Returns the resources one pass creates. */
            [[nodiscard]] VE_INLINE std::span<const FrameGraphResourceID> GetCreates(u32 passIndex) const {
                const PassNode &pass = mPassNodes[passIndex];

                return { mCreates.data() + pass.mCreateBegin, pass.mCreateCount };
            }

            /** @brief Returns one pass's read accesses. */
            [[nodiscard]] VE_INLINE std::span<const ResourceAccess> GetReads(u32 passIndex) const {
                const PassNode &pass = mPassNodes[passIndex];

                return { mReads.data() + pass.mReadBegin, pass.mReadCount };
            }

            /** @brief Returns one pass's write accesses. */
            [[nodiscard]] VE_INLINE std::span<const ResourceAccess> GetWrites(u32 passIndex) const {
                const PassNode &pass = mPassNodes[passIndex];

                return { mWrites.data() + pass.mWriteBegin, pass.mWriteCount };
            }

            /** @brief Returns the entries released after one pass. */
            [[nodiscard]] VE_INLINE std::span<const FrameGraphResourceEntryID> GetReleases(u32 passIndex) const {
                const PassNode &pass = mPassNodes[passIndex];

                return { mReleases.data() + pass.mReleaseBegin, pass.mReleaseCount };
            }

            /** @brief Returns the transitions to emit before one pass. */
            [[nodiscard]] VE_INLINE std::span<const ResourceBarrier> GetBarriers(u32 passIndex) const {
                const PassNode &pass = mPassNodes[passIndex];

                return { mBarriers.data() + pass.mBarrierBegin, pass.mBarrierCount };
            }

            [[nodiscard]] VE_INLINE const PassNode &GetPassNode(FrameGraphPassID passID) const {
                VASSERT(passID.IsValid() && passID.Get() < mPassNodes.size(), "Pass ID is out of range.");

                return mPassNodes[passID.Get()];
            }

            [[nodiscard]] VE_INLINE const ResourceNode &GetResourceNode(FrameGraphResourceID resourceID) const {
                VASSERT(resourceID.IsValid() && resourceID.Get() < mResourceNodes.size(), "Resource ID is out of range.");

                return mResourceNodes[resourceID.Get()];
            }

            [[nodiscard]] VE_INLINE StaticString GetPassName(FrameGraphPassID passID) const {
                VASSERT(passID.IsValid() && passID.Get() < mPassNames.size(), "Pass ID is out of range.");

                return mPassNames[passID.Get()];
            }

            [[nodiscard]] VE_INLINE StaticString GetResourceName(FrameGraphResourceID resourceID) const {
                VASSERT(resourceID.IsValid() && resourceID.Get() < mResourceNames.size(), "Resource ID is out of range.");

                return mResourceNames[resourceID.Get()];
            }

            /** @brief Renders the compiled graph as a GraphViz DOT document. */
            [[nodiscard]] std::string ToDot() const;

            /** @brief Asserts that a pass's accesses form one contiguous run in each graph-level array. They do,
             * because a pass's setup runs to completion before the next begins - this checks that invariant rather
             * than assuming it. */
            void AssertContiguousRanges(const PassNode &passNode) const;

        private:
            /** @brief Appends a resource version node and links it to the previous version of the same entry. */
            [[nodiscard]] FrameGraphResourceID createResourceNode(StaticString name, FrameGraphResourceEntryID entryID, u32 version, FrameGraphPassID producer);

            /** @brief Records a read, merging into an existing access if the pass already reads the resource. */
            FrameGraphResourceID registerRead(PassNode &passNode, FrameGraphResourceID resourceID, const ResourceUsage &usage);

            /** @brief Produces the next version of a resource, so two passes writing it are forced into an order. */
            [[nodiscard]] FrameGraphResourceID createNextVersion(FrameGraphResourceID resourceID, FrameGraphPassID producer);

            /** @brief Checks that a handle refers to the current version of its resource. */
            [[nodiscard]] bool isCurrentVersion(FrameGraphResourceID resourceID) const;

            /** @brief Returns whether a pass creates the given resource, scanning only that pass's create range. */
            [[nodiscard]] bool passCreatesResource(const PassNode &passNode, FrameGraphResourceID resourceID) const;

            void computeReferenceCounts();
            void cullUnreferencedPasses();
            void buildExecutionOrder();
            void computeResourceLifetimes();
            void buildAliasingPlan(std::span<const ResourceMemoryRequirements> requirements);
            void buildBarriers();

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

            std::vector<PassNode> mPassNodes;
            std::vector<ResourceNode> mResourceNodes;

            /** @brief Compile-time state of every resource entry, indexed by `FrameGraphResourceEntryID`. The
             * resource objects themselves live in the typed facade at the same indices. */
            std::vector<EntryState> mEntryStates;

            /** @brief For each resource version, the node holding the next version of the same entry, or an invalid
             * id. Lets `Compile` derive write-after-read edges without a per-entry search. */
            std::vector<FrameGraphResourceID> mNextVersion;

            std::vector<FrameGraphResourceID> mCreates;
            std::vector<ResourceAccess> mReads;
            std::vector<ResourceAccess> mWrites;
            std::vector<FrameGraphResourceEntryID> mReleases;
            std::vector<ResourceBarrier> mBarriers;
            std::vector<u32> mExecutionOrder;

            std::vector<u32> mInDegrees;
            std::vector<u32> mReadyPasses;
            std::vector<std::pair<u32, u32>> mEdges;
            std::vector<FrameGraphResourceID> mCullWorklist;
            std::vector<ResourceUsage> mCurrentUsages;
            std::vector<u32> mReleaseCounts;
            std::vector<FrameGraphResourceID> mLatestVersionOfEntry;
            std::vector<AliasCandidate> mAliasCandidates;
            std::vector<u32> mAliasPlacements;
            std::vector<i64> mAliasLiveDelta;
            std::vector<FrameGraphResourceEntryID> mAliasPredecessors;
            std::vector<u32> mAliasPredecessorBegin;
            std::vector<u32> mAliasPredecessorCount;

            std::vector<StaticString> mPassNames;
            std::vector<StaticString> mResourceNames;

            FrameGraphAliasingReport mAliasingReport;

            bool mCompiled = false;
            bool mInSetup = false;
        };

    } // namespace detail

} // namespace Vulkyrie
