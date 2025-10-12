/**
 * \file generate.h
 * \brief Dungeon generation.
 */


#ifndef GENERATE_H
#define GENERATE_H

#include "game-world.h"
#include "monster.h"

#if  __STDC_VERSION__ < 199901L
#define ROOM_LOG  if (OPT(player, cheat_room)) msg
#else
#define ROOM_LOG(...) if (OPT(player, cheat_room)) msg(__VA_ARGS__);
#endif

/**
 * Constants for generation.  Note that MAX_CHUNKS needs to be at least three
 * times ARENA_CHUNKS squared to allow proper saving off and reloading of
 * chunks from the chunk_list.
 */
#define ARENA_CHUNKS 3
#define ARENA_SIDE (CHUNK_SIDE * ARENA_CHUNKS)
#define WIDEN_RATIO 25			/* River miles length per extra grid width */
#define MAX_CHUNKS 256
#define CHUNK_TEMP -2
#define CHUNK_CUR -1

/**
 * Dungeon allocation places and types, used with alloc_object().
 */
enum
{
	SET_CORR = 0x01, /*!< Hallway */
	SET_ROOM = 0x02, /*!< Room */
	SET_BOTH = 0x03 /*!< Anywhere */
};

enum
{
	TYP_RUBBLE,	/*!< Rubble */
	TYP_OBJECT	/*!< Object */
};

/**
 * Flag for room types
 */
enum {
	ROOMF_NONE,
	#define ROOMF(a, b) ROOMF_##a,
	#include "list-room-flags.h"
	#undef ROOMF
};

#define ROOMF_SIZE FLAG_SIZE(ROOMF_MAX)

#define roomf_has(f, flag) flag_has_dbg(f, ROOMF_SIZE, flag, #f, #flag)
#define roomf_next(f, flag) flag_next(f, ROOMF_SIZE, flag)
#define roomf_count(f) flag_count(f, ROOMF_SIZE)
#define roomf_is_empty(f) flag_is_empty(f, ROOMF_SIZE)
#define roomf_is_full(f) flag_is_full(f, ROOMF_SIZE)
#define roomf_is_inter(f1, f2) flag_is_inter(f1, f2, ROOMF_SIZE)
#define roomf_is_subset(f1, f2) flag_is_subset(f1, f2, ROOMF_SIZE)
#define roomf_is_equal(f1, f2) flag_is_equal(f1, f2, ROOMF_SIZE)
#define roomf_on(f, flag) flag_on_dbg(f, ROOMF_SIZE, flag, #f, #flag)
#define roomf_off(f, flag) flag_off(f, ROOMF_SIZE, flag)
#define roomf_wipe(f) flag_wipe(f, ROOMF_SIZE)
#define roomf_setall(f) flag_setall(f, ROOMF_SIZE)
#define roomf_negate(f) flag_negate(f, ROOMF_SIZE)
#define roomf_copy(f1, f2) flag_copy(f1, f2, ROOMF_SIZE)
#define roomf_union(f1, f2) flag_union(f1, f2, ROOMF_SIZE)
#define roomf_inter(f1, f2) flag_iter(f1, f2, ROOMF_SIZE)
#define roomf_diff(f1, f2) flag_diff(f1, f2, ROOMF_SIZE)


/**
 * Structure to hold the corners of a room
 */
struct rectangle {
	struct loc top_left;
	struct loc bottom_right;
};

struct area_profile {
	struct area_profile *next;

	const char *name;
	int frequency;
	int attempts;
	int feat;
	random_value size;
};

struct formation_profile {
	struct formation_profile *next;

	const char *name;
	char *feats;
	int num_feats;
	int proportion;
	random_value size;
};

struct surface_profile {
    struct surface_profile *next;

    const char *name;
	enum biome_type code;
	char *base_feats;
	int num_base_feats;
	struct area_profile *areas;
	struct formation_profile *formations;
	int num_form_types;
};

extern struct surface_profile *surface_profiles;

/**
 * Structure to hold all "dungeon generation" data
 */
struct dun_data {
    /*!< The profile used to generate the level */
    const struct cave_profile *profile;

