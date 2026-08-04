#include "memory_test_support.h"

#include <catch2/catch_test_macros.hpp>
#include <memory/callstack.h>
#include <memory/memory_tracker.h>

#include <algorithm>
#include <atomic>
#include <thread>
#include <vector>

using namespace Vulkyrie;
using namespace Vulkyrie::MemoryTests;

namespace {

    /** @brief Returns the record for a specific address, or nullopt if the deep table does not hold it. */
    [[nodiscard]] std::optional<AllocationRecord> FindRecord(const void *address) {
        const UntrackedVector<AllocationRecord> records = MemoryTracker::LiveAllocationRecords();
        const auto it = std::ranges::find(records, address, &AllocationRecord::Address);

        if (it == records.end()) {
            return std::nullopt;
        }

        return *it;
    }

} // namespace

TEST_CASE("MemoryTracker - Deep tracking is on in debug builds", "[memory][deep]") {
    // The whole deep tier is conditional, so every test below is only meaningful when it is compiled in. This
    // documents which side of the switch this build is on rather than asserting one.
    INFO("deep tracking: " << MemoryTracker::DeepTrackingEnabled() << ", callstacks: " << CallstacksEnabled());

#if defined(VULKYRIE_DEBUG)
    REQUIRE(MemoryTracker::DeepTrackingEnabled());
#endif

    SUCCEED();
}

TEST_CASE("MemoryTracker - An allocation enters and leaves the deep table", "[memory][deep]") {
    if (!MemoryTracker::DeepTrackingEnabled() || !GlobalNewOverrideActive()) {
        SUCCEED("Deep tracking or the global new override is disabled in this build.");
        return;
    }

    // A size no other allocation in the suite is likely to share, so the record is unambiguous.
    constexpr std::size_t DISTINCTIVE_SIZE = 4099;

    std::byte *block = nullptr;

    {
        VE_MEMORY_SCOPE(MemoryTag::Platform);
        block = new std::byte[DISTINCTIVE_SIZE];
    }

    const std::optional<AllocationRecord> record = FindRecord(block);

    REQUIRE(record.has_value());
    REQUIRE(record->Size >= DISTINCTIVE_SIZE);
    REQUIRE(record->Tag == MemoryTag::Platform); // The record carries the scope in force at allocation time.
    REQUIRE(record->ThreadId != 0);

    delete[] block;

    REQUIRE_FALSE(FindRecord(block).has_value());
}

TEST_CASE("MemoryTracker - Outstanding allocations are detectable as a leak", "[memory][deep]") {
    if (!MemoryTracker::DeepTrackingEnabled() || !GlobalNewOverrideActive()) {
        SUCCEED("Deep tracking or the global new override is disabled in this build.");
        return;
    }

    constexpr std::size_t LEAK_SIZE = 8192;
    constexpr auto TAG = MemoryTag::ThirdParty;
    constexpr std::size_t TAG_INDEX = static_cast<std::size_t>(TAG);

    const LeakSummary before = MemoryTracker::CollectLeakSummary();

    std::byte *leaked = nullptr;

    {
        VE_MEMORY_SCOPE(TAG);
        leaked = new std::byte[LEAK_SIZE];
    }

    const LeakSummary during = MemoryTracker::CollectLeakSummary();

    REQUIRE(during.Count == before.Count + 1);
    REQUIRE(during.Bytes >= before.Bytes + static_cast<i64>(LEAK_SIZE));
    REQUIRE(during.BytesByTag[TAG_INDEX] >= before.BytesByTag[TAG_INDEX] + static_cast<i64>(LEAK_SIZE));

    // Reclaiming it must take the table back to exactly where it started - this is what a leak gate asserts.
    delete[] leaked;

    const LeakSummary after = MemoryTracker::CollectLeakSummary();

    REQUIRE(after.Count == before.Count);
    REQUIRE(after.BytesByTag[TAG_INDEX] == before.BytesByTag[TAG_INDEX]);
}

TEST_CASE("MemoryTracker - Deep records agree with the cheap counters", "[memory][deep]") {
    if (!MemoryTracker::DeepTrackingEnabled() || !GlobalNewOverrideActive()) {
        SUCCEED("Deep tracking or the global new override is disabled in this build.");
        return;
    }

    constexpr auto TAG = MemoryTag::Editor;
    constexpr std::size_t ALLOCATION_COUNT = 64;
    constexpr std::size_t BLOCK_SIZE = 512;

    // Reserved before the baseline is taken: the vector's own buffer is an allocation too, and it must be on the
    // same side of the measurement as everything else that is not under test.
    std::vector<std::byte *> blocks;
    blocks.reserve(ALLOCATION_COUNT);

    const i64 liveBefore = MemoryTracker::LiveAllocations(TAG);
    const i64 bytesBefore = MemoryTracker::CurrentBytes(TAG);
    const LeakSummary deepBefore = MemoryTracker::CollectLeakSummary();

    {
        VE_MEMORY_SCOPE(TAG);

        for (std::size_t i = 0; i < ALLOCATION_COUNT; ++i) {
            blocks.push_back(new std::byte[BLOCK_SIZE]);
        }
    }

    // The two tiers are independent code paths over the same events; they must not disagree.
    REQUIRE(MemoryTracker::LiveAllocations(TAG) == liveBefore + static_cast<i64>(ALLOCATION_COUNT));
    REQUIRE(MemoryTracker::CurrentBytes(TAG) >= bytesBefore + static_cast<i64>(ALLOCATION_COUNT * BLOCK_SIZE));
    REQUIRE(MemoryTracker::CollectLeakSummary().Count == deepBefore.Count + static_cast<i64>(ALLOCATION_COUNT));

    for (std::byte *block : blocks) {
        delete[] block;
    }

    REQUIRE(MemoryTracker::LiveAllocations(TAG) == liveBefore);
    REQUIRE(MemoryTracker::CurrentBytes(TAG) == bytesBefore);
    REQUIRE(MemoryTracker::CollectLeakSummary().Count == deepBefore.Count);
}

