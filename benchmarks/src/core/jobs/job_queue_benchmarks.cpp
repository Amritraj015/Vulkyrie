// The Chase-Lev deque on its own, with the scheduler stripped away (hence the engine/src include
// path in benchmarks/CMakeLists.txt — the deque is a private header). These numbers are the floor
// under every job-system number: whatever a push/pop pair costs here is paid by every job that is
// ever queued, so a regression that shows up in dispatch overhead can be attributed here or ruled
// out from here.
#include "support/benchmark_support.h"

#include "core/jobs/job_queue.h"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/benchmark/catch_chronometer.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Vulkyrie;

namespace {

    constexpr std::size_t kCapacity = 1024;

} // namespace

TEST_CASE("JobQueue owner-side operations", "[jobs][queue]") {
    JobQueue queue(kCapacity);

    // The hot path in miniature: the owning thread publishing one job and immediately taking it
    // back (what every scheduling thread does when it out-runs the thieves).
    BENCHMARK("Push + pop round trip") {
        const bool pushed = queue.TryPush(u64{ 1 });
        const std::optional<u64> popped = queue.TryPop();
        return pushed && popped.has_value();
    };

    // Fill the whole deque, then drain it from the owner's end. Amortizes the round trip above over
    // a full buffer, so the difference between the two is the cost of the empty/full boundary
    // checks rather than the steady state.
    BENCHMARK_ADVANCED("Fill 1024, drain via TryPop")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&queue] {
            for (u64 i = 0; i < kCapacity; ++i) {
                (void)queue.TryPush(i);
            }

            u64 taken = 0;
            while (queue.TryPop().has_value()) {
                ++taken;
            }

            return taken;
        });
    };

    // Identical to the row above except the drain uses the thief path. Uncontended, so the delta
    // between the two rows is what the steal-side CAS costs even when nobody is competing for it.
    BENCHMARK_ADVANCED("Fill 1024, drain via TrySteal")(Catch::Benchmark::Chronometer meter) {
        meter.measure([&queue] {
            for (u64 i = 0; i < kCapacity; ++i) {
                (void)queue.TryPush(i);
            }

            u64 taken = 0;
            while (queue.TrySteal().has_value()) {
                ++taken;
            }

            return taken;
        });
    };
}
