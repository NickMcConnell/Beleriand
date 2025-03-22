/**
 * \file mon-move.h
 * \brief Monster movement
 *
 * Copyright (c) 1997 Ben Harrison, David Reeve Sward, Keldon Jones.
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
#ifndef MONSTER_MOVE_H
#define MONSTER_MOVE_H


enum monster_stagger {
	 NO_STAGGER = 0,
	 CONFUSED_STAGGER = 1,
	 INNATE_STAGGER = 2
};

/**
 * Monster alertness states
 */
enum monster_alertness {
	ALERTNESS_MIN = -20,
	ALERTNESS_UNWARY = -10,
	ALERTNESS_ALERT = 0,
	ALERTNESS_QUITE_ALERT = 5,
	ALERTNESS_VERY_ALERT = 10,
	ALERTNESS_MAX = 20
};

/**
 * Monster stances
 */
enum monster_stance {
	STANCE_ALLIED = -3,
	STANCE_FRIENDLY = -2,
	STANCE_CAUTIOUS = -1,
	STANCE_NEUTRAL = 0,
	STANCE_FLEEING = 1,
	STANCE_CONFIDENT = 2,
	STANCE_AGGRESSIVE = 3
};

int monster_entry_chance(struct chunk *c, struct monster *mon, struct loc grid,
						 bool *bash);
int adj_mon_count(struct loc grid);
void tell_allies(struct monster *mon, int flag);
bool multiply_monster(const struct monster *mon);
void process_monsters(int minimum_energy);
void reset_monsters(void);
void restore_monsters(int place, int num_turns);

#endif /* !MONSTER_MOVE_H */
