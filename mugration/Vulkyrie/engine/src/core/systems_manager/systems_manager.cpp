#include "systems_manager.hpp"
#include "core/logger/ilogger.hpp"

b8 SystemsManager::initialized = FALSE;

void SystemsManager::initialize_systems()
{
    if (initialized)
    {
        V_WARN("All systems are initialized. SystemsManager::initialize_systems() should only be invoked once");
        return;
    }

    V_DEBUG("All systems initialized successfully!.");

    initialized = TRUE;
}

void SystemsManager::terminate_systems()
{
    if (!initialized)
    {
        V_WARN("All systems are already in terminated status");
        return;
    }

    V_DEBUG("All systems terminated successfully!.");

    initialized = FALSE;
}