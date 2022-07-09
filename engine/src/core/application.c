#include "application.h"
#include "logger.h"
#include "platform/platform.h"
#include "game_types.h"
#include "core/vmemory.h"
#include "core/event.h"

typedef struct application_state
{
    game *game_instance;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    f64 last_time;

} application_state;

static b8 initialized = FALSE;
static application_state app_state;

b8 application_create(game *game_instance)
{
    if (initialized)
    {
        V_ERROR("Application has already been initialized");
        return FALSE;
    }

    app_state.game_instance = game_instance;

    // Initialize sub-systems.
    initialize_logging();

    /*****************************************************/
    // TODO: Remove this later...
    V_FATAL("A test message: %f", 3.14f);
    V_ERROR("A test message: %f", 3.14f);
    V_WARN("A test message: %f", 3.14f);
    V_INFO("A test message: %f", 3.14f);
    V_DEBUG("A test message: %f", 3.14f);
    V_TRACE("A test message: %f", 3.14f);
    // V_ASSERT(1 == 0);
    /*****************************************************/

    app_state.is_running = TRUE;
    app_state.is_suspended = FALSE;

    if (!event_initialize())
    {
        V_ERROR("Event system failed initialization. Application cannot continue.");
        return FALSE;
    }

    if (!platform_startup(
            &app_state.platform,
            game_instance->app_config.name,
            game_instance->app_config.start_position_x,
            game_instance->app_config.start_position_y,
            game_instance->app_config.start_width,
            game_instance->app_config.start_height))
    {
        return FALSE;
    }

    // Initialize game.
    if (!app_state.game_instance->initialize(app_state.game_instance))
    {
        V_FATAL("Game failed to initialize");
        return FALSE;
    }

    app_state.game_instance->on_resize(app_state.game_instance, app_state.width, app_state.height);

    initialized = TRUE;

    return TRUE;
}

b8 application_run()
{
    V_INFO(get_memory_usage_str());

    while (app_state.is_running)
    {
        if (!platform_pump_messages(&app_state.platform))
        {
            app_state.is_running = FALSE;
        }

        if (!app_state.is_suspended)
        {
            if (!app_state.game_instance->update(app_state.game_instance, (f32)0))
            {
                V_FATAL("Game update failed. Shutting down!");
                app_state.is_running = FALSE;
                break;
            }

            if (!app_state.game_instance->render(app_state.game_instance, (f32)0))
            {
                V_FATAL("Game render failed. Shutting down!");
                app_state.is_running = FALSE;
                break;
            }
        }
    }

    app_state.is_running = FALSE;

    event_shutdown();

    platform_shutdown(&app_state.platform);

    return TRUE;
}