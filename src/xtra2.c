/** \file xtra2.c 
    \brief Monster death, player attributes, targetting

 * Handlers for most of the player's temporary attributes, resistances,
 * nutrition, experience.  Monsters that drop a specific treasure, monster
 * death and subsequent events, screen scrolling, monster health descrip-
 * tions.  Sorting, targetting, what and how squares appear when looked at,
 * prompting for a direction to aim at and move in.
 *
 * Copyright (c) 2009 Nick McConnell, Leon Marrick & Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
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
#include "cave.h"
#include "cmds.h"
#include "history.h"
#include "keymap.h"
#include "spells.h"
#include "squelch.h"
#include "target.h"

#ifdef _WIN32_WCE
#include "angbandcw.h"
#endif				/* _WIN32_WCE */


/* Private function that is shared by verify_panel() and center_panel() */
void verify_panel_int(bool centered);

/**
 * Set "p_ptr->word_recall", notice observable changes
 */
bool word_recall(int v)
{
    if (!p_ptr->word_recall) {
	return set_recall(v);
    } else {
	return set_recall(0);
    }
}


/**
 * Advance experience levels and print experience
 */
void check_experience(void)
{
    int i;


    /* Note current level */
    i = p_ptr->lev;


    /* Hack -- lower limit */
    if (p_ptr->exp < 0)
	p_ptr->exp = 0;

    /* Hack -- lower limit */
    if (p_ptr->max_exp < 0)
	p_ptr->max_exp = 0;

    /* Hack -- upper limit */
    if (p_ptr->exp > PY_MAX_EXP)
	p_ptr->exp = PY_MAX_EXP;

    /* Hack -- upper limit */
    if (p_ptr->max_exp > PY_MAX_EXP)
	p_ptr->max_exp = PY_MAX_EXP;


    /* Hack -- maintain "max" experience */
    if (p_ptr->exp > p_ptr->max_exp)
	p_ptr->max_exp = p_ptr->exp;

    /* Redraw experience */
    p_ptr->redraw |= (PR_EXP);

    /* Handle stuff */
    handle_stuff(p_ptr);


    /* Lose levels while possible */
    while ((p_ptr->lev > 1) && (p_ptr->exp < (player_exp[p_ptr->lev - 2]))) {
	/* Lose a level */
	p_ptr->lev--;

	/* Update some stuff */
	p_ptr->update |=
	    (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);

	/* Redraw some stuff */
	p_ptr->redraw |= (PR_EXP | PR_LEV | PR_TITLE);

	/* Handle stuff */
	handle_stuff(p_ptr);
    }


    /* Gain levels while possible */
    while ((p_ptr->lev < PY_MAX_LEVEL)
	   && (p_ptr->exp >= (player_exp[p_ptr->lev - 1]))) {
	bool first_time = FALSE;

	/* Gain a level */
	p_ptr->lev++;

	/* Save the highest level */
	if (p_ptr->lev > p_ptr->max_lev) {
	    p_ptr->max_lev = p_ptr->lev;
	    first_time = TRUE;
	}

	/* Sound */
	sound(MSG_LEVEL);

	/* Message */
	msgt(MSG_LEVEL, "Welcome to level %d.", p_ptr->lev);

	/* Write a note to the file every 5th level. */

	if (((p_ptr->lev % 5) == 0) && first_time) {

	    char buf[120];

	    /* Log level updates */
	    strnfmt(buf, sizeof(buf), "Reached level %d", p_ptr->lev);
	    history_add(buf, HISTORY_GAIN_LEVEL, 0);

	}

	/* Update some stuff */
	p_ptr->update |=
	    (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);

	/* Redraw some stuff */
	p_ptr->redraw |= (PR_EXP | PR_LEV | PR_TITLE);

	/* Handle stuff */
	handle_stuff(p_ptr);
    }

    /* Gain max levels while possible Called rarely - only when leveling while
     * experience is drained.  */
    while ((p_ptr->max_lev < PY_MAX_LEVEL)
	   && (p_ptr->max_exp >= (player_exp[p_ptr->max_lev - 1]))) {
	/* Gain max level */
	p_ptr->max_lev++;

	/* Update some stuff */
	p_ptr->update |=
	    (PU_BONUS | PU_HP | PU_MANA | PU_SPELLS | PU_SPECIALTY);

	/* Redraw some stuff */
	p_ptr->redraw |= (PR_LEV | PR_TITLE);

	/* Handle stuff */
	handle_stuff(p_ptr);
    }
}


