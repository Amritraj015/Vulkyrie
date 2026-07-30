// The logger must never heap-allocate: the memory tracker's global operator new/delete override
// reports failures through it (VASSERT -> VERROR), and the shutdown memory report must not perturb
// the very counters it is printing. These tests pin that guarantee, using the memory tracker itself
// as the allocation detector, and cover the call-site prefix each message now carries.
#include "core/logger.h"

#include "core/log_formatting.h"

#include "memory/memory_tag.h"
#include "memory/memory_tracker.h"

#include <catch2/catch_test_macros.hpp>

#include <array>
#include <cstdint>
#include <string_view>

using namespace Vulkyrie;

namespace {

    [[nodiscard]] i64 TotalAllocatedAcrossAllTags() {
        i64 total = 0;
        for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
            total += MemoryTracker::TotalAllocated(static_cast<MemoryTag>(index));
        }
        return total;
    }

    [[nodiscard]] i64 LiveAllocationsAcrossAllTags() {
        i64 total = 0;
        for (std::uint32_t index = 0; index < MemoryTagCount; ++index) {
            total += MemoryTracker::LiveAllocations(static_cast<MemoryTag>(index));
        }
        return total;
    }

} // namespace

TEST_CASE("Console log sink formats and logs without heap allocation", "[core][logger]") {
    REQUIRE(Logger::InitializeLogger(LoggerType::Console) == StatusCode::Successful);

    // Warm-up: the first write to stdout may lazily set up libc stream buffers (malloc-based, so
    // invisible to the tracker anyway); keep it out of the measured region regardless.
    Logger::Log(LogLevel::Info, VE_LOG_SITE, "logger allocation test: warm-up");

    const i64 allocatedBefore = TotalAllocatedAcrossAllTags();
    const i64 liveBefore = LiveAllocationsAcrossAllTags();

    // A representative formatted message (mirrors the shutdown memory report's row format).
    Logger::Log(LogLevel::Info, VE_LOG_SITE, "{:<12}{:>14}{:>14}", std::string_view{ "Subsystem" }, i64{ 123456 }, i64{ 789 });

    // A message wider than the sink's 512-byte stack buffer, forcing the truncation path.
    Logger::Log(LogLevel::Info, VE_LOG_SITE, "{:>600}", std::string_view{ "(truncation test)" });

    REQUIRE(TotalAllocatedAcrossAllTags() == allocatedBefore);
    REQUIRE(LiveAllocationsAcrossAllTags() == liveBefore);
}

TEST_CASE("Log call sites render as \"<file> (<line>): \"", "[core][logger]") {
    // Resolved at compile time - a log call must not pay to walk the absolute path __FILE__ expands to.
    STATIC_REQUIRE(FileNameFromPath("/home/user/vulkyrie/engine/src/core/logger.cpp") == "logger.cpp");
    STATIC_REQUIRE(FileNameFromPath("C:\\vulkyrie\\engine\\logger.cpp") == "logger.cpp");
    STATIC_REQUIRE(FileNameFromPath("logger.cpp") == "logger.cpp");

    std::array<char, 64> buffer{};
    const std::size_t written = FormatSiteToBuffer(buffer.data(), buffer.size(), "logger.cpp", 42);

    REQUIRE(std::string_view(buffer.data(), written) == "logger.cpp (42): ");

    // A buffer too small to hold the site truncates rather than overrunning, matching how the message itself
    // behaves - the sinks share one bounded-write mechanism.
    std::array<char, 8> tiny{};
    const std::size_t clipped = FormatSiteToBuffer(tiny.data(), tiny.size(), "a_very_long_file_name.cpp", 1234);

    REQUIRE(clipped == tiny.size());
}
