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
 * Maximum x and y values for square miles
 */
#define MAX_Y_REGION (12 * 49)
#define MAX_X_REGION (15 * 49)

/**
 * Codes for the different realms
 */
enum {
	#define REALM(a, b) REALM_##a,
	#include "list-realms.h"
	#undef REALM
	REALM_MAX
};

#define REALM_SIZE                FLAG_SIZE(REALM_MAX)

#define realm_has(f, flag)        flag_has_dbg(f, REALM_SIZE, flag, #f, #flag)
#define realm_next(f, flag)       flag_next(f, REALM_SIZE, flag)
#define realm_is_empty(f)         flag_is_empty(f, REALM_SIZE)
#define realm_is_full(f)          flag_is_full(f, REALM_SIZE)
#define realm_is_inter(f1, f2)    flag_is_inter(f1, f2, REALM_SIZE)
#define realm_is_subset(f1, f2)   flag_is_subset(f1, f2, REALM_SIZE)
#define realm_is_equal(f1, f2)    flag_is_equal(f1, f2, REALM_SIZE)
#define realm_on(f, flag)         flag_on_dbg(f, REALM_SIZE, flag, #f, #flag)
#define realm_off(f, flag)        flag_off(f, REALM_SIZE, flag)
#define realm_wipe(f)             flag_wipe(f, REALM_SIZE)
#define realm_setall(f)           flag_setall(f, REALM_SIZE)
#define realm_negate(f)           flag_negate(f, REALM_SIZE)
#define realm_copy(f1, f2)        flag_copy(f1, f2, REALM_SIZE)
#define realm_union(f1, f2)       flag_union(f1, f2, REALM_SIZE)
#define realm_inter(f1, f2)       flag_inter(f1, f2, REALM_SIZE)
#define realm_diff(f1, f2)        flag_diff(f1, f2, REALM_SIZE)


/**
 * Codes for the different surface biomes
 */
enum biome_type {
	#define BIOME(a, b) BIOME_##a = b,
	#include "list-biomes.h"
	#undef BIOME
};

#define SMELL_STRENGTH 80

/**
 * Information about landmarks
 */
struct landmark {
	struct landmark *next;
	unsigned int lidx;
	char *name;
	char *profile;
	char *message;
	char *text;

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

	uint8_t realm;       	           /**< Realm region lies in */
	uint8_t danger;       	           /**< Region danger */
	uint16_t height;       	           /**< Region height */
	uint16_t width;       	  	       /**< Region width */
	uint16_t y_offset;                 /**< Region y location */
	uint16_t x_offset;                 /**< Region x location */
};

extern uint16_t daycount;

/**
 * Description of a river mile
 */
enum river_part {
	RIVER_NONE,
	RIVER_SOURCE,		/**< Source of the river */
	RIVER_SPLIT,		/**< A split in the river to form two stretches */
	RIVER_STRETCH,		/**< A simple piece of a river stretch */
	RIVER_JOIN,			/**< A place where two stretches join into one */
	RIVER_UNDERGROUND,	/**< The river goes underground */
	RIVER_EMERGE,		/**< The river emerges from underground */
	RIVER_LAKE,			/**< The river flows into a lake */
	RIVER_SEA			/**< The river flows into the sea */
};

struct map_square;
struct square_mile;

/**
 * Grid making up part of a river in a chunk
 *
 * Note that the actual terrain (shallow or deep water) is calculated after
 * this is placed, so there is no need to record it here
 */
struct river_grid {
	struct river_grid *next;
	struct loc grid;
};

/**
 * Grids making up part of a river in a chunk
 *
 * Note that the actual terrain (shallow or deep water) is calculated after
 * this is placed, so there is no need to record it here
 */
struct river_piece {
	struct river_grid *grids;	/**< Set of river grids in this chunk */
	int num_grids;				/**< Number of river grids in this chunk */
	enum direction dir;			/**< Direction this piece of river is flowing */
};

/**
 * Information about a piece of river at a square mile
 */
struct river_mile {
	struct river *river;			/**< The river we're a part of */
	enum river_part part;			/**< Description of this river mile */
	struct river_stretch *stretch;	/**< The stretch we're in */
	struct square_mile *sq_mile;	/**< The square mile we're in */
	struct river_mile *upstream;	/**< The river mile that flows into us */
	struct river_mile *downstream;	/**< The river mile we flow into */
	struct loc entry;				/**< The chunk we enter the square mile */
	struct loc exit;				/**< The chunk we leave the square mile */

	struct river_mile *next;		/**< Next river mile in this square mile */
};

/**
 * Information about river stretches, which is a contiguous piece of river
 * made up of river_miles
 */
