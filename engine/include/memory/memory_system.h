#pragma once

namespace Vulkyrie {

    /** @brief Lifecycle entry point for the memory subsystem.
     *
     * The cheap-tier counters and the global `operator new`/`delete` override work independently of
     * this facade (their state is in constant-initialized static storage), so calling `Initialize`
     * is not required for attribution to function. It exists so the memory subsystem has an explicit
     * bootstrap/shutdown hook alongside `Logger` and the profiler in `main`, and is where later
     * phases will install budgets, third-party allocator hooks, and the leak check.
     */
    class MemorySystem final {
    public:
        MemorySystem() = delete;

        /** @brief Announces the memory subsystem is up. Safe to call once, after the logger is ready. */
        static void Initialize();

        /** @brief Emits the shutdown memory report. Call at program teardown. */
        static void Shutdown();
    };

} // namespace Vulkyrie
