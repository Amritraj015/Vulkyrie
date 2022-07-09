#pragma once

#include "defines.h"

struct game;

typedef struct application_config
{
    /* Starting position of the window on x-axis, if applicable. */
    i16 start_position_x;

    /* Starting position of the window on y-axis, if applicable. */
    i16 start_position_y;

    /* Starting width of the window, if applicable. */
    i16 start_width;

    /* Starting height of the window, if applicable. */
    i16 start_height;

    /* The application name on the window, if applicable. */
    char *name;

} application_config;

V_API b8 application_create(struct game *game_instance);

V_API b8 application_run();