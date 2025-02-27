/**
 * \file mon-make.h
 * \brief Structures and functions for monster creation / deletion.
 *
 * Copyright (c) 1997-2007 Ben Harrison, James E. Wilson, Robert A. Koeneke
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

#ifndef MONSTER_MAKE_H
#define MONSTER_MAKE_H

#include "game-world.h"
#include "monster.h"

extern struct monster *monsters;
extern struct monster_group **monster_groups;
extern uint16_t mon_max;
extern uint16_t mon_cnt;
extern int mon_current;

void monsters_init(void);
void monsters_free(void);
struct monster *monster(int idx);
void delete_monster_idx(int m_idx);
void delete_monster(struct loc grid);
void monster_index_move(int i1, int i2);
void compact_monsters(int num_to_compact);
void delete_temp_monsters(void);
void wipe_mon_list(void);
void get_mon_num_prep(bool (*get_mon_num_hook)(struct monster_race *race));
struct monster_race *get_mon_num(int level, enum biome_type biome,
								 bool special, bool allow_non_smart,
								 bool vault);
void set_monster_place_current(void);
int16_t place_monster(struct chunk *c, struct loc grid, struct monster *mon,
	uint8_t origin);
int mon_hp(const struct monster_race *race, aspect hp_aspect);
bool place_new_monster_one(struct chunk *c, struct loc grid,
						   struct monster_race *race, bool sleep,
						   bool ignore_depth, 
						   struct monster_group_info group_info,
						   uint8_t origin);
bool place_new_monster(struct chunk *c, enum biome_type biome, struct loc grid,
	struct monster_race *race, bool sleep, bool group_ok,
	struct monster_group_info group_info, uint8_t origin);
bool pick_and_place_monster(struct chunk *c, enum biome_type biome,
							struct loc grid, int depth, bool sleep,
							bool group_okay, uint8_t origin);
void place_monster_by_flag(struct chunk *c, enum biome_type biome,
						   struct loc grid, int flg1, int flg2,
						   bool allow_unique, int max_depth, bool spell);
void place_monster_by_letter(struct chunk *c, enum biome_type biome,
							 struct loc grid, char ch,
							 bool allow_unique, int max_depth);
bool pick_and_place_monster_on_stairs(struct chunk *c, struct player *p,
									  enum biome_type biome, bool sleep,
									  int depth, bool force_undead);
bool pick_and_place_distant_monster(struct chunk *c, struct player *p,
									enum biome_type biome, bool sleep,
									int depth);

#endif /* MONSTER_MAKE_H */