TEST_CASE("MemoryTracker - Deep table reconciles under concurrent allocation", "[memory][deep]") {
    if (!MemoryTracker::DeepTrackingEnabled() || !GlobalNewOverrideActive()) {
        SUCCEED("Deep tracking or the global new override is disabled in this build.");
        return;
    }

    constexpr auto TAG = MemoryTag::Input;
    constexpr std::size_t THREAD_COUNT = 8;
    constexpr std::size_t PER_THREAD = 256;

    const LeakSummary before = MemoryTracker::CollectLeakSummary();
    const i64 liveBefore = MemoryTracker::LiveAllocations(TAG);

    std::atomic<i64> observedRecords{ 0 };

    // Scoped so the thread vector and every thread's internal state are gone before the table is re-measured;
    // otherwise the harness's own allocations show up as a one-record discrepancy.
    {
        std::vector<std::thread> threads;
        threads.reserve(THREAD_COUNT);

        for (std::size_t t = 0; t < THREAD_COUNT; ++t) {
            threads.emplace_back([&observedRecords, t] {
                VE_MEMORY_SCOPE(TAG);

                std::vector<std::byte *> local;
                local.reserve(PER_THREAD);

                for (std::size_t i = 0; i < PER_THREAD; ++i) {
                    // Varying sizes so the shards see a spread of addresses rather than one size class.
                    local.push_back(new std::byte[64 + ((i + t) % 97)]);
                }

                observedRecords.fetch_add(static_cast<i64>(local.size()), std::memory_order_relaxed);

                for (std::byte *block : local) {
                    delete[] block;
                }
            });
        }

        for (std::thread &thread : threads) {
            thread.join();
        }
    }

    REQUIRE(observedRecords.load() == static_cast<i64>(THREAD_COUNT * PER_THREAD));

    const LeakSummary after = MemoryTracker::CollectLeakSummary();

    // Every insert had a matching erase, from a different thread's stripe in many cases.
    REQUIRE(after.Count == before.Count);
    REQUIRE(after.Bytes == before.Bytes);
    REQUIRE(MemoryTracker::LiveAllocations(TAG) == liveBefore);
    REQUIRE(MemoryTracker::CurrentBytes(TAG) >= 0);
}

TEST_CASE("MemoryTracker - Top allocation sites reflect the callstack build flag", "[memory][deep]") {
    if (!MemoryTracker::DeepTrackingEnabled() || !GlobalNewOverrideActive()) {
        SUCCEED("Deep tracking or the global new override is disabled in this build.");
        return;
    }

    std::byte *block = nullptr;

    {
        VE_MEMORY_SCOPE(MemoryTag::Audio);
        block = new std::byte[16384];
    }

    const UntrackedVector<AllocationSite> sites = MemoryTracker::TopAllocationSites(8);

    if (CallstacksEnabled()) {
        // With capture on, the allocation above must be attributable to some site.
        REQUIRE_FALSE(sites.empty());
        REQUIRE(sites.front().LiveBytes > 0);

        // Sites come back largest-first, which is the only ordering a leak hunt cares about.
        for (std::size_t i = 1; i < sites.size(); ++i) {
            REQUIRE(sites[i - 1].LiveBytes >= sites[i].LiveBytes);
        }
    } else {
        // Without capture there is nothing to group by, and the API says so by returning nothing rather than
        // inventing a single "unknown" bucket that would just restate the per-tag counters.
        REQUIRE(sites.empty());
    }

    delete[] block;
}

TEST_CASE("MemoryTracker - Callstack formatting is safe on an empty stack", "[memory][deep]") {
    const Callstack empty{};

    REQUIRE(FormatCallstack(empty).empty());

    const Callstack captured = CaptureCallstack();

    if (CallstacksEnabled()) {
        REQUIRE(captured.FrameCount > 0);
        REQUIRE(captured.Hash != 0);
        REQUIRE_FALSE(FormatCallstack(captured).empty());
    } else {
        REQUIRE(captured.FrameCount == 0);
        REQUIRE(captured.Hash == 0);
    }
}