    /*!< Array of centers of rooms */
    int cent_n;
    struct loc *cent;

    /*!< Array (cent_n elements) for counts of marked entrance points */
    int *ent_n;

    /*!< Array of arrays (cent_n by ent_n[i]) for locations of marked entrance points */
    struct loc **ent;

    /*!< Lookup for room number of a room entrance by (y,x) for the entrance */
    int **ent2room;

    /*!< Array of possible door locations */
    int door_n;
    struct loc *door;

    /*!< Array of wall piercing locations */
    int wall_n;
    struct loc *wall;

    /*!< Array of tunnel grids */
    int tunn_n;
    struct loc *tunn;

    /*!< Number of grids in each block (vertically) */
    int block_hgt;

    /*!< Number of grids in each block (horizontally) */
    int block_wid;

    /*!< Number of blocks along each axis */
    int row_blocks;
    int col_blocks;

    /*!< Array of which blocks are used */
    bool **room_map;

    /*!< Info for connecting between levels */
    struct connector *join;

    /*!< The connection information to use for the next staircase room */
    struct connector *curr_join;

    /*!< The number of staircase rooms */
    int nstair_room;

    /*!< Whether or not this is a quest level */
    bool quest;

    /*!< Whether this is the first time this level has been generated */
    bool first_time;

    /*!< Whether to keep fixed room parameters to build consistent rooms */
    bool fix_room_parameters;

    /*!< Saved seed value for quick random number generator */
    uint32_t seed;
};


struct tunnel_profile {
    const char *name;
    int rnd; /*!< % chance of choosing random direction */
    int chg; /*!< % chance of changing direction */
    int con; /*!< % chance of extra tunneling */
    int pen; /*!< % chance of placing doors at room entrances */
    int jct; /*!< % chance of doors at tunnel junctions */
};

struct streamer_profile {
    const char *name;
    int den; /*!< Density of streamers */
    int rng; /*!< Width of streamers */
    int qua; /*!< Number of quartz streamers */
};

/*
 * cave_builder is a function pointer which builds a level.
 */
typedef struct chunk * (*cave_builder) (struct player *p);


struct cave_profile {
    struct cave_profile *next;

    const char *name;
	char biome;
    cave_builder builder;	/*!< Function used to build the level */
    int block_size;			/*!< Default height and width of dungeon blocks */
    int dun_rooms;			/*!< Maximum number of rooms */
    int dun_unusual;		/*!< Level/chance of unusual room */
    int max_rarity;			/*!< Max number of room generation rarity levels */
    int n_room_profiles;	/*!< Number of room profiles */
    struct tunnel_profile tun;		/*!< Used to build tunnels */
    struct streamer_profile str;	/*!< Used to build mineral streamers*/
    struct room_profile *room_profiles;	/*!< Used to build rooms */
    int alloc;				/*!< Allocation weight for this profile */
};


/**
 * room_builder is a function pointer which builds rooms in the cave given
 * anchor coordinates.
 */
typedef bool (*room_builder) (struct chunk *c, struct loc centre);


/**
 * This tracks information needed to generate the room, including the room's
 * name and the function used to build it.
 */
struct room_profile {
    struct room_profile *next;

    const char *name;
    room_builder builder;	/*!< Function used to build fixed size rooms */
    int height, width;		/*!< Space required in grids */
    int level;				/*!< Minimum dungeon level */
    int rarity;				/*!< How unusual this room is */
    int cutoff;				/*!< Upper limit of 1-100 roll for room gen */
};


/*
 * Information about vault generation
 */
struct vault {
    struct vault *next; 		/*!< Pointer to next vault template */

    char *name;         		/*!< Vault name */
	int16_t index;      		/*!< Vault index */
    char *text;         		/*!< Grid by grid description of vault layout */
    char *typ;					/*!< Vault type */
    bitflag flags[ROOMF_SIZE];	/*!< Vault flags */
    uint8_t hgt;				/*!< Vault height */
    uint8_t wid;				/*!< Vault width */
    uint8_t depth;				/*!< Vault depth */
    uint32_t rarity;				/*!< Vault rarity */
    bool forge;					/*!< Is there a forge in it? */
};

