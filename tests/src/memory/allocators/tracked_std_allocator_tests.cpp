#include "../memory_test_support.h"

#include <catch2/catch_test_macros.hpp>
#include <memory/allocators/tracked_std_allocator.h>
#include <memory/allocators/untracked_allocator.h>
#include <memory/memory_tracker.h>

#include <string>
#include <vector>

using namespace Vulkyrie;
using namespace Vulkyrie::MemoryTests;

TEST_CASE("TrackedStdAllocator - Container storage is charged to the allocator's tag", "[memory][allocator][std]") {
    constexpr auto TAG = MemoryTag::Platform;
    constexpr std::size_t ELEMENTS = 4096;

    const i64 taggedBefore = MemoryTracker::TotalAllocated(TAG);
    const i64 ambientBefore = MemoryTracker::TotalAllocated(MemoryTag::Assets);

    {
        // Deliberately under a *different* ambient scope. Scope-based attribution would charge this to Assets;
        // the typed allocator charges it to Platform, which is the whole reason the type exists.
        VE_MEMORY_SCOPE(MemoryTag::Assets);

        TrackedVector<u64, TAG> values;
        values.reserve(ELEMENTS);

        for (u64 i = 0; i < ELEMENTS; ++i) {
            values.push_back(i);
        }

        REQUIRE(values.size() == ELEMENTS);
        REQUIRE(values.back() == ELEMENTS - 1);
    }

    if (!GlobalNewOverrideActive()) {
        SUCCEED("Global new override disabled in this build; attribution cannot be observed.");
        return;
    }

    const i64 taggedGrowth = MemoryTracker::TotalAllocated(TAG) - taggedBefore;
    const i64 ambientGrowth = MemoryTracker::TotalAllocated(MemoryTag::Assets) - ambientBefore;

    REQUIRE(taggedGrowth >= static_cast<i64>(ELEMENTS * sizeof(u64)));
    REQUIRE(ambientGrowth < static_cast<i64>(ELEMENTS * sizeof(u64)));

    // The storage is ordinary tracked heap, so it balances out on destruction like anything else.
    REQUIRE(MemoryTracker::TotalFreed(TAG) >= static_cast<i64>(ELEMENTS * sizeof(u64)));
}

TEST_CASE("TrackedStdAllocator - Node-based containers rebind correctly", "[memory][allocator][std]") {
    constexpr auto TAG = MemoryTag::ThirdParty;

    const i64 before = MemoryTracker::TotalAllocated(TAG);

    {
        // unordered_map allocates its node type, not the pair, so this only compiles and behaves if rebind
        // carries the tag across.
        TrackedUnorderedMap<i32, std::string, TAG> map;

        for (i32 i = 0; i < 256; ++i) {
            map.emplace(i, "value" + std::to_string(i));
        }

        REQUIRE(map.size() == 256);
        REQUIRE(map.at(42) == "value42");
        REQUIRE(map.contains(255));
        REQUIRE_FALSE(map.contains(256));

        if (GlobalNewOverrideActive()) {
            REQUIRE(MemoryTracker::TotalAllocated(TAG) > before);
        }
    }
}

TEST_CASE("TrackedStdAllocator - Tracked strings behave like std::string", "[memory][allocator][std]") {
    constexpr auto TAG = MemoryTag::Core;

    TrackedString<TAG> text = "frame graph";
    text += " allocator";

    REQUIRE(text == "frame graph allocator");
    REQUIRE(text.size() == 21);

    // Long enough to defeat the small-string buffer and actually reach the allocator.
    const i64 before = MemoryTracker::TotalAllocated(TAG);
    TrackedString<TAG> big(4096, 'x');

    REQUIRE(big.size() == 4096);
    REQUIRE(big.front() == 'x');
    REQUIRE(big.back() == 'x');

    if (GlobalNewOverrideActive()) {
        REQUIRE(MemoryTracker::TotalAllocated(TAG) > before);
    }
}

TEST_CASE("UntrackedAllocator - Storage bypasses the tracker entirely", "[memory][allocator][std]") {
    constexpr std::size_t ELEMENTS = 8192;

    const i64 before = MemoryTracker::TotalAllocated(MemoryTag::Untagged);
    const i64 physicsBefore = MemoryTracker::TotalAllocated(MemoryTag::Physics);

    {
        VE_MEMORY_SCOPE(MemoryTag::Physics);

        std::vector<u64, UntrackedAllocator<u64>> values;
        values.reserve(ELEMENTS);

        for (u64 i = 0; i < ELEMENTS; ++i) {
            values.push_back(i);
        }

        REQUIRE(values.size() == ELEMENTS);
        REQUIRE(values.back() == ELEMENTS - 1);
    }

    // Invisible to every bucket - this is what keeps the tracker's own bookkeeping from being recorded by the
    // tracker, and from recursing while it does so.
    REQUIRE(MemoryTracker::TotalAllocated(MemoryTag::Untagged) - before < static_cast<i64>(ELEMENTS * sizeof(u64)));
    REQUIRE(MemoryTracker::TotalAllocated(MemoryTag::Physics) - physicsBefore < static_cast<i64>(ELEMENTS * sizeof(u64)));
}

TEST_CASE("UntrackedAllocator - Over-aligned types are honoured", "[memory][allocator][std]") {
    struct alignas(64) Wide {
        u64 Values[8];
    };

    UntrackedAllocator<Wide> allocator;
    Wide *block = allocator.allocate(4);

    REQUIRE(block != nullptr);
    REQUIRE(reinterpret_cast<uintptr_t>(block) % 64 == 0);

    allocator.deallocate(block, 4);
}