/**
 * Gain experience
 */
void gain_exp(s32b amount)
{
    /* Gain some experience */
    p_ptr->exp += amount;

    /* Slowly recover from experience drainage */
    if (p_ptr->exp < p_ptr->max_exp) {
	/* Gain max experience (10%) */
	p_ptr->max_exp += amount / 10;
    }

    /* Check Experience */
    check_experience();
}


/**
 * Lose experience
 */
void lose_exp(s32b amount)
{
    /* Never drop below zero experience */
    if (amount > p_ptr->exp)
	amount = p_ptr->exp;

    /* Lose some experience */
    p_ptr->exp -= amount;

    /* Check Experience */
    check_experience();
}


void town_adjust(int *dungeon_hgt, int *dungeon_wid)
{
    bool small_town = ((p_ptr->stage < 151) && (!OPT(adult_dungeon)));

    (*dungeon_hgt) /= 3;
    (*dungeon_wid) /= (small_town ? 6 : 3);
}

/*
 * Modify the current panel to the given coordinates, adjusting only to
 * ensure the coordinates are legal, and return TRUE if anything done.
 *
 * The town should never be scrolled around.
 *
 * Note that monsters are no longer affected in any way by panel changes.
 *
 * As a total hack, whenever the current panel changes, we assume that
 * the "overhead view" window should be updated.
 */
bool modify_panel(term *t, int wy, int wx)
{
	int dungeon_hgt = DUNGEON_HGT;
	int dungeon_wid = DUNGEON_WID;

	/* Adjust for town */
	if (p_ptr->danger == 0) town_adjust(&dungeon_hgt, &dungeon_wid);

	/* Verify wy, adjust if needed */
	if (wy > dungeon_hgt - SCREEN_HGT) wy = dungeon_hgt - SCREEN_HGT;
	if (wy < 0) wy = 0;

	/* Verify wx, adjust if needed */
	if (wx > dungeon_wid - SCREEN_WID) wx = dungeon_wid - SCREEN_WID;
	if (wx < 0) wx = 0;

	/* React to changes */
	if ((t->offset_y != wy) || (t->offset_x != wx))
	{
		/* Save wy, wx */
		t->offset_y = wy;
		t->offset_x = wx;

		/* Redraw map */
		p_ptr->redraw |= (PR_MAP);

		/* Redraw for big graphics */
		if ((tile_width > 1) || (tile_height > 1)) redraw_stuff(p_ptr);
      
		/* Hack -- optional disturb on "panel change" */
		if (OPT(disturb_panel) && !OPT(center_player)) disturb(0, 0);
  
		/* Changed */
		return (TRUE);
	}

	/* No change */
	return (FALSE);
}


/*
 * Perform the minimum "whole panel" adjustment to ensure that the given
 * location is contained inside the current panel, and return TRUE if any
 * such adjustment was performed.
 */
bool adjust_panel(int y, int x)
{
	bool changed = FALSE;

	int j;

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
		int wx, wy;
		int screen_hgt, screen_wid;

		term *t = angband_term[j];

		/* No window */
		if (!t) continue;

		/* No relevant flags */
		if ((j > 0) && !(op_ptr->window_flag[j] & PW_MAP)) continue;

		wy = t->offset_y;
		wx = t->offset_x;

		screen_hgt = (j == 0) ? SCREEN_HGT : t->hgt;
		screen_wid = (j == 0) ? SCREEN_WID : t->wid;

		/* Adjust as needed */
		while (y >= wy + screen_hgt) wy += screen_hgt / 2;
		while (y < wy) wy -= screen_hgt / 2;

		/* Adjust as needed */
		while (x >= wx + screen_wid) wx += screen_wid / 2;
		while (x < wx) wx -= screen_wid / 2;

		/* Use "modify_panel" */
		if (modify_panel(t, wy, wx)) changed = TRUE;
	}

	return (changed);
}


/*
 * Change the current panel to the panel lying in the given direction.
 *
 * Return TRUE if the panel was changed.
 */
