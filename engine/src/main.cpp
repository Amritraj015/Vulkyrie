#include "vulkyrie.h"

#include "core/jobs/job_system.h"
#include "memory/memory_system.h"

int main([[maybe_unused]] i32 argc, [[maybe_unused]] char **argv) {
    // Initialize the logger sub-system.
    auto statusCode = Vulkyrie::Logger::InitializeLogger(Vulkyrie::LoggerType::Console);

    // If logger initialization failed, return the error code.
    if (Vulkyrie::StatusCode::Successful != statusCode) {
        return std::to_underlying(statusCode);
    }

    // Initialize the memory sub-system (attribution already works via static storage; this sets up
    // reporting and, in later phases, budgets and third-party allocator hooks).
    Vulkyrie::MemorySystem::Initialize();

    // Initialize the job sub-system (worker pool + task graph). Must come after the memory
    // sub-system so the pools are tracked, and shut down before it so workers are joined before
    // the shutdown report is emitted.
    Vulkyrie::JobSystem::Initialize();

    // Initialize the Vulkyrie Application.
    auto application = CreateApplication();

    // If the application is invalid, return an invalid application error.
    if (nullptr == application) {
        return std::to_underlying(Vulkyrie::StatusCode::InvalidApplication);
    }

    VLKY_PROFILE_BEGIN_SESSION("Game Loop", "profile_game_loop.json");

    // Run the application and get its status code.
    statusCode = application->Run();

    VLKY_PROFILE_END_SESSION();

    delete application;

    // Join every worker before the memory report so no counter is read mid-flight.
    Vulkyrie::JobSystem::Shutdown();

    // Emit the per-subsystem memory report (and, in later phases, the leak check).
    Vulkyrie::MemorySystem::Shutdown();

    // Terminate the logger sub-system and return its status code.
    return std::to_underlying(statusCode);
}
