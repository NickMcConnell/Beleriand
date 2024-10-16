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
	int index;				/* Index of this group */
	int leader;				/* Group leader index */
	struct flow flow;		/* Group flow */
	int wandering_pause;	/* Length of pause from wandering */
	int size;				/* Number of members */
	int dist;				/* Distance from destination */
	int furthest;			/* */
	struct mon_group_list_entry *member_list;
};

void monster_group_free(struct monster_group *group);
void monster_group_remove_leader(struct monster *leader,
								 struct monster_group *group);
void monster_remove_from_group(struct monster *mon);
int monster_group_index_new(void);
void monster_add_to_group(struct monster *mon, struct monster_group *group);
void monster_group_assign(struct monster *mon, struct monster_group_info info,
						  bool loading);
struct monster_group *monster_group_by_index(int index);
bool monster_group_change_index(int new, int old);
int monster_group_size(const struct monster *mon);
int monster_group_leader_idx(struct monster_group *group);
struct monster *monster_group_leader(struct monster *mon);
void monster_group_new_wandering_flow(struct monster *mon, struct loc tgrid);
void monster_groups_verify(void);

#endif /* !MON_GROUP_H */
