/**
 * \file mon-group.h
 * \brief Monster group behaviours
 *
 * Copyright (c) 2018 Nick McConnell
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
#ifndef MON_GROUP_H
#define MON_GROUP_H

#include "monster.h"

struct mon_group_list_entry {
	int midx;
	struct mon_group_list_entry *next;
};

struct monster_group {
	int index;
	int leader;
	struct player_race *player_race;
	struct mon_group_list_entry *member_list;
};

void monster_group_free(struct monster_group *group);
void monster_remove_from_groups(struct monster *mon);
int monster_group_index_new(void);
void monster_add_to_group(struct monster *mon, struct monster_group *group);
void monster_group_start(struct monster *mon, int which);
void monster_group_assign(struct monster *mon,
						  struct monster_group_info *info, bool loading);
struct monster_group *monster_group_by_index(int index);
bool monster_group_change_index(int new, int old);
struct monster_group *summon_group(int midx);
void monster_group_rouse(struct monster *mon);
int monster_primary_group_size(const struct monster *mon);
struct monster *group_monster_tracking(const struct monster *mon);
struct monster *monster_group_leader(struct monster *mon);
struct player_race *monster_group_player_race(struct monster *mon);
void set_monster_group_player_race(struct monster *mon,
								   struct player_race *race);
void monster_groups_verify(void);

#endif /* !MON_GROUP_H */
