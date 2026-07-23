// The Catch2 `tests` binary gets its `main` from Catch2, not from `engine/src/main.cpp`, so the
// job system's explicit lifecycle is driven by this run listener instead: workers come up before
// the first test and are joined after the last one (before Catch2's own teardown).
#include "core/jobs/job_system.h"

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

namespace {

    class JobSystemRunListener final : public Catch::EventListenerBase {
    public:
        using EventListenerBase::EventListenerBase;

        void testRunStarting(const Catch::TestRunInfo & /*testRunInfo*/) override {
            Vulkyrie::JobSystemConfig config{};
            config.PinToCores = false; // Don't fight the OS scheduler on shared dev/CI machines.
            Vulkyrie::JobSystem::Initialize(config);
        }

        void testRunEnded(const Catch::TestRunStats & /*testRunStats*/) override {
            Vulkyrie::JobSystem::Shutdown();
        }
    };

} // namespace

CATCH_REGISTER_LISTENER(JobSystemRunListener)
