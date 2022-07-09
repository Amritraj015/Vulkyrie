#include "game.h"
#include <entry.h>

// TODO: REMOVE THIS....
#include <core/vmemory.h>

b8 create_game(game *out_game)
{
    // Application configuration.
    out_game->app_config.start_position_x = 100;
    out_game->app_config.start_position_y = 100;
    out_game->app_config.start_width = 1200;
    out_game->app_config.start_height = 720;
    out_game->app_config.name = "Vulkyrie Game Engine";
    out_game->initialize = game_initialize;
    out_game->update = game_update;
    out_game->render = game_render;
    out_game->on_resize = game_on_resize;

    // Create the game state.
    out_game->state = v_allocate(sizeof(game_state), MEMORY_TAG_GAME);

    return TRUE;
}