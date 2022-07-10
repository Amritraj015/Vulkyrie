#include <factories/service_provider.hpp>
#include <core/systems_manager/systems_manager.hpp>

int main()
{
    SystemsManager::initialize_systems();

    V_FATAL("A fetal message: %f", 3.2f);
    V_ERROR("An error message: %f", 3.13f);
    V_WARN("A warning message: %f", 3.4f);
    V_INFO("A info message: %f", 3.5f);
    V_DEBUG("A debug message: %f", 3.6f);
    V_TRACE("A trace message: %f", 3.7f);

    SystemsManager::terminate_systems();

    return FALSE;
}