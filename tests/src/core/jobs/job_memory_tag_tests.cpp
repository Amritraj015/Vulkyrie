// The Phase 0 seam: `MemoryScope`'s stack is thread_local, so without explicit propagation every
// allocation inside a job would silently land in MemoryTag::Untagged the moment work moves to a
// worker thread. `JobSystem::Create` captures the submitting thread's tag and execution re-enters
// it, so a VE_MEMORY_SCOPE on the main thread follows the work onto every worker. This test keeps
// that seam from rotting.
#include "core/jobs/job_system.h"
#include "memory/memory_tracker.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstddef>

using namespace Vulkyrie;

namespace {

    // Escapes the allocation so the compiler cannot elide the new/delete pair (C++ N3664), and
    // launders the size so it is not a compile-time constant — both mirror memory_tracker_tests.cpp.
    std::atomic<void *> gMemorySink{ nullptr };

    [[nodiscard]] std::size_t RuntimeSize(std::size_t value) {
        volatile std::size_t v = value;
        return v;
    }

} // namespace

TEST_CASE("Jobs inherit the submitting thread's memory tag across threads", "[jobs][memory]") {
    const i64 physicsBefore = MemoryTracker::CurrentBytes(MemoryTag::Physics);

    std::atomic<MemoryTag> observedTag{ MemoryTag::Untagged };
    std::atomic<i64> observedPhysicsDelta{ 0 };

    JobHandle handle{};
    {
        VE_MEMORY_SCOPE(MemoryTag::Physics);
        handle = JobSystem::Run([&observedTag, &observedPhysicsDelta, physicsBefore] {
            // Runs on whichever thread picked the job up; the scope must have followed the work.
            observedTag.store(CurrentMemoryTag(), std::memory_order_relaxed);

            const std::size_t size = RuntimeSize(1U << 20U);
            char *buffer = new char[size];
            gMemorySink.store(buffer, std::memory_order_relaxed); // Escape to defeat allocation elision.
            buffer[0] = char{ 1 };
            buffer[size - 1] = char{ 2 };

            observedPhysicsDelta.store(MemoryTracker::CurrentBytes(MemoryTag::Physics) - physicsBefore, std::memory_order_relaxed);
            delete[] buffer;
        });
    }

    JobSystem::Wait(handle);

    REQUIRE(observedTag.load(std::memory_order_relaxed) == MemoryTag::Physics);
#if !defined(VE_MEMORY_DISABLE_GLOBAL_NEW)
    // The counter assertion needs the tracked operator new override, which sanitizer builds
    // compile out via VE_MEMORY_DISABLE_GLOBAL_NEW; the tag-propagation check above is the seam.
    REQUIRE(observedPhysicsDelta.load(std::memory_order_relaxed) >= static_cast<i64>(1U << 20U));
#endif
}

TEST_CASE("A job created outside any memory scope attributes to Untagged, not a stale tag", "[jobs][memory]") {
    std::atomic<MemoryTag> observedTag{ MemoryTag::Physics };

    const JobHandle handle = JobSystem::Run([&observedTag] { observedTag.store(CurrentMemoryTag(), std::memory_order_relaxed); });
    JobSystem::Wait(handle);

    REQUIRE(observedTag.load(std::memory_order_relaxed) == MemoryTag::Untagged);
}
