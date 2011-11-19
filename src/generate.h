/* generate.h - dungeon generation interface */

#ifndef GENERATE_H
#define GENERATE_H

/*
 * Dungeon generation values
 */
/** Number of rooms to attempt */
#define DUN_ROOMS	     30
/** 1/chance of being a destroyed level */
#define DEST_LEVEL_CHANCE    25
/** 1/chance of being a moria-style level */
#define MORIA_LEVEL_CHANCE   40

/** 
 * 1/chance of being a themed level - higher in wilderness -NRM-
 */
#define THEMED_LEVEL_CHANCE  (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE \
			      ? 180 : 70)


/*
 * Dungeon tunnel generation values
 */
/** 1 in # chance of random direction */
#define DUN_TUN_RND  	30
/**1 in # chance of adjusting direction */
#define DUN_TUN_ADJ  	10
/** Chance of doors at room entrances */
#define DUN_TUN_PEN	35
/** Chance of doors at tunnel junctions */
#define DUN_TUN_JCT	70

/*
 * Dungeon streamer generation values
 */
/** Width of streamers (can sometimes be higher) */
#define DUN_STR_WID	2
/** Number of magma streamers */
#define DUN_STR_MAG	3
/** 1/chance of treasure per magma */
#define DUN_STR_MC	70
/** Number of quartz streamers */
#define DUN_STR_QUA	2
/** 1/chance of treasure per quartz */
#define DUN_STR_QC	35
/** 1/(4 + chance) of altering direction */
#define DUN_STR_CHG	16

/*
 * Dungeon treasure allocation values
 */
/** Amount of objects for rooms */
#define DUN_AMT_ROOM	9
/** Amount of objects for rooms/corridors */
#define DUN_AMT_ITEM	2
/** Amount of treasure for rooms/corridors */
#define DUN_AMT_GOLD	2

/*
 * Hack -- Dungeon allocation "places"
 */
/** Hallway */
#define ALLOC_SET_CORR		1
/** Room */
#define ALLOC_SET_ROOM		2
/** Anywhere */
#define ALLOC_SET_BOTH		3

/*
 * Hack -- Dungeon allocation "types"
 */
/** Rubble */
#define ALLOC_TYP_RUBBLE	1
/** Trap */
#define ALLOC_TYP_TRAP		3
/** Gold */
#define ALLOC_TYP_GOLD		4
/** Object */
#define ALLOC_TYP_OBJECT	5


/**
 * Maximum numbers of rooms along each axis (currently 6x18)
 */
#define MAX_ROOMS_ROW	(DUNGEON_HGT / BLOCK_HGT)
#define MAX_ROOMS_COL	(DUNGEON_WID / BLOCK_WID)

/*
 * Maximal number of room types
 */
#define ROOM_MAX	11

/*
 * Bounds on some arrays used in the "dun_data" structure.
 * These bounds are checked, though usually this is a formality.
 */
#define CENT_MAX	DUN_ROOMS
#define DOOR_MAX	100
#define WALL_MAX	40
#define TUNN_MAX	300
#define STAIR_MAX	30

/**
 * Tree type chances 
 */
#define HIGHLAND_TREE_CHANCE 30

#define CHUNK_EMPTY   { 0 }

/**
 * Height and width for the currently generated level
 *
 * This differs from DUNGEON_HGT (and dungeon_hgt) in that it bounds the part
 * of the level that might actually contain open squares. It will vary from
 * level to level, unlike the constant.
 */
int level_hgt;
int level_wid;


/**
 * Simple structure to hold a map location
 */

typedef struct coord coord;

struct coord {
    byte y;
    byte x;
};

/**
 * Structure to hold all dungeon generation data
 */

typedef struct dun_data dun_data;

struct dun_data {
    /* Array of centers of rooms */
    int cent_n;
    coord cent[CENT_MAX];

    /* Array to store whether rooms are connected or not. */
    bool connected[CENT_MAX];

    /* Array of possible door locations */
    int door_n;
    coord door[DOOR_MAX];

    /* Array of wall piercing locations */
    int wall_n;
    coord wall[WALL_MAX];

    /* Array of tunnel grids */
    int tunn_n;
    coord tunn[TUNN_MAX];

    /* Array of good potential stair grids */
    int stair_n;
    coord stair[STAIR_MAX];

    /* Number of blocks along each axis */
    int row_rooms;
    int col_rooms;

    /* Array to store block usage */
    int room_map[MAX_ROOMS_ROW][MAX_ROOMS_COL];
};

extern dun_data *dun;
extern bool moria_level;
extern bool underworld;
extern int wild_vaults;
extern char mon_symbol_at_depth[12][13];

extern bool build_themed_level(void);
extern char *mon_restrict(char symbol, byte depth, bool * ordered,
			  bool unique_ok);
extern void get_chamber_monsters(int y1, int x1, int y2, int x2);
extern void spread_monsters(char symbol, int depth, int num, int y0, int x0,
			    int dy, int dx);
extern void general_monster_restrictions(void);
extern void get_vault_monsters(char racial_symbol[], byte vault_type, const char *data,
			       int y1, int y2, int x1, int x2);

extern void correct_dir(int *row_dir, int *col_dir, int y1, int x1, int y2,
			int x2);
extern void adjust_dir(int *row_dir, int *col_dir, int y1, int x1, int y2,
		       int x2);

extern void generate_mark(int y1, int x1, int y2, int x2, int flg);
extern void place_unlocked_door(int y, int x);
extern void place_closed_door(int y, int x);
extern void destroy_level(bool new_level);
extern bool passable(int feat);
extern bool generate_starburst_room(int y1, int x1, int y2, int x2, bool light,
				    int feat, bool special_ok);
extern bool build_vault(int y0, int x0, int ymax, int xmax, const char *data,
			bool light, bool icky, byte vault_type);
extern bool room_build(int room_type);

int next_to_walls(int y, int x);
void new_player_spot(void);
void place_random_stairs(int y, int x);
void place_secret_door(int y, int x);
void place_random_door(int y, int x);
void alloc_stairs(int feat, int num, int walls);
void alloc_object(int set, int typ, int num);

int chunk_find(chunk_ref ref);
void chunk_read(int idx, int y_offset, int x_offset);
int chunk_store(int y_offset, int x_offset, u16b region, byte z_pos, byte y_pos,
		byte x_pos, bool write);
int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset);
void chunk_adjacent_data(chunk_ref *ref, int z_offset, int y_offset, 
			 int x_offset);
void chunk_generate(chunk_ref ref, int y_offset, int x_offset);
void chunk_change(int z_offset, int y_offset, int x_offset);

extern bool no_vault(void);
extern void plain_gen(chunk_ref ref, int y_offset, int x_offset);
extern void mtn_gen(void);
extern void mtntop_gen(void);
extern void forest_gen(void);
extern void swamp_gen(void);
extern void desert_gen(void);
extern void river_gen(void);
extern void valley_gen(void);
extern void cave_gen(void);
extern void generate_cave(void);

#endif /* !GENERATE_H */

