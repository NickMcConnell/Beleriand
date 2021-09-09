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

void monster_group_free(struct chunk *c, struct monster_group *group);
void monster_group_remove_leader(struct chunk *c, struct monster *leader,
								 struct monster_group *group);
void monster_remove_from_group(struct chunk *c, struct monster *mon);
int monster_group_index_new(struct chunk *c);
void monster_add_to_group(struct chunk *c, struct monster *mon,
						  struct monster_group *group);
void monster_group_assign(struct chunk *c, struct monster *mon,
						  struct monster_group_info info, bool loading);
int monster_group_index(struct monster_group *group);
struct monster_group *monster_group_by_index(struct chunk *c, int index);
bool monster_group_change_index(struct chunk *c, int new, int old);
void monster_group_rouse(struct chunk *c, struct monster *mon);
int monster_group_size(struct chunk *c, const struct monster *mon);
struct monster *group_monster_tracking(struct chunk *c,
									   const struct monster *mon);
int monster_group_leader_idx(struct monster_group *group);
struct monster *monster_group_leader(struct chunk *c, struct monster *mon);
void monster_group_new_wandering_flow(struct chunk *c, struct monster *mon,
									  struct loc tgrid);
void monster_groups_verify(struct chunk *c);

#endif /* !MON_GROUP_H */
