#pragma once

// Shared support for the job tests that rebuild the job system around a custom configuration.
// Restoration is RAII rather than a call at the end of the test body because a failed assertion
// throws out of the test: a missed restore would leave every following test running on (say) a
// two-slot pool, turning one failure into a cascade of unrelated ones.
#include "core/jobs/job_system.h"

namespace Vulkyrie::Tests {

    /** @brief Restores the run listener's job-system configuration (automatic worker count, no core
     * pinning) when it goes out of scope. Declare one in any test that calls `JobSystem::Shutdown`
     * or `JobSystem::Initialize`. */
    class JobSystemConfigRestorer final {
    public:
        JobSystemConfigRestorer() = default;

        VE_DELETE_MOVE_AND_COPY(JobSystemConfigRestorer);

        ~JobSystemConfigRestorer() {
            JobSystem::Shutdown();

            JobSystemConfig config{}; // Mirrors jobs_test_listener.cpp.
            config.PinToCores = false;
            JobSystem::Initialize(config);
        }
    };

} // namespace Vulkyrie::Tests
