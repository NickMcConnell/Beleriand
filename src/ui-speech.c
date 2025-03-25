/**
 * \file ui-speech.c
 * \brief Text-based user interface for player speech
 *
 * Copyright (c) 1987 - 2025 Angband contributors
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

#include "angband.h"
#include "game-input.h"
#include "ui-input.h"
#include "ui-menu.h"
#include "ui-speech.h"

/**
 * ------------------------------------------------------------------------
 * Speaking to animals
 * ------------------------------------------------------------------------ */


/**
 * ------------------------------------------------------------------------
 * Speaking to people
 * ------------------------------------------------------------------------ */

/**
 * ------------------------------------------------------------------------
 * Language choice
 * ------------------------------------------------------------------------ */
const char *language_names[] = {
	#define LANG(a, b) b,
	#include "list-languages.h"
	#undef LANG
};

static int player_languages[LANGUAGE_MAX];

static int get_languages(void)
{
	int i, count = 0;
	for (i = 0; i < LANGUAGE_MAX; i++) {
		if (language_has(player->languages, i)) {
			player_languages[count++] = i;
		}
	}
	return count;
}

/**
 * Display an entry in the skill menu.
 */
static void language_display(struct menu *menu, int oid, bool cursor, int row,
						 int col, int width)
{
	int *choice = menu->menu_data;
	uint8_t attr = (cursor ? COLOUR_L_BLUE : COLOUR_WHITE);
	c_put_str(attr, language_names[choice[oid]], row, col);	
}

/**
 * Handle keypresses.
 */
static bool language_action(struct menu *m, const ui_event *event, int oid)
{
	return (event->type == EVT_SELECT) ? false : true;
}

/**
 * Display the languages main menu.
 */
int textui_choose_language(void)
{
	struct menu menu;
	menu_iter menu_f = { NULL, NULL, language_display, language_action, NULL };
	int count = get_languages();
	ui_event evt = EVENT_EMPTY;
	int selection;

	screen_save();
	clear_from(0);

	/* Set up the menu */
	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu.title = "Languages";
	menu_setpriv(&menu, count, player_languages);
	menu.selections = lower_case;
	menu.flags = MN_CASELESS_TAGS;
	menu_layout(&menu, &SCREEN_REGION);

	/* Select an entry */
	evt = menu_select(&menu, 0, false);
	if (evt.type & EVT_SELECT) {
		selection = menu.cursor;
	} else {
		selection = -1;
	}

	screen_load();
	return selection;
}
