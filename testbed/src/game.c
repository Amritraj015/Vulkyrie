#include "game.h"
#include <core/logger.h>

/* Function to initialize the game. */
b8 game_initialize(struct game *game_inst)
{
    V_DEBUG("Game initialized!");
    return TRUE;
}

/* Function to update the game. */
b8 game_update(struct game *game_inst, f32 delta_time)
{
    return TRUE;
}

/* Function to render the game. */
b8 game_render(struct game *game_inst, f32 delta_time)
{
    return TRUE;
}

/* Function to resize the game. */
void game_on_resize(struct game *game_inst, u32 width, u32 height)
{
}