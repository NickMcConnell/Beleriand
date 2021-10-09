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

/**
 * Size increment of the generated locations array
 */
#define GEN_LOC_INCR 128

/**
 * Maximum x and y values for region grids 
 */
#define MAX_Y_REGION 588
#define MAX_X_REGION 735

/**
 * Codes for the different types of wilderness
 */
enum wild_type {
	WILD_PLAIN = 0x2e,			/**< . */
	WILD_FOREST = 0x2b,			/**< + */
	WILD_LAKE = 0x2d,			/**< - */
	WILD_SNOW = 0x2a,			/**< * */
	WILD_DESERT = 0x2f,			/**< ? */
	WILD_DARK = 0x7c,			/**< | */
	WILD_MOUNTAIN = 0x5e,		/**< ^ */
	WILD_MOOR = 0x2c,			/**< , */
	WILD_SWAMP = 0x5f,			/**< _ */
	WILD_IMPASS = 0x58,			/**< X */
	WILD_TOWN = 0x3d,			/**< = */
	WILD_OCEAN = 0x7e,			/**< ~ */
};

/**
 * Information about landmarks
 */
struct landmark {
	struct landmark *next;
	unsigned int lidx;
	char *name;
	char *message;
	char *text;

	int map_z;         /**< Map z coordinate of landmark */
	int map_y;         /**< Map y coordinate of landmark */
	int map_x;         /**< Map x coordinate of landmark */
	int height;        /**< Number of chunks high */
	int width;         /**< Number of chunks wide */
};

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

enum river_part {
	RIVER_NONE,
	RIVER_SOURCE,
	RIVER_SPLIT,
	RIVER_STRETCH,
	RIVER_JOIN,
	RIVER_UNDERGROUND,
	RIVER_EMERGE,
	RIVER_LAKE,
	RIVER_SEA
};

struct map_square;
struct square_mile;

/**
 * Information about a piece of river at a chunk
 */
struct river_chunk {
	int map_y;         /**< Map y coordinate of river chunk */
	int map_x;         /**< Map x coordinate of river chunk */
	u16b width;        /**< River width */
};

/**
 * Information about a piece of river at a square mile
 */
//struct river_mile {
//	struct square_mile *mile;
//	enum river_part part;
//	struct river_stretch *in1;
//	struct river_stretch *in2;
//	struct river_stretch *out1;
//	struct river_stretch *out2;
//	struct river *river;
//};

/**
 * Information about river stretches
 */
struct river_stretch {
	int index;
	struct square_mile *miles;
	struct river_stretch *in1;
	struct river_stretch *in2;
	struct river_stretch *out1;
	struct river_stretch *out2;
	//struct river_mile *start;
	//struct river_mile *end;
	//struct river *river;
	//struct river *big;
	struct river_stretch *next;
};

/**
 * Information about rivers
 */
struct river {
	char *name;
	char *filename;
	int index;
	struct river_stretch *stretch;
	//struct map_square *map_squares;
	//struct river_chunk *chunks;
	char *join;
	struct river *next;
};

/**
 * Information about a piece of road at a chunk
 */
struct road_chunk {
	int map_y;         /**< Map y coordinate of road chunk */
	int map_x;         /**< Map x coordinate of road chunk */
	u16b width;        /**< Road width */
};

/**
 * Information about roads
 */
struct road {
	char *name;
	struct map_square *map_squares;
	struct river_chunk *chunks;
};

/**
 * Information about map squares.
 *
 * A map square is 49 miles on a side (should be 50, but having it 7x7 was
 * convenient), and the map is 12 map squares down and 15 across.
 *
 * Reference is the second Silmarillion map, (The War of the Jewels, pp182-185)
 */
struct map_square {
	char letter;				/**< y coordinate, letters A-M excluding I */
	int number;					/**< x coordinate, numbers 1-15 */
	//struct world_region *regions;	/**< Regions overlapping the map square */
	//struct river *rivers;		/**< Rivers passing through the map square */
	//struct road *roads;			/**< Roads passing through the map square */
};

