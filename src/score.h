/**
 * \file score.h
 * \brief Highscore handling for Angband
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

#ifndef INCLUDED_SCORE_H
#define INCLUDED_SCORE_H

struct player;

/**
 * Maximum number of high scores in the high score file
 */
#define MAX_HISCORES    100


/**
 * Semi-Portable High Score List Entry (128 bytes)
 *
 * All fields listed below are null terminated ascii strings.
 *
 * In addition, the "number" fields are right justified, and
 * space padded, to the full available length (minus the "null").
 *
 * Note that "string comparisons" are thus valid on "pts".
 */
struct high_score {
	char what[8];		/* Version info (string) */
	char pts[10];		/* Total Score (number) */
	char turns[10];		/* Turns Taken (number) */
	char day[10];		/* Time stamp (string) */
	char who[16];		/* Player Name (string) */
	char uid[8];		/* Player UID (number) */
	char p_s[3];		/* Player Sex (number) */
	char p_r[3];		/* Player Race (number) */
	char p_h[3];		/* Player House (number) */
	char cur_dun[4];	/* Current Dungeon Level (number) */
	char max_dun[4];	/* Max Dungeon Level (number) */
	char how[50];		/* Method of death (string) */
	char silmarils[2];		/* Number of Silmarils (number) */
	char morgoth_slain[2];	/* Has player slain Morgoth (t/f) */
	char escaped[2];		/* Has player escaped (t/f) */
};



size_t highscore_read(struct high_score scores[], size_t sz);
size_t highscore_where(const struct high_score *entry,
					   const struct high_score scores[], size_t sz);
size_t highscore_add(const struct high_score *entry, struct high_score scores[],
					 size_t sz);
void build_score(struct high_score *entry, const struct player *p,
		const char *died_from, const time_t *death_time);
void enter_score(const struct player *p, const time_t *death_time);

#endif /* INCLUDED_SCORE_H */