bool change_panel(int dir)
{
	bool changed = FALSE;
	int j;

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
		int screen_hgt, screen_wid;
		int wx, wy;

		term *t = angband_term[j];

		/* No window */
		if (!t) continue;

		/* No relevant flags */
		if ((j > 0) && !(op_ptr->window_flag[j] & PW_MAP)) continue;

		screen_hgt = (j == 0) ? SCREEN_HGT : t->hgt;
		screen_wid = (j == 0) ? SCREEN_WID : t->wid;

		/* Shift by half a panel */
		wy = t->offset_y + ddy[dir] * screen_hgt / 2;
		wx = t->offset_x + ddx[dir] * screen_wid / 2;

		/* Use "modify_panel" */
		if (modify_panel(t, wy, wx)) changed = TRUE;
	}

	return (changed);
}


/*
 * Verify the current panel (relative to the player location).
 *
 * By default, when the player gets "too close" to the edge of the current
 * panel, the map scrolls one panel in that direction so that the player
 * is no longer so close to the edge.
 *
 * The "OPT(center_player)" option allows the current panel to always be centered
 * around the player, which is very expensive, and also has some interesting
 * gameplay ramifications.
 */
void verify_panel(void)
{
	verify_panel_int(OPT(center_player));
}

void center_panel(void)
{
	verify_panel_int(TRUE);
}

void verify_panel_int(bool centered)
{
	int wy, wx;
	int screen_hgt, screen_wid;

	int panel_wid, panel_hgt;

	int py = p_ptr->py;
	int px = p_ptr->px;

	int j;

	int hor = 3 * (1 + op_ptr->panel_change);
	int vert = 3 * (1 + op_ptr->panel_change);

	/* Scan windows */
	for (j = 0; j < ANGBAND_TERM_MAX; j++)
	{
		term *t = angband_term[j];

		/* No window */
		if (!t) continue;

		/* No relevant flags */
		if ((j > 0) && !(op_ptr->window_flag[j] & (PW_MAP))) continue;

		wy = t->offset_y;
		wx = t->offset_x;

		screen_hgt = (j == 0) ? SCREEN_HGT : t->hgt;
		screen_wid = (j == 0) ? SCREEN_WID : t->wid;

		panel_wid = screen_wid / 2;
		panel_hgt = screen_hgt / 2;


		/* Scroll screen vertically when off-center */
		if (centered && !p_ptr->running && (py != wy + panel_hgt))
			wy = py - panel_hgt;

		/* Scroll screen vertically when 3 grids from top/bottom edge */
		else if ((py < wy + vert) || (py >= wy + screen_hgt - vert))
			wy = py - panel_hgt;


		/* Scroll screen horizontally when off-center */
		if (centered && !p_ptr->running && (px != wx + panel_wid))
			wx = px - panel_wid;

		/* Scroll screen horizontally when 3 grids from left/right edge */
		else if ((px < wx + hor) || (px >= wx + screen_wid - hor))
			wx = px - panel_wid;


		/* Scroll if needed */
		modify_panel(t, wy, wx);
	}
}

/*
 * Given a "source" and "target" location, extract a "direction",
 * which will move one step from the "source" towards the "target".
 *
 * Note that we use "diagonal" motion whenever possible.
 *
 * We return "5" if no motion is needed.
 */
int motion_dir(int y1, int x1, int y2, int x2)
{
    /* No movement required */
    if ((y1 == y2) && (x1 == x2))
	return (5);

    /* South or North */
    if (x1 == x2)
	return ((y1 < y2) ? 2 : 8);

    /* East or West */
    if (y1 == y2)
	return ((x1 < x2) ? 6 : 4);

    /* South-east or South-west */
    if (y1 < y2)
	return ((x1 < x2) ? 3 : 1);

    /* North-east or North-west */
    if (y1 > y2)
	return ((x1 < x2) ? 9 : 7);

    /* Paranoia */
    return (5);
}


/*
 * Extract a direction (or zero) from a character
 */
