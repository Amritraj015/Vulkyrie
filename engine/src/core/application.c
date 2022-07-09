#include "application.h"
#include "game_types.h"
#include "platform/platform.h"

#include "core/logger.h"
#include "core/vmemory.h"
#include "core/event.h"
#include "core/input.h"
#include "core/clock.h"

#include "renderer/renderer_frontend.h"

typedef struct application_state
{
    game *game_instance;
    b8 is_running;
    b8 is_suspended;
    platform_state platform;
    i16 width;
    i16 height;
    clock clock;
    f64 last_time;

} application_state;

static b8 initialized = FALSE;
static application_state app_state;

// Event handlers
b8 application_on_event(u16 code, void *sender, void *listener_inst, event_context context);
b8 application_on_key(u16 code, void *sender, void *listener_inst, event_context context);

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
    input_initialize();

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

    event_register(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_register(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_register(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

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

    // Renderer startup
    if (!renderer_initialize(game_instance->app_config.name, &app_state.platform))
    {
        V_FATAL("Failed to initialize renderer. Shutting down!");
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
    clock_start(&app_state.clock);
    clock_update(&app_state.clock);
    app_state.last_time = app_state.clock.elapsed;
    f64 running_time = 0;
    u8 frame_count = 0;
    f64 target_frame_seconds = 1.0f / 60;

    V_INFO(get_memory_usage_str());

    while (app_state.is_running)
    {
        if (!platform_pump_messages(&app_state.platform))
        {
            app_state.is_running = FALSE;
        }

        if (!app_state.is_suspended)
        {
            // Update clock and get delta time.
            clock_update(&app_state.clock);
            f64 current_time = app_state.clock.elapsed;
            f64 delta = (current_time - app_state.last_time);
            f64 frame_start_time = platform_get_absolute_time();

            if (!app_state.game_instance->update(app_state.game_instance, (f32)delta))
            {
                V_FATAL("Game update failed. Shutting down!");
                app_state.is_running = FALSE;
                break;
            }

            if (!app_state.game_instance->render(app_state.game_instance, (f32)delta))
            {
                V_FATAL("Game render failed. Shutting down!");
                app_state.is_running = FALSE;
                break;
            }

            // TODO: refactor packet creation
            render_packet packet;
            packet.delta_time = delta;
            renderer_draw_frame(&packet);

            // Figure out how long the frame took and, if below
            f64 frame_end_time = platform_get_absolute_time();
            f64 frame_elapsed_time = frame_end_time - frame_start_time;
            running_time += frame_elapsed_time;
            f64 remaining_seconds = target_frame_seconds - frame_elapsed_time;

            if (remaining_seconds > 0)
            {
                u64 remaining_ms = (remaining_seconds * 1000);

                // If there is time left, give it back to the OS.
                b8 limit_frames = FALSE;
                if (remaining_ms > 0 && limit_frames)
                {
                    platform_sleep(remaining_ms - 1);
                }

                frame_count++;
            }

            // NOTE: Input update/state copying should always be handled
            // after any input should be recorded; I.E. before this line.
            // As a safety, input is the last thing to be updated before
            // this frame ends.
            input_update(delta);

            // Update last time
            app_state.last_time = current_time;
        }
    }

    app_state.is_running = FALSE;

    event_unregister(EVENT_CODE_APPLICATION_QUIT, 0, application_on_event);
    event_unregister(EVENT_CODE_KEY_PRESSED, 0, application_on_key);
    event_unregister(EVENT_CODE_KEY_RELEASED, 0, application_on_key);

    event_shutdown();
    input_shutdown();
    renderer_shutdown();

    platform_shutdown(&app_state.platform);

    return TRUE;
}

b8 application_on_event(u16 code, void *sender, void *listener_inst, event_context context)
{
    switch (code)
    {
    case EVENT_CODE_APPLICATION_QUIT:
        V_INFO("EVENT_CODE_APPLICATION_QUIT received. Shutting down!");
        app_state.is_running = FALSE;
        return TRUE;
    }

    return FALSE;
}

b8 application_on_key(u16 code, void *sender, void *listener_inst, event_context context)
{
    if (code == EVENT_CODE_KEY_PRESSED)
    {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_ESCAPE)
        {
            // NOTE: Technically firing an event to itself, but there may be other listeners.
            event_context data = {};
            event_fire(EVENT_CODE_APPLICATION_QUIT, 0, data);

            // Block anything else from processing this.
            return TRUE;
        }
        else if (key_code == KEY_A)
        {
            // Example on checking for a key
            V_DEBUG("Explicit - A key pressed!");
        }
        else
        {
            V_DEBUG("'%c' key pressed in window.", key_code);
        }
    }
    else if (code == EVENT_CODE_KEY_RELEASED)
    {
        u16 key_code = context.data.u16[0];
        if (key_code == KEY_B)
        {
            // Example on checking for a key
            V_DEBUG("Explicit - B key released!");
        }
        else
        {
            V_DEBUG("'%c' key released in window.", key_code);
        }
    }
    return FALSE;
}