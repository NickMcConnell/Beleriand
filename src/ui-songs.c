/**
 * \file ui-songs.c
 * \brief Selection of player songs
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "player-abilities.h"
#include "songs.h"
#include "ui-menu.h"
#include "ui-songs.h"

struct song_menu_info {
	struct song *song;
	bool swap;
};

static struct song_menu_info *songlist;

static int get_songs(void)
{
	struct ability *a = abilities;
	int count = 0;

	/* Add the chance to stop singing */
	if (player->song[SONG_MAIN]) {
		songlist[count].swap = false;
		songlist[count++].song = NULL;
	}

	/* Add the chance to exchange themes */
	if (player->song[SONG_MINOR]) {
		songlist[count].swap = true;
		songlist[count++].song = NULL;
	}

	/* Find available songs */
	while (a) {
		if ((a->skill == SKILL_SONG) && strstr(a->name, "Song of") &&
			player_active_ability(player, a->name)) {
			songlist[count].swap = false;
			songlist[count++].song = lookup_song(a->name + strlen("Song of "));
		}
		a = a->next;
	}
	return count;
}


/**
 * Display an entry in the song menu.
 */
static void song_display(struct menu *menu, int oid, bool cursor, int row,
						 int col, int width)
{
	struct song_menu_info *choice = menu->menu_data;
	struct song *song = choice[oid].song;
	char *str;
	uint8_t attr = (cursor ? COLOUR_L_BLUE : COLOUR_WHITE);

	if (song) {
		str = format("Song of %s", song->name);
	} else if (choice[oid].swap) {
		str = "Exchange themes";
	} else {
		str = "Stop singing";
	}
	c_put_str(attr, str, row, col);	
}

/**
 * Handle keypresses in the song menu.
 */
static bool song_action(struct menu *m, const ui_event *event, int oid)
{
	struct song_menu_info *choice = m->menu_data;
	struct song *song = choice[oid].song;
	if (event->type == EVT_SELECT) {
		if (song == NULL) {
			if (choice[oid].swap) {
				/* Exchange themes */
				player_change_song(player, NULL, true);
			} else {
				/* End the song */
				player_change_song(player, NULL, false);
			}
		} else {
			/* Start singing */
			player_change_song(player, song, false);
		}
		return false;
	}
	return true;
}

void textui_change_song(void)
{
	struct menu menu;
	menu_iter menu_f = { NULL, NULL, song_display, song_action, NULL };
	region area = { 10, 2, 0, 0 };
	int count;

	songlist = mem_zalloc(100 * sizeof(struct song_menu_info));
	count = get_songs();
	if (!count) {
		msg("You do not know any songs of power.");
		mem_free(songlist);
		return;
	}

	menu_init(&menu, MN_SKIN_SCROLL, &menu_f);
	menu.title = "Songs";
	menu.selections = lower_case;
	menu.flags = MN_CASELESS_TAGS;

	menu_setpriv(&menu, count, songlist);
	menu_layout(&menu, &area);
	menu_select(&menu, 0, true);

	mem_free(songlist);
}
