#pragma once

#include "defines.h"
#include <game_types.h>

typedef struct game_state
{
    f32 delta_time;
} game_state;

/* Function to initialize the game. */
b8 game_initialize(struct game *game_inst);

/* Function to update the game. */
b8 game_update(struct game *game_inst, f32 delta_time);

/* Function to render the game. */
b8 game_render(struct game *game_inst, f32 delta_time);

/* Function to resize the game. */
void game_on_resize(struct game *game_inst, u32 width, u32 height);