int target_dir(struct keypress ch)
{
    int d = 0;


    /* Already a direction? */
    if (isdigit((unsigned char) ch.code)) {
	d = D2I(ch.code);
    } else if (isarrow(ch.code)) {
	switch (ch.code) {
	case ARROW_DOWN:
	    d = 2;
	    break;
	case ARROW_LEFT:
	    d = 4;
	    break;
	case ARROW_RIGHT:
	    d = 6;
	    break;
	case ARROW_UP:
	    d = 8;
	    break;
	}
    } else {
	int mode;
	const struct keypress *act;

	if (OPT(rogue_like_commands))
	    mode = KEYMAP_MODE_ROGUE;
	else
	    mode = KEYMAP_MODE_ORIG;

	/* XXX see if this key has a digit in the keymap we can use */
	act = keymap_find(mode, ch);
	if (act) {
	    const struct keypress *cur;
	    for (cur = act; cur->type == EVT_KBRD; cur++) {
		if (isdigit((unsigned char) cur->code))
		    d = D2I(cur->code);
	    }
	}
    }

    /* Paranoia */
    if (d == 5)
	d = 0;

    /* Return direction */
    return (d);
}


int dir_transitions[10][10] = {
    /* 0-> */ {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
    /* 1-> */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* 2-> */ {0, 0, 2, 0, 1, 0, 3, 0, 5, 0},
    /* 3-> */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* 4-> */ {0, 0, 1, 0, 4, 0, 5, 0, 7, 0},
    /* 5-> */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* 6-> */ {0, 0, 3, 0, 5, 0, 6, 0, 9, 0},
    /* 7-> */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    /* 8-> */ {0, 0, 5, 0, 7, 0, 9, 0, 8, 0},
    /* 9-> */ {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
};


/*
 * Get an "aiming direction" (1,2,3,4,6,7,8,9 or 5) from the user.
 *
 * Return TRUE if a direction was chosen, otherwise return FALSE.
 *
 * The direction "5" is special, and means "use current target".
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", if it is set.
 *
 * Note that "Force Target", if set, will pre-empt user interaction,
 * if there is a usable target already set.
 *
 * Currently this function applies confusion directly.
 */
bool get_aim_dir(int *dp)
{
    /* Global direction */
    int dir = 0;

    ui_event ke;

    const char *p;

    /* Initialize */
    (*dp) = 0;

    /* Hack -- auto-target if requested */
    if (OPT(use_old_target) && target_okay() && !dir)
	dir = 5;

    /* Ask until satisfied */
    while (!dir) {
	/* Choose a prompt */
	if (!target_okay())
	    p = "Direction ('*' or <click> to target, \"'\" for closest, Escape to cancel)? ";
	else
	    p = "Direction ('5' for target, '*' or <click> to re-target, Escape to cancel)? ";

	/* Get a command (or Cancel) */
	if (!get_com_ex(p, &ke))
	    break;

	if (ke.type == EVT_MOUSE) {
	    if (target_set_interactive
		(TARGET_KILL, KEY_GRID_X(ke), KEY_GRID_Y(ke)))
		dir = 5;
	} else if (ke.type == EVT_KBRD) {
	    if (ke.key.code == '*') {
		/* Set new target, use target if legal */
		if (target_set_interactive(TARGET_KILL, -1, -1))
		    dir = 5;
	    } else if (ke.key.code == '\'') {
		/* Set to closest target */
		if (target_set_closest(TARGET_KILL))
		    dir = 5;
	    } else if (ke.key.code == 't' || ke.key.code == '5' || ke.key.code == '0'
		       || ke.key.code == '.') {
		if (target_okay())
		    dir = 5;
	    } else {
		/* Possible direction */
		int keypresses_handled = 0;

		while (ke.key.code != 0) {
		    int this_dir;

		    /* XXX Ideally show and move the cursor here to indicate
		     * the currently "Pending" direction. XXX */
		    this_dir = target_dir(ke.key);

		    if (this_dir)
			dir = dir_transitions[dir][this_dir];
		    else
			break;

		    if (lazymove_delay == 0 || ++keypresses_handled > 1)
			break;

		    /* See if there's a second keypress within the defined
		     * period of time. */
		    inkey_scan = lazymove_delay;
		    ke = inkey_ex();
		}
	    }
	}

	/* Error */
	if (!dir)
	    bell("Illegal aim direction!");
    }

    /* No direction */
    if (!dir)
	return (FALSE);

    /* Save direction */
    (*dp) = dir;

    /* Check for confusion */
    if (p_ptr->timed[TMD_CONFUSED]) {
	/* Random direction */
	dir = ddd[randint0(8)];
    }

    /* Notice confusion */
    if ((*dp) != dir) {
	/* Warn the user */
	msg("You are confused.");
    }

    /* Save direction */
    (*dp) = dir;

    /* A "valid" direction was entered */
    return (TRUE);
}


/*
 * Request a "movement" direction (1,2,3,4,6,7,8,9) from the user.
 *
 * Return TRUE if a direction was chosen, otherwise return FALSE.
 *
 * This function should be used for all "repeatable" commands, such as
 * run, walk, open, close, bash, disarm, spike, tunnel, etc, as well
 * as all commands which must reference a grid adjacent to the player,
 * and which may not reference the grid under the player.
 *
 * Directions "5" and "0" are illegal and will not be accepted.
 *
 * This function tracks and uses the "global direction", and uses
 * that as the "desired direction", if it is set.
 */
bool get_rep_dir(int *dp)
{
    int dir = 0;

    ui_event ke;

    /* Initialize */
    (*dp) = 0;

    /* Get a direction */
    while (!dir) {
	/* Paranoia XXX XXX XXX */
	message_flush();

	/* Get first keypress - the first test is to avoid displaying the
	 * prompt for direction if there's already a keypress queued up and
	 * waiting - this just avoids a flickering prompt if there is a "lazy"
	 * movement delay. */
	inkey_scan = SCAN_INSTANT;
	ke = inkey_ex();
	inkey_scan = SCAN_OFF;

	if (ke.type == EVT_KBRD && target_dir(ke.key) == 0) {
	    prt("Direction or <click> (Escape to cancel)? ", 0, 0);
	    ke = inkey_ex();
	}

	/* Check mouse coordinates */
	if (ke.type == EVT_MOUSE) {
	    /* if (ke.button) */
	    {
		int y = KEY_GRID_Y(ke);
		int x = KEY_GRID_X(ke);

		/* Calculate approximate angle */
		int angle = get_angle_to_target(p_ptr->py, p_ptr->px, y, x, 0);

		/* Convert angle to direction */
		if (angle < 15)
		    dir = 6;
		else if (angle < 33)
		    dir = 9;
		else if (angle < 59)
		    dir = 8;
		else if (angle < 78)
		    dir = 7;
		else if (angle < 104)
		    dir = 4;
		else if (angle < 123)
		    dir = 1;
		else if (angle < 149)
		    dir = 2;
		else if (angle < 168)
		    dir = 3;
		else
		    dir = 6;
	    }
	}

	/* Get other keypresses until a direction is chosen. */
	else {
	    int keypresses_handled = 0;

	    while (ke.type == EVT_KBRD && ke.key.code != 0) {
		int this_dir;

		if (ke.key.code == ESCAPE) {
		    /* Clear the prompt */
		    prt("", 0, 0);

		    return (FALSE);
		}

		/* XXX Ideally show and move the cursor here to indicate the
		 * currently "Pending" direction. XXX */
		this_dir = target_dir(ke.key);

		if (this_dir) {
		    dir = dir_transitions[dir][this_dir];
		}

		if (lazymove_delay == 0 || ++keypresses_handled > 1)
		    break;

		inkey_scan = lazymove_delay;
		ke = inkey_ex();
	    }

	    /* 5 is equivalent to "escape" */
	    if (dir == 5) {
		/* Clear the prompt */
		prt("", 0, 0);

		return (FALSE);
	    }
	}

	/* Oops */
	if (!dir)
	    bell("Illegal repeatable direction!");
    }

    /* Clear the prompt */
    prt("", 0, 0);

    /* Save direction */
    (*dp) = dir;

    /* Success */
    return (TRUE);
}


/**
 * Apply confusion, if needed, to a direction
 *
 * Display a message and return TRUE if direction changes.
 */
bool confuse_dir(int *dp)
{
    int dir;

    /* Default */
    dir = (*dp);

    /* Apply "confusion" */
    if (p_ptr->timed[TMD_CONFUSED]) {
	/* Apply confusion XXX XXX XXX */
	if ((dir == 5) || (randint0(100) < 75)) {
	    /* Random direction */
	    dir = ddd[randint0(8)];
	}
    }

    /* Notice confusion */
    if ((*dp) != dir) {
	/* Warn the user */
	msg("You are confused.");

	/* Save direction */
	(*dp) = dir;

	/* Confused */
	return (TRUE);
    }

    /* Not confused */
    return (FALSE);
}