struct river_stretch {
	struct river *river;		/**< The river we're a part of */
	int index;					/**< Index of this stretch in the river */
	struct river_mile *miles;	/**< List of river miles forming the stretch */
	struct river_stretch *in1;	/**< Stretch flowing into this one, if any */
	struct river_stretch *in2;	/**< Second stretch flowing into this one */
	struct river_stretch *out1;	/**< Stretch flowing out of this one, if any */
	struct river_stretch *out2;	/**< Second stretch flowing out of this one */

	struct river_stretch *next;	/**< Next stretch in the list for this river */
};

/**
 * Information about rivers
 */
struct river {
	char *name;						/**< The river's name */
	char *filename;					/**< The river's filename */
	int index;						/**< Index in river_info */
	struct river_stretch *stretch;	/**< The river's list of stretches */
	char *join;						/**< Name of the river this flows into */
	struct river *next;				/**< River this flows into */
};

/**
 * Information about a piece of road at a chunk
 */
struct road_chunk {
	int map_y;			/**< Map y coordinate of road chunk */
	int map_x;			/**< Map x coordinate of road chunk */
	uint16_t width;		/**< Road width */
};

/**
 * Information about how a road crosses the border of a chunk
 *
 * Note that this structure holds information about one side only; roads
 * crossing a corner will require two overlapping road_edges
 */
struct road_edge {
	struct road_edge *next;

	struct road *road;		/**< The road */
	enum direction side;	/**< Side of the chunk crossed */
	uint8_t start;			/**< Smallest crossing coordinate */
	uint8_t finish;			/**< Largest crossing coordinate */
};

/**
 * Information about roads
 */
struct road {
	char *name;
	struct map_square *map_squares;
	struct road_chunk *chunks;
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
};

/**
 * Information about square miles.
 *
 * A square mile contains 400 regular size chunks (20x20), and there are
 * 49x49 (= 2401) of them to a map square.  Each square mile is represented
 * as a single grid in region.txt.
 */
struct square_mile {
	enum biome_type biome;			/**< Biome of this square mile */
	struct world_region *region;	/**< The region containing us */
	struct map_square map_square;	/**< The map square containing us */
	struct loc map_square_grid;		/**< Our position (49x49) in map_square */
	struct loc map_grid;			/**< Our position in the whole map */
	struct river_mile *river_miles;	/**< List of river miles we contain */
	bool mapped;					/**< Our rivers/roads have been plotted */
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
	uint8_t feat;
	bitflag info[SQUARE_SIZE];
	enum biome_type type;
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
	uint16_t place;			/**< Index of this chunk */
	int32_t turn;			/**< Turn this chunk was created */
	uint16_t region;		/**< Region the chunk is from */
	int16_t z_pos;			/**< Depth of the chunk below ground */
	uint16_t y_pos;			/**< y position of the chunk */
	uint16_t x_pos;			/**< x position of the chunk */
	struct chunk *chunk;	/**< The actual chunk */
	struct chunk *p_chunk;	/**< The player's knowledge of the chunk */
	uint32_t gen_loc_idx;	/**< The chunk index in the generated locations list */
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
	enum biome_type type;	/**< Biome of the location */
    int x_pos;			/**< x position of the chunk */
    int y_pos;			/**< y position of the chunk */
    int z_pos;			/**< Depth of the chunk below ground */
	uint32_t seed;			/**< RNG seed for generating the chunk repeatably */
    struct terrain_change *change;	/**< Changes made since generation */
    struct connector *join;	/**< Information for generating adjoining chunks */
	struct river_piece *river_piece;	/**< Piece of river in the location */
	struct road_edge *road_edge;	/**< Road edge crossing data */
};

extern uint32_t seed_randart;
extern uint32_t seed_flavor;
extern int32_t turn;
extern bool character_generated;
extern bool character_dungeon;
extern const uint8_t extract_energy[8];
extern struct world_region *region_info;
extern struct square_mile **square_miles;
extern struct landmark *landmark_info;
extern struct gen_loc *gen_loc_list;
extern struct river *river_info;
extern uint16_t chunk_max;
extern uint16_t chunk_cnt;
extern struct chunk_ref *chunk_list;
extern uint32_t gen_loc_max;
extern uint32_t gen_loc_cnt;
extern struct gen_loc *gen_loc_list;

void gen_loc_list_init(void);
void gen_loc_list_cleanup(void);
bool gen_loc_find(int x_pos, int y_pos, int z_pos, int *below, int *above);
void gen_loc_make(int x_pos, int y_pos, int z_pos, int idx);
struct square_mile *square_mile(wchar_t letter, int number, int y, int x);
bool is_daytime(void);
bool outside(void);
bool is_daylight(void);
bool is_night(void);
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