/*
 * Information about settlement generation
 */
struct settlement {
    struct settlement *next; 		/*!< Pointer to next settlement template */

    char *name;         		/*!< Settlement name */
	int16_t index;      		/*!< Settlement index */
    char *text;         		/*!< Grid by grid settlement layout */
    char *typ;					/*!< Settlement type */
    bitflag flags[ROOMF_SIZE];	/*!< Settlement flags */
    uint8_t hgt;				/*!< Settlement height */
    uint8_t wid;				/*!< Settlement width */
    uint8_t depth;				/*!< Settlement depth */
    uint32_t rarity;				/*!< Settlement rarity */
    bool forge;					/*!< Is there a forge in it? */
};



/**
 * Constants for working with random symmetry transforms
 */
#define SYMTR_FLAG_NONE (0)
#define SYMTR_FLAG_NO_ROT (1)
#define SYMTR_FLAG_NO_REF (2)
#define SYMTR_FLAG_FORCE_REF (4)
#define SYMTR_MAX_WEIGHT (32768)

extern struct dun_data *dun;
extern struct vault *vaults;
extern struct settlement *settlements;
extern struct room_template *room_templates;

/* generate.c */
void prepare_next_level(struct player *p);
int get_room_builder_count(void);
int get_room_builder_index_from_name(const char *name);
const char *get_room_builder_name_from_index(int i);
int get_level_profile_index_from_name(const char *name);
const char *get_level_profile_name_from_index(int i);

/* gen-cave.c */
struct chunk *angband_gen(struct player *p);
struct chunk *elven_gen(struct player *p);
struct chunk *dwarven_gen(struct player *p);
struct chunk *throne_gen(struct player *p);
bool build_landmark(struct chunk *c, struct landmark *landmark, int map_y,
					int map_x, int y_coord, int x_coord);

/* gen-river.c */
void map_river_miles(struct square_mile *sq_mile);

/* gen-surface.c */
void surface_gen(struct chunk *c, struct chunk_ref *ref, int y_coord,
				 int x_coord, struct connector *first);

/* gen-chunk.c */
void symmetry_transform(struct loc *grid, int y0, int x0, int height, int width,
	int rotate, bool reflect);
void get_random_symmetry_transform(int height, int width, int flags,
	int transpose_weight, int *rotate, bool *reflect,
	int *theight, int *twidth);
int calc_default_transpose_weight(int height, int width);
bool chunk_copy(struct chunk *dest, struct player *p, struct chunk *source,
	 int y0, int x0, int rotate, bool reflect);
void chunk_read(struct player *p, int idx, int y_coord, int x_coord);
void chunk_validate_objects(struct chunk *c);
int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset);
int find_region(int y_pos, int x_pos);
void chunk_offset_data(struct chunk_ref *ref, int z_offset, int y_offset,
						 int x_offset);
void connectors_free(struct connector *join);
void chunk_list_init(void);
void chunk_list_cleanup(void);
int chunk_find(struct chunk_ref ref);
int chunk_store(int y_coord, int x_coord, uint16_t region, uint16_t z_pos,
				uint16_t y_pos, uint16_t x_pos, uint32_t gen_loc_idx,
				bool write);
int chunk_fill(struct chunk *c, struct chunk_ref *ref, int y_coord,
			   int x_coord);
int chunk_get_centre(void);
void chunk_change(struct player *p, int z_offset, int y_offset, int x_offset);


/* gen-room.c */
struct vault *random_vault(int depth, const char *typ, bool forge);
void generate_mark(struct chunk *c, struct point_set *grids, int flag);
void fill_point_set(struct chunk *c, struct point_set *grids, int feat,
					int flag);
void fill_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat,
					int flag);
void draw_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat, 
					int flag, bool overwrite_perm);
void fill_ellipse(struct chunk *c, int y0, int x0, int y_radius, int x_radius,
				  int feat, int flag, bool light);
void set_marked_granite(struct chunk *c, struct loc grid, int flag);
void set_bordering_walls(struct chunk *c, int y1, int x1, int y2, int x2);
extern bool generate_starburst_room(struct chunk *c, struct point_set *set,
									int y1, int x1, int y2, int x2,
									bool light, int feat, bool special_ok);
