#include "memory/memory_tracker.h"

#include "memory/callstack.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace Vulkyrie {

    namespace {

#if VE_MEMORY_DEEP_TRACKING

        /** @brief Shards the deep table so concurrent allocations on different threads rarely contend. A power of
         * two, because the shard index is a mask of the pointer hash. */
        constexpr std::size_t SHARD_COUNT = 64;

        static_assert((SHARD_COUNT & (SHARD_COUNT - 1)) == 0, "SHARD_COUNT must be a power of two for the index mask.");

        /** @brief Spreads pointer values across shards. Allocator-returned addresses are typically aligned and
         * clustered, so the low bits are nearly constant - mixing before masking is what keeps the shards balanced. */
        struct PointerHash {
            [[nodiscard]] std::size_t operator()(const void *pointer) const noexcept {
                auto value = reinterpret_cast<std::uintptr_t>(pointer);
                value ^= value >> 33U;
                value *= 0xFF51AFD7ED558CCDULL;
                value ^= value >> 33U;

                return static_cast<std::size_t>(value);
            }
        };

        template <typename TKey, typename TValue, typename THash>
        using UntrackedMap = std::unordered_map<TKey, TValue, THash, std::equal_to<TKey>, UntrackedAllocator<std::pair<const TKey, TValue>>>;

        /** @brief One stripe of the allocation table. */
        struct RecordShard {
            std::mutex Lock;
            UntrackedMap<const void *, AllocationRecord, PointerHash> Records;
        };

        /** @brief One stripe of the call-site table. */
        struct SiteShard {
            std::mutex Lock;
            UntrackedMap<std::uint64_t, AllocationSite, std::hash<std::uint64_t>> Sites;
        };

        struct DeepTable {
            std::array<RecordShard, SHARD_COUNT> Records;
            std::array<SiteShard, SHARD_COUNT> Sites;
            std::chrono::steady_clock::time_point Origin = std::chrono::steady_clock::now();
        };

        static_assert(alignof(DeepTable) <= alignof(std::max_align_t), "DeepTable must not be over-aligned; it is placed into malloc'd storage.");

        /** @brief Guards the tracker against re-entering itself. Nothing in the deep path should allocate through
         * `operator new` - the containers are untracked and callstack capture has its own guard - but a single
         * missed path would recurse until the stack ran out, so the invariant is enforced rather than assumed. */
        thread_local bool tInTracker = false;

        /** @brief RAII form of `tInTracker`. */
        class TrackerReentrancyGuard final {
        public:
            TrackerReentrancyGuard()
                : _entered{ !tInTracker } {
                tInTracker = true;
            }

            ~TrackerReentrancyGuard() {
                if (_entered) {
                    tInTracker = false;
                }
            }

            VE_DELETE_MOVE_AND_COPY(TrackerReentrancyGuard);

            /** @brief True when this guard is the outermost one, i.e. the caller may proceed. */
            [[nodiscard]] bool Entered() const {
                return _entered;
            }

        private:
            bool _entered;
        };

        /** @brief The deep table, deliberately never destroyed.
         *
         * Allocations made by other translation units' globals are freed during static destruction, in an order
         * this file cannot control. A table with a destructor would be gone by then and every late free would
         * touch freed memory. Leaking one fixed-size structure at exit is the correct trade. */
        [[nodiscard]] DeepTable &Table() {
            static auto *table = ::new (std::malloc(sizeof(DeepTable))) DeepTable{};
            return *table;
        }

        [[nodiscard]] std::size_t ShardIndex(std::size_t hash) {
            return hash & (SHARD_COUNT - 1);
        }

        [[nodiscard]] std::uint32_t CurrentThreadId() {
            return static_cast<std::uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
        }

#endif // VE_MEMORY_DEEP_TRACKING

    } // namespace

    // ===========================================================================================
    // Cheap tier
    // ===========================================================================================

    i64 MemoryTracker::CurrentBytes(MemoryTag tag) {
        return detail::CountersFor(tag).CurrentBytes.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::PeakBytes(MemoryTag tag) {
        return detail::CountersFor(tag).PeakBytes.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::LiveAllocations(MemoryTag tag) {
        return detail::CountersFor(tag).LiveAllocations.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::TotalAllocated(MemoryTag tag) {
        return detail::CountersFor(tag).TotalAllocated.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::TotalFreed(MemoryTag tag) {
        return detail::CountersFor(tag).TotalFreed.load(std::memory_order_relaxed);
    }

    // ===========================================================================================
    // Reserved pools
    // ===========================================================================================

    i64 MemoryTracker::PoolReservedBytes(MemoryTag tag) {
        return detail::CountersFor(tag).PoolReserved.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::PoolUsedBytes(MemoryTag tag) {
        return detail::CountersFor(tag).PoolUsed.load(std::memory_order_relaxed);
    }

    i64 MemoryTracker::PoolPeakUsedBytes(MemoryTag tag) {
        return detail::CountersFor(tag).PoolPeakUsed.load(std::memory_order_relaxed);
    }

    // ===========================================================================================
    // Deep tier
    // ===========================================================================================

    void MemoryTracker::OnAllocationDeep([[maybe_unused]] void *address, [[maybe_unused]] MemoryTag tag, [[maybe_unused]] i64 size) {
#if VE_MEMORY_DEEP_TRACKING
        if (address == nullptr) {
            return;
        }

        const TrackerReentrancyGuard guard;

        if (!guard.Entered()) {
            return;
        }

        DeepTable &table = Table();

        // Captured before taking any lock: the capture routine can be slow, and holding a shard while it runs
        // would serialize every allocating thread that hashes to the same stripe.
        const Callstack callstack = CaptureCallstack(2);

        const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - table.Origin).count();

        const AllocationRecord record{ .Address = address,
                                       .Size = static_cast<std::size_t>(size),
                                       .Tag = tag,
                                       .ThreadId = CurrentThreadId(),
                                       .TimestampNanos = static_cast<std::uint64_t>(elapsed),
                                       .SiteHash = callstack.Hash };

        {
            RecordShard &shard = table.Records[ShardIndex(PointerHash{}(address))];
            const std::scoped_lock lock{ shard.Lock };
            shard.Records[address] = record;
        }

        if (callstack.Hash != 0) {
            SiteShard &shard = table.Sites[ShardIndex(std::hash<std::uint64_t>{}(callstack.Hash))];
            const std::scoped_lock lock{ shard.Lock };

            AllocationSite &site = shard.Sites[callstack.Hash];

            if (site.Hash == 0) {
                site.Hash = callstack.Hash;
                site.Frames = callstack.Frames;
                site.FrameCount = callstack.FrameCount;
                site.Tag = tag;
            }

            site.LiveBytes += size;
            site.LiveCount += 1;
            site.TotalBytes += size;
            site.TotalCount += 1;
        }
#endif
    }

    void MemoryTracker::OnFreeDeep([[maybe_unused]] void *address) {
#if VE_MEMORY_DEEP_TRACKING
        if (address == nullptr) {
            return;
        }

        const TrackerReentrancyGuard guard;

        if (!guard.Entered()) {
            return;
        }

        DeepTable &table = Table();
        AllocationRecord record{};

        {
            RecordShard &shard = table.Records[ShardIndex(PointerHash{}(address))];
            const std::scoped_lock lock{ shard.Lock };

            const auto it = shard.Records.find(address);

            // Not an error: anything allocated before the table first came up is legitimately absent.
            if (it == shard.Records.end()) {
                return;
            }

            record = it->second;
            shard.Records.erase(it);
        }

        if (record.SiteHash != 0) {
            SiteShard &shard = table.Sites[ShardIndex(std::hash<std::uint64_t>{}(record.SiteHash))];
            const std::scoped_lock lock{ shard.Lock };

            const auto it = shard.Sites.find(record.SiteHash);

            if (it != shard.Sites.end()) {
                it->second.LiveBytes -= static_cast<i64>(record.Size);
                it->second.LiveCount -= 1;
            }
        }
#endif
    }

    UntrackedVector<AllocationRecord> MemoryTracker::LiveAllocationRecords() {
        UntrackedVector<AllocationRecord> records;

#if VE_MEMORY_DEEP_TRACKING
        const TrackerReentrancyGuard guard;
        DeepTable &table = Table();

        for (RecordShard &shard : table.Records) {
            const std::scoped_lock lock{ shard.Lock };

            for (const auto &[address, record] : shard.Records) {
                records.push_back(record);
            }
        }
#endif

        return records;
    }

    LeakSummary MemoryTracker::CollectLeakSummary() {
        LeakSummary summary{};

#if VE_MEMORY_DEEP_TRACKING
        const TrackerReentrancyGuard guard;
        DeepTable &table = Table();

        for (RecordShard &shard : table.Records) {
            const std::scoped_lock lock{ shard.Lock };

            for (const auto &[address, record] : shard.Records) {
                summary.Count += 1;
                summary.Bytes += static_cast<i64>(record.Size);
                summary.BytesByTag[static_cast<std::size_t>(record.Tag)] += static_cast<i64>(record.Size);
            }
        }
#endif

        return summary;
    }

    UntrackedVector<AllocationSite> MemoryTracker::TopAllocationSites([[maybe_unused]] std::size_t limit) {
        UntrackedVector<AllocationSite> sites;

#if VE_MEMORY_DEEP_TRACKING
        const TrackerReentrancyGuard guard;
        DeepTable &table = Table();

        for (SiteShard &shard : table.Sites) {
            const std::scoped_lock lock{ shard.Lock };

            for (const auto &[hash, site] : shard.Sites) {
                if (site.LiveBytes > 0) {
                    sites.push_back(site);
                }
            }
        }

        std::ranges::sort(sites, std::ranges::greater{}, &AllocationSite::LiveBytes);

        if (sites.size() > limit) {
            sites.resize(limit);
        }
#endif

        return sites;
    }

    void MemoryTracker::ReportToLog() {
        // Guard the whole body so release builds (where VINFO compiles out) don't warn on unused locals.
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_INFO_LEVEL_LOG
        VINFO("================= Memory report (per subsystem) =================");
        VINFO("{:<12}{:>14}{:>14}{:>10}{:>16}{:>16}", "Subsystem", "Current(B)", "Peak(B)", "Live", "TotalAlloc(B)", "TotalFreed(B)");

        for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
            const auto tag = static_cast<MemoryTag>(index);
            VINFO("{:<12}{:>14}{:>14}{:>10}{:>16}{:>16}",
                  MemoryTagName(tag),
                  CurrentBytes(tag),
                  PeakBytes(tag),
                  LiveAllocations(tag),
                  TotalAllocated(tag),
                  TotalFreed(tag));
        }

        // Only worth the lines when something actually uses a toolkit allocator.
        bool anyPools = false;

        for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
            if (PoolReservedBytes(static_cast<MemoryTag>(index)) != 0 || PoolPeakUsedBytes(static_cast<MemoryTag>(index)) != 0) {
                anyPools = true;
                break;
            }
        }

        if (anyPools) {
            VINFO("--------------- Reserved pools (allocator toolkit) --------------");
            VINFO("{:<12}{:>16}{:>16}{:>16}", "Subsystem", "Reserved(B)", "InUse(B)", "PeakInUse(B)");

            for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
                const auto tag = static_cast<MemoryTag>(index);

                if (PoolReservedBytes(tag) == 0 && PoolPeakUsedBytes(tag) == 0) {
                    continue;
                }

                VINFO("{:<12}{:>16}{:>16}{:>16}", MemoryTagName(tag), PoolReservedBytes(tag), PoolUsedBytes(tag), PoolPeakUsedBytes(tag));
            }
        }

        VINFO("================================================================");
#endif
    }

    void MemoryTracker::ReportOutstandingToLog([[maybe_unused]] std::size_t maxSites) {
#if VULKYRIE_LOG_LEVEL >= VULKYRIE_INFO_LEVEL_LOG
        if constexpr (!DeepTrackingEnabled()) {
            VINFO("Deep memory tracking is disabled; build with VE_MEMORY_DEEP_TRACKING=1 for outstanding-allocation detail.");
            return;
        }

        const LeakSummary summary = CollectLeakSummary();

        VINFO("=============== Outstanding allocations at report ===============");
        VINFO("{} allocation(s), {} byte(s) still live.", summary.Count, summary.Bytes);

        for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
            const i64 bytes = summary.BytesByTag[index];

            if (bytes != 0) {
                VINFO("  {:<12}{:>16} B", MemoryTagName(static_cast<MemoryTag>(index)), bytes);
            }
        }

        if constexpr (!CallstacksEnabled()) {
            VINFO("Call sites unavailable; rebuild with VE_MEMORY_CALLSTACKS=1 to attribute these to source locations.");
            VINFO("================================================================");
            return;
        }

        const UntrackedVector<AllocationSite> sites = TopAllocationSites(maxSites);

        if (!sites.empty()) {
            VINFO("--- Top {} call site(s) by outstanding bytes ---", sites.size());

            for (const AllocationSite &site : sites) {
                Callstack callstack{};
                callstack.Frames = site.Frames;
                callstack.FrameCount = site.FrameCount;
                callstack.Hash = site.Hash;

                VINFO("{} B in {} allocation(s), tag {}: {}", site.LiveBytes, site.LiveCount, MemoryTagName(site.Tag), FormatCallstack(callstack));
            }
        }

        VINFO("================================================================");
#endif
    }

} // namespace Vulkyrie
