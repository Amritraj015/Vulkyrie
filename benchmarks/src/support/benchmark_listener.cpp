// The benchmark binary gets its `main` from Catch2, not from `engine/src/main.cpp`, so engine
// subsystems with an explicit lifecycle are brought up here — once per run, before the first
// benchmark, and torn down after the last one. A benchmark for any subsystem can therefore assume
// the engine is in its normal running state; add subsystems here as they grow benchmarks.
#include "core/jobs/job_system.h"
#include "core/logger.h"

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

namespace {

    class BenchmarkRunListener final : public Catch::EventListenerBase {
    public:
        using EventListenerBase::EventListenerBase;

        void testRunStarting(const Catch::TestRunInfo & /*testRunInfo*/) override {
            // Without a sink, an engine warning during a benchmark vanishes — and a benchmark that
            // is quietly hitting a slow path (an exhausted pool, a failed core pin) is worse than
            // no benchmark at all.
            (void)Vulkyrie::Logger::InitializeLogger(Vulkyrie::LoggerType::Console);

            Vulkyrie::JobSystemConfig config{};
            config.PinToCores = false; // See JobSystemScope: pinning distorts numbers on a busy machine.
            Vulkyrie::JobSystem::Initialize(config);
        }

        void testRunEnded(const Catch::TestRunStats & /*testRunStats*/) override {
            Vulkyrie::JobSystem::Shutdown();
        }
    };

} // namespace

CATCH_REGISTER_LISTENER(BenchmarkRunListener)
