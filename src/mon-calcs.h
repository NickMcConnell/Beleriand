/**
 * \file mon-calcs.h
 * \brief Monster status calculation 
 *	status changes.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2022 Nick McConnell
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

#include "cave.h"

int monster_elf_bane_bonus(struct monster *mon, struct player *p);
void calc_morale(struct monster *mon);
void calc_stance(struct monster *mon);
void make_alert(struct monster *mon, int dam);
void set_alertness(struct monster *mon, int alertness);
void update_mon(struct monster *mon, struct chunk *c, bool full);
void update_monsters(bool full);
int monster_skill(struct monster *mon, int skill_type);
int monster_stat(struct monster *mon, int stat_type);
void calc_monster_speed(struct monster *mon);
