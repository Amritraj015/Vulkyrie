#include "core/jobs/job_system.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

using namespace Vulkyrie;

namespace {

    /** Ticket dispenser: each graph node records the order it executed in. */
    struct ExecutionOrder {
    public:
        std::atomic<u32> NextTicket{ 0 };
        std::array<std::atomic<u32>, 4> Tickets{};

        void Record(std::size_t node) {
            Tickets[node].store(NextTicket.fetch_add(1, std::memory_order_relaxed), std::memory_order_relaxed);
        }

        [[nodiscard]] u32 Of(std::size_t node) const {
            return Tickets[node].load(std::memory_order_relaxed);
        }
    };

} // namespace

TEST_CASE("A chain A->B->C executes in dependency order even when scheduled in reverse", "[jobs]") {
    ExecutionOrder order;

    const JobHandle a = JobSystem::Create([&order] { order.Record(0); });
    const JobHandle b = JobSystem::Create([&order] { order.Record(1); });
    const JobHandle c = JobSystem::Create([&order] { order.Record(2); });

    JobSystem::AddDependency(b, a);
    JobSystem::AddDependency(c, b);

    // Scheduling order must not matter — only the edges do.
    JobSystem::Schedule(c);
    JobSystem::Schedule(b);
    JobSystem::Schedule(a);

    JobSystem::Wait(c);

    REQUIRE(order.NextTicket.load(std::memory_order_relaxed) == 3);
    REQUIRE(order.Of(0) < order.Of(1));
    REQUIRE(order.Of(1) < order.Of(2));
}

TEST_CASE("A diamond A->{B,C}->D runs D last and every node exactly once", "[jobs]") {
    ExecutionOrder order;
    std::atomic<u32> executions{ 0 };

    const JobHandle a = JobSystem::Create([&order, &executions] {
        order.Record(0);
        executions.fetch_add(1, std::memory_order_relaxed);
    });
    const JobHandle b = JobSystem::Create([&order, &executions] {
        order.Record(1);
        executions.fetch_add(1, std::memory_order_relaxed);
    });
    const JobHandle c = JobSystem::Create([&order, &executions] {
        order.Record(2);
        executions.fetch_add(1, std::memory_order_relaxed);
    });
    const JobHandle d = JobSystem::Create([&order, &executions] {
        order.Record(3);
        executions.fetch_add(1, std::memory_order_relaxed);
    });

    JobSystem::AddDependency(b, a);
    JobSystem::AddDependency(c, a);
    JobSystem::AddDependency(d, b);
    JobSystem::AddDependency(d, c);

    JobSystem::Schedule(d);
    JobSystem::Schedule(c);
    JobSystem::Schedule(b);
    JobSystem::Schedule(a);

    JobSystem::Wait(d);

    REQUIRE(executions.load(std::memory_order_relaxed) == 4);
    REQUIRE(order.Of(0) < order.Of(1));
    REQUIRE(order.Of(0) < order.Of(2));
    REQUIRE(order.Of(1) < order.Of(3));
    REQUIRE(order.Of(2) < order.Of(3));
}

TEST_CASE("A job with an unmet dependency never runs early", "[jobs]") {
    std::atomic<bool> aRan{ false };
    std::atomic<bool> bSawA{ false };

    const JobHandle a = JobSystem::Create([&aRan] { aRan.store(true, std::memory_order_release); });
    const JobHandle b = JobSystem::Create([&aRan, &bSawA] { bSawA.store(aRan.load(std::memory_order_acquire), std::memory_order_relaxed); });

    JobSystem::AddDependency(b, a);
    JobSystem::Schedule(b);

    // A is created but deliberately not scheduled yet: B must stay pending.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    REQUIRE_FALSE(JobSystem::IsComplete(b));
    REQUIRE_FALSE(aRan.load(std::memory_order_relaxed));

    JobSystem::Schedule(a);
    JobSystem::Wait(b);

    REQUIRE(aRan.load(std::memory_order_relaxed));
    REQUIRE(bSawA.load(std::memory_order_relaxed));
}

TEST_CASE("A dependency on an already-finished job is satisfied immediately", "[jobs]") {
    const JobHandle finished = JobSystem::Run([] {});
    JobSystem::Wait(finished);

    std::atomic<bool> ran{ false };
    const JobHandle successor = JobSystem::Create([&ran] { ran.store(true, std::memory_order_relaxed); });
    JobSystem::AddDependency(successor, finished);
    JobSystem::Schedule(successor);
    JobSystem::Wait(successor);

    REQUIRE(ran.load(std::memory_order_relaxed));
}

TEST_CASE("Fan-in and fan-out edges all resolve", "[jobs]") {
    constexpr u32 kFan = 64;

    // Fan-in: kFan predecessors gate a single join job.
    std::atomic<u32> predecessorsDone{ 0 };
    std::atomic<u32> violations{ 0 };

    const JobHandle join = JobSystem::Create([&predecessorsDone, &violations] {
        if (predecessorsDone.load(std::memory_order_acquire) != kFan) {
            violations.fetch_add(1, std::memory_order_relaxed);
        }
    });

    for (u32 i = 0; i < kFan; ++i) {
        const JobHandle predecessor = JobSystem::Create([&predecessorsDone] { predecessorsDone.fetch_add(1, std::memory_order_release); });
        JobSystem::AddDependency(join, predecessor);
        JobSystem::Schedule(predecessor);
    }

    JobSystem::Schedule(join);
    JobSystem::Wait(join);

    REQUIRE(violations.load(std::memory_order_relaxed) == 0);
    REQUIRE(predecessorsDone.load(std::memory_order_relaxed) == kFan);

    // Fan-out: a single root gates kFan successors.
    std::atomic<bool> rootDone{ false };
    std::atomic<u32> successorsDone{ 0 };

    const JobHandle root = JobSystem::Create([&rootDone] { rootDone.store(true, std::memory_order_release); });

    std::vector<JobHandle> successors;
    successors.reserve(kFan);
    for (u32 i = 0; i < kFan; ++i) {
        const JobHandle successor = JobSystem::Create([&rootDone, &violations, &successorsDone] {
            if (!rootDone.load(std::memory_order_acquire)) {
                violations.fetch_add(1, std::memory_order_relaxed);
            }
            successorsDone.fetch_add(1, std::memory_order_relaxed);
        });
        JobSystem::AddDependency(successor, root);
        JobSystem::Schedule(successor);
        successors.push_back(successor);
    }

    JobSystem::Schedule(root);
    for (const JobHandle &successor : successors) {
        JobSystem::Wait(successor);
    }

    REQUIRE(violations.load(std::memory_order_relaxed) == 0);
    REQUIRE(successorsDone.load(std::memory_order_relaxed) == kFan);
}
