/**
 * \file combat.h
 * \brief All forms of combat
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

#ifndef COMBAT_H
#define COMBAT_H

#include "angband.h"

struct source;
struct monster;
struct player;

bool knock_back(struct loc grid1, struct loc grid2);
int skill_check(struct source attacker, int skill, int difficulty,
				struct source defender);
int hit_roll(int att, int evn, struct source attacker, struct source defender,
			 bool display_roll);
int total_player_attack(struct player *p, struct monster *mon, int base);
int total_player_evasion(struct player *p, struct monster *mon, bool archery);
int total_monster_attack(struct player *p, struct monster *mon, int base);
int total_monster_evasion(struct player *p, struct monster *mon, bool archery);
int stealth_melee_bonus(const struct monster *mon);
int overwhelming_att_mod(struct monster *mon);
int crit_bonus(struct player *p, int hit_result, int weight,
			   const struct monster_race *race, int skill_type, bool thrown);
int protection_roll(struct player *p, int typ, bool melee, aspect prot_aspect);

#endif /* !COMBAT_H */