/**
 * Information about square miles.
 *
 * A square mile contains 100 regular size chunks (10x10), and there are
 * 49x49 (= 2401) of them to a map square.  Each square mile is represented
 * as a single grid in region.txt.
 */
struct square_mile {
	struct map_square map_square;	/**< The map square containing us */
	struct loc map_square_grid;		/**< Our position (49x49) in map_square */
	//struct world_region *region;	/**< Region containing us */
	enum river_part river_type;		/**< Part of a river, if any */
	//struct river *rivers;			/**< Rivers passing through us */
	//struct road *roads;				/**< Roads passing through us */
	struct square_mile *next;
};

/**
 * Terrain information for a grid for use in generation of adjacent chunks
 *
 * When a chunk is generated, it first checks for which adjacent ones are
 * already generated, and adjusts generation to make sure that connection
 * is consistent.  Then for any boundaries (horizontal edges, or stairs and
 * chasms) where there was no existing chunk, the connector information is
 * written to the generated location for use by those chunks in the future.
 */
struct connector {
	struct loc grid;
	byte feat;
	bitflag info[SQUARE_SIZE];
	enum wild_type type;
	struct connector *next;
};

/**
 * Location data for a (standard 22x22) chunk
 *
 * A list of the 256 most recent chunks is kept to be swapped back in as the
 * playing arena changes.  When a new chunk needs to be stored, the oldest
 * will be removed; if needed again it will need to be re-generated from
 * the generated locations list.
 */
struct chunk_ref {
	u16b place;			/**< Index of this chunk */
	s32b turn;			/**< Turn this chunk was created */
	u16b region;		/**< Region the chunk is from */
	u16b z_pos;			/**< Depth of the chunk below ground */
	u16b y_pos;			/**< y position of the chunk */
	u16b x_pos;			/**< x position of the chunk */
	struct chunk *chunk;	/**< The actual chunk */
	struct chunk *p_chunk;	/**< The player's knowledge of the chunk */
	u32b gen_loc_idx;	/**< The chunk index in the generated locations list */
	int adjacent[11];	/**< Adjacent chunks */
};

/**
 * A change to terrain made after generation
 *
 * This is used to store any changes that happen to the terrain of a chunk
 * after its initial generation, so they can be restored if the chunk needs
 * to be reloaded from the generated locations list.
 */
struct terrain_change {
	struct loc grid;
    int feat;
    struct terrain_change *next;
};

/**
 * Generation data for a generated location
 *
 * This is stored for every chunk the player has visited, and enables chunks
 * to be restored if the have aged off from the chunk list.
 */
struct gen_loc {
	enum wild_type type;/**< Wilderness type of the location */
	int x_pos;			/**< x position of the chunk */
	int y_pos;			/**< y position of the chunk */
	int z_pos;			/**< Depth of the chunk below ground */
	u32b seed;			/**< RNG seed for generating the chunk repeatably */
	struct terrain_change *change;	/**< Changes made since generation */
	struct connector *join;	/**< Information for generating adjoining chunks */
};

extern u32b seed_randart;
extern u32b seed_flavor;
extern s32b turn;
extern bool character_generated;
extern bool character_dungeon;
extern const byte extract_energy[200];
extern struct world_region *region_info;
extern char **region_terrain;
extern struct landmark *landmark_info;
extern struct river *river_info;
extern u16b chunk_max;
extern u16b chunk_cnt;
extern struct chunk_ref *chunk_list;
extern u32b gen_loc_max;
extern u32b gen_loc_cnt;
extern struct gen_loc *gen_loc_list;

void gen_loc_list_init(void);
void gen_loc_list_cleanup(void);
bool gen_loc_find(int x_pos, int y_pos, int z_pos, int *lower, int *upper);
void gen_loc_make(int x_pos, int y_pos, int z_pos, int idx);
bool no_vault(int place);
bool is_daytime(void);
bool outside(void);
bool is_daylight(void);
bool is_night(void);
int turn_energy(int speed);
void play_ambient_sound(void);
void process_world(struct chunk *c);
void on_new_level(void);
void process_player(void);
void run_game_loop(void);

#endif /* !GAME_WORLD_H */
