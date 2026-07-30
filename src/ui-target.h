/**
 * \file ui-target.h
 * \brief UI for targetting code
 *
 * Copyright (c) 1997-2014 Angband contributors
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */


#ifndef UI_TARGET_H
#define UI_TARGET_H

#include "generate.h"
#include "ui-event.h"

#define ZOOM_LEVEL (player->upkeep->zoom_level)
#define SY_MIN MAX(0, Term->offset_y - ((ZOOM_LEVEL - 1) * SCREEN_HGT) / 2)
#define SX_MIN MAX(0, Term->offset_x - ((ZOOM_LEVEL - 1) * SCREEN_WID) / 2)
#define SY_MAX MIN(Term->offset_y + ((ZOOM_LEVEL - 1) * SCREEN_HGT) / 2 \
				   + SCREEN_HGT, ARENA_SIDE)
#define SX_MAX MIN(Term->offset_x + ((ZOOM_LEVEL - 1) * SCREEN_WID) / 2 \
				   + SCREEN_WID, ARENA_SIDE)
#define Y_ADD MAX(0, (SCREEN_HGT - ((SY_MAX - SY_MIN) / ZOOM_LEVEL)) / 2)
#define X_ADD MAX(0, (SCREEN_WID - ((SX_MAX - SX_MIN) / ZOOM_LEVEL)) / 2)

/**
 * Convert a "key event" into a "location" (Y)
 */
#define KEY_GRID_Y(K) \
	((int) (((K.mouse.y - ROW_MAP - Y_ADD) * ZOOM_LEVEL / tile_height)	\
			+ SY_MIN))

/**
 * Convert a "key event" into a "location" (X)
 */
#define KEY_GRID_X(K) \
	((int) (((K.mouse.x - COL_MAP - X_ADD) * ZOOM_LEVEL / tile_width)	\
			+ SX_MIN))

/**
 * Convert a "key event" into a "location"
 */
#define KEY_GRID(K) (loc(KEY_GRID_X(K), KEY_GRID_Y(K)))


/**
 * Height of the help screen; any higher than 4 will overlap the health
 * bar which we want to keep in targeting mode.
 */
#define HELP_HEIGHT 3

/**
 * Size of the array that is used for object names during targeting.
 */
#define TARGET_OUT_VAL_SIZE 256

int target_dir(struct keypress ch);
int target_dir_allow(struct keypress ch, bool allow_5, bool allow_esc);
void target_display_help(bool monster, bool object, bool free);
void textui_target(void);
void textui_target_closest(void);
bool target_set_interactive(int mode, struct loc grid, int range);

#endif /* UI_TARGET_H */
