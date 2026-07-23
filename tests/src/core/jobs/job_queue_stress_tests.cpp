// White-box exactly-once stress of the private Chase-Lev deque (hence the engine/src include path
// in tests/CMakeLists.txt). ThreadSanitizer proves the deque's relaxed atomics are race-free, but
// it does not model the two standalone seq_cst fences the algorithm's *ordering* rests on — a
// clean TSan run is weak evidence for the deque specifically. This test checks the contract that
// actually matters: every pushed value is taken exactly once, by the owner or by a thief, with
// none lost and none duplicated.
#include "core/jobs/job_queue.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <thread>
#include <vector>

using namespace Vulkyrie;

TEST_CASE("JobQueue delivers every pushed value exactly once under owner/thief contention", "[jobs]") {
    constexpr u64 kItems = 2'000'000;
    constexpr u32 kThieves = 4;

    JobQueue queue(1024);
    std::atomic<bool> stop{ false };
    std::vector<std::vector<u64>> stolenPerThief(kThieves);
    std::vector<u64> ownerTaken;

    {
        std::vector<std::jthread> thieves;
        thieves.reserve(kThieves);
        for (u32 t = 0; t < kThieves; ++t) {
            thieves.emplace_back([&queue, &stop, &stolenPerThief, t] {
                std::vector<u64> &stolen = stolenPerThief[t];
                while (!stop.load(std::memory_order_relaxed)) {
                    if (const auto value = queue.TrySteal()) {
                        stolen.push_back(*value);
                    } else {
                        std::this_thread::yield();
                    }
                }
                // Drain whatever is left after the owner stops producing.
                while (const auto value = queue.TrySteal()) {
                    stolen.push_back(*value);
                }
            });
        }

        for (u64 i = 0; i < kItems; ++i) {
            // On a full queue the owner pops (mirroring the scheduler's inline fallback pressure).
            while (!queue.TryPush(i)) {
                if (const auto value = queue.TryPop()) {
                    ownerTaken.push_back(*value);
                }
            }
            // Interleave owner pops with pushes to exercise the bottom/top race paths.
            if ((i & 1U) == 0U) {
                if (const auto value = queue.TryPop()) {
                    ownerTaken.push_back(*value);
                }
            }
        }

        while (const auto value = queue.TryPop()) {
            ownerTaken.push_back(*value);
        }

        stop.store(true, std::memory_order_relaxed);
    } // jthreads join here; each thief drains before exiting.

    // Post-join sweep in case the last element was abandoned in a lost pop/steal race.
    while (const auto value = queue.TryPop()) {
        ownerTaken.push_back(*value);
    }

    std::vector<u8> seen(kItems, 0);
    u64 duplicated = 0;
    for (const u64 value : ownerTaken) {
        if (seen[value]++ != 0) {
            ++duplicated;
        }
    }
    for (const std::vector<u64> &stolen : stolenPerThief) {
        for (const u64 value : stolen) {
            if (seen[value]++ != 0) {
                ++duplicated;
            }
        }
    }

    u64 lost = 0;
    for (u64 i = 0; i < kItems; ++i) {
        if (seen[i] == 0) {
            ++lost;
        }
    }

    REQUIRE(lost == 0);
    REQUIRE(duplicated == 0);
}
