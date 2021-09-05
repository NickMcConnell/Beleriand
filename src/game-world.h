/**
 * \file game-world.h
 * \brief Game core management of the game world
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

#ifndef GAME_WORLD_H
#define GAME_WORLD_H

#include "cave.h"

enum {
	HEALTH_DEAD,
	HEALTH_ALMOST_DEAD,
	HEALTH_BADLY_WOUNDED,
	HEALTH_WOUNDED,
	HEALTH_SOMEWHAT_WOUNDED,
	HEALTH_UNHURT
};

/**
 * Size increment of the generated locations array
 */
#define GEN_LOC_INCR 128

/**
 * Maximum x and y values for region grids 
 */
#define MAX_Y_REGION 588
#define MAX_X_REGION 735

struct level {
	int depth;
	char *name;
	char *up;
	char *down;
	struct level *next;
};

#define SMELL_STRENGTH 80

/**
 * Information about regions
 */
struct world_region {
    struct world_region *next;
    unsigned int index;
    char *name;
    char *message;
    char *text;

    byte danger;       	           /**< Region danger */
    u16b height;       	           /**< Region height */
    u16b width;       	           /**< Region width */
    u16b y_offset;                 /**< Region y location */
    u16b x_offset;                 /**< Region x location */

};

extern u16b daycount;
extern u32b seed_randart;
extern u32b seed_flavor;
extern s32b turn;
extern bool character_generated;
extern bool character_dungeon;
extern const byte extract_energy[8];
extern struct level *maps;
extern struct level *world;
extern struct world_region *region_info;
extern char **region_terrain;
extern struct gen_loc *gen_loc_list;
extern u32b gen_loc_max;
extern u32b gen_loc_cnt;

bool gen_loc_find(int x_pos, int y_pos, int z_pos, int *lower, int *upper);
void gen_loc_make(int x_pos, int y_pos, int z_pos, int idx);
struct level *level_by_name(const char *name);
struct level *level_by_depth(int depth);
bool is_daytime(void);
int turn_energy(int speed);
int regen_amount(int turn_number, int max, int period);
int health_level(int current, int max);
void play_ambient_sound(void);
void update_flow(struct chunk *c, struct flow *flow, struct monster *mon);
int flow_dist(struct flow flow, struct loc grid);
int get_scent(struct chunk *c, struct loc grid);
void process_world(struct chunk *c);
void on_new_level(void);
void process_player(void);
void run_game_loop(void);

#endif /* !GAME_WORLD_H */