bool build_vault(struct chunk *c, struct loc *centre, bool *rotated,
				 struct vault *v);
void unset_room_parameters(void);
bool build_staircase(struct chunk *c, struct loc centre);
bool build_simple(struct chunk *c, struct loc centre);
bool build_circular(struct chunk *c, struct loc centre);
bool build_elliptical(struct chunk *c, struct loc centre);
bool build_overlap(struct chunk *c, struct loc centre);
bool build_crossed(struct chunk *c, struct loc centre);
bool build_room_of_chambers(struct chunk *c, struct loc centre);
bool build_interesting(struct chunk *c, struct loc centre);
bool build_lesser_vault(struct chunk *c, struct loc centre);
bool build_greater_vault(struct chunk *c, struct loc centre);
bool build_throne(struct chunk *c, struct loc centre);
bool build_gates(struct chunk *c, struct loc centre);
bool room_build(struct chunk *c, struct loc centre,
				struct room_profile profile);


/* gen-util.c */
extern uint8_t get_angle_to_grid[41][41];

struct point_set *get_rectangle_point_set(int y1, int x1, int y2, int x2);
struct point_set *get_ellipse_point_set(int y0, int x0, int y_rad, int x_rad);
struct loc get_rotated_grid(struct loc start, int sin, int cos, int mult);
int *cave_find_init(struct point_set *points, struct loc top_left,
					struct loc bottom_right);
void cave_find_reset(int *state);
bool cave_find_get_grid(int *index, struct loc *grid, int *state);

bool cave_find_in_range(struct chunk *c, struct loc *grid, struct loc top_left,
	struct loc bottom_right, square_predicate pred);
bool cave_find_in_point_set(struct chunk *c, struct loc *grid,
							struct point_set *points, square_predicate pred);
bool cave_find(struct chunk *c, struct loc *grid, square_predicate pred);
bool find_empty(struct chunk *c, struct loc *grid);
bool find_empty_range(struct chunk *c, struct loc *grid, struct loc top_left,
					  struct loc bottom_right);
bool find_nearby_grid(struct chunk *c, struct loc *grid, struct loc centre,
					  int yd, int xd);
bool find_nearest_point_set_grid(struct chunk *c, struct loc *grid,
								 struct loc centre, struct point_set *points);
void correct_dir(struct loc *offset, struct loc grid1, struct loc grid2);
void adjust_dir(struct loc *offset, struct loc grid1, struct loc grid2);
void rand_dir(struct loc *offset);
enum direction opposite_dir(enum direction dir);
int trap_placement_chance(struct chunk *c, struct loc grid);
void place_traps(struct chunk *c);
void place_object(struct chunk *c, struct loc grid, int level, bool good,
	bool great, uint8_t origin, struct drop *drop);
void place_secret_door(struct chunk *c, struct loc grid);
void place_closed_door(struct chunk *c, struct loc grid);
void place_random_door(struct chunk *c, struct loc grid);
void place_forge(struct chunk *c, struct loc grid);
void alloc_stairs(struct chunk *c, int feat, int num, int minsep, bool any);
int alloc_object(struct chunk *c, int set, int typ, int num, int depth,
	uint8_t origin);
struct room_profile lookup_room_profile(const char *name);
void uncreate_artifacts(struct chunk *c);
void uncreate_greater_vaults(struct chunk *c, struct player *p);
void chunk_validate_objects(struct chunk *c);
void get_terrain(struct chunk *c, struct loc top_left, struct loc bottom_right,
				 struct loc place, int height, int width, int rotate,
				 bool reflect, bitflag *flags, bool floor, const char *data,
				 bool landmark);
struct landmark *find_landmark(int x_pos, int y_pos, int tolerance);
void dump_level_simple(const char *basefilename, const char *title,
	struct chunk *c);
void dump_level(ang_file *fo, const char *title, struct chunk *c, int **dist);
void dump_level_header(ang_file *fo, const char *title);
void dump_level_body(ang_file *fo, const char *title, struct chunk *c,
	int **dist);
void dump_level_footer(ang_file *fo);


#endif /* !GENERATE_H */
