/** \file variable.c 
    \brief Definitions of global variables

 * The copyright.  Definitions for a large number of variables, arrays,
 * and pointers, plus the color table and sound names. 
 *
 * Copyright (c) 2009 Nick McConnell, Andi Sidwell, Leon Marrick, Bahman Rabii, 
 * Ben Harrison, James E. Wilson, Robert A. Koeneke
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

#include "angband.h"
#include "buildid.h"


/*
 * Hack -- Link a copyright message into the executable
 */
const char *copyright =
  "Copyright (c) 1987-2009 Angband contributors.\n"
  "\n"
  "This work is free software; you can redistribute it and/or modify it\n"
  "under the terms of either:\n"
  "\n"
  "a) the GNU General Public License as published by the Free Software\n"
  "   Foundation, version 2, or\n"
  "\n"
  "b) the Angband licence:\n"
  "   This software may be copied and distributed for educational, research,\n"
  "   and not for profit purposes provided that this copyright and statement\n"
  "   are included in all such copies.  Other copyrights may also apply.\n";


/*
 * Version of Beleriand.
 */
byte version_major = VERSION_MAJOR;
byte version_minor = VERSION_MINOR;
byte version_patch = VERSION_PATCH;
byte version_extra = VERSION_EXTRA;

/*
 * Savefile version 
 */
byte sf_major;		/**< Savefile's "version_major" */
byte sf_minor;		/* Savefile's "version_minor" */
byte sf_patch;		/* Savefile's "version_patch" */
byte sf_extra;		/* Savefile's "version_extra". Used for enryption */


/*
 * Run-time arguments
 */
bool arg_wizard;		/* Command arg -- Request wizard mode */
bool arg_rebalance;
int arg_graphics;		/* Command arg -- Request graphics mode */
bool arg_graphics_nice;	        /* Command arg -- Request nice graphics mode */

/*
 * Various things
 */

bool game_start;                /* Restart after death? */
bool character_generated;	/* The character exists */
bool character_dungeon;		/* The character has a dungeon */
bool character_loaded;		/* The character was loaded from a savefile */
bool character_saved;		/* The character was just saved to a savefile */

s16b character_icky;		/* Depth of the game in special mode */
s16b character_xtra;		/* Depth of the game in startup mode */

u32b seed_flavor;		/* Hack -- consistent object colors */

s16b num_repro;			/* Current reproducer count */
s16b object_level;		/* Current object creation level */
s16b monster_level;		/* Current monster creation level */

wchar_t summon_kin_type;		/* Hack -- See summon_specific() */

s32b turn;			/* Current game turn */

int use_graphics;		/* "graphics" mode */
bool use_graphics_nice;	        /* The 'nice' "graphics" mode is enabled */
bool use_transparency = FALSE;  /* Use transparent tiles */
char notes_start[80];           /* Opening line of notes */

s16b signal_count;		/* Hack -- Count interupts */

bool msg_flag;			/* Player has pending message */

bool inkey_base;		/* See the "inkey()" function */
bool inkey_xtra;		/* See the "inkey()" function */
u32b inkey_scan;		/* See the "inkey()" function */
bool inkey_flag;		/* See the "inkey()" function */

s16b coin_type;			/* Hack -- force coin type */
bool magic_throw;               /* Hack -- for magical throw spell */
bool opening_chest;		/* Hack -- prevent chest generation */
u16b j_level;                   /* Hack -- object level for jewellery */

bool shimmer_monsters;	/* Hack -- optimize multi-hued monsters */
bool shimmer_objects;	/* Hack -- optimize multi-hued objects */

bool repair_mflag_show;	/* Hack -- repair monster flags (show) */
bool repair_mflag_mark;	/* Hack -- repair monster flags (mark) */

s16b o_max = 1;			/* Number of allocated objects */
s16b o_cnt = 0;			/* Number of live objects */

s16b m_max = 1;			/* Number of allocated monsters */
s16b m_cnt = 0;			/* Number of live monsters */

s16b trap_max = 1;		/* Number of allocated traps */

u16b chunk_max = 1;		/* Number of allocated chunks */
u16b chunk_cnt = 0;		/* Number of live chunks */

u32b gen_loc_max = GEN_LOC_INCR;/* Number of allocated generated locations */
u32b gen_loc_cnt = 0;		/* Number of actual generated locations */

u16b group_id = 1;              /* Number of group IDs allocated */    

/*
 * Player info
 */
int player_uid;
int player_euid;
int player_egid;


/**
 * Buffer to hold the current savefile name
 */
char savefile[1024];


/**
 * The array[TERM_WIN_MAX] of window pointers
 */
term *angband_term[TERM_WIN_MAX];


/**
 * The array[TERM_WIN_MAX] of window names (modifiable?)
 */
char angband_term_name[TERM_WIN_MAX][16] =
{
  "Beleriand",
  "Term-1",
  "Term-2",
  "Term-3",
  "Term-4",
  "Term-5",
  "Term-6",
  "Term-7"
};




/**
 * Standard sound names (modifiable?)
 */
const const char *angband_sound_name[SOUND_MAX] =
{
  "",
  "hit",
  "miss",
  "flee",
  "drop",
  "kill",
  "level",
  "death",
  "study",
  "teleport",
  "shoot",
  "quaff",
  "zap_rod",
  "walk",
  "tpother",
  "hitwall",
  "eat",
  "store1",
  "store2",
  "store3",
  "store4",
  "dig",
  "opendoor",
  "shutdoor",
  "tplevel",
  "bell",
  "nothing_to_open",
  "lockpick_fail",
  "stairs_down", 
  "hitpoint_warn",
  "act_artifact", 
  "use_staff", 
  "destroy", 
  "mon_hit", 
  "mon_touch", 
  "mon_punch", 
  "mon_kick", 
  "mon_claw", 
  "mon_bite", 
  "mon_sting", 
  "mon_butt", 
  "mon_crush", 
  "mon_engulf", 
  "mon_crawl", 
  "mon_drool", 
  "mon_spit", 
  "mon_gaze", 
  "mon_wail", 
  "mon_spore", 
  "mon_beg", 
  "mon_insult", 
  "mon_moan", 
  "recover", 
  "blind", 
  "confused", 
  "poisoned", 
  "afraid", 
  "paralyzed", 
  "drugged", 
  "speed", 
  "slow", 
  "shield", 
  "blessed", 
  "hero", 
  "berserk", 
  "bold", 
  "prot_evil", 
  "invuln", 
  "see_invis", 
  "infrared", 
  "res_acid", 
  "res_elec", 
  "res_fire", 
  "res_cold", 
  "res_pois", 
  "stun", 
  "cut", 
  "stairs_up", 
  "store_enter", 
  "store_leave", 
  "store_home", 
  "money1", 
  "money2", 
  "money3", 
  "shoot_hit", 
  "store5", 
  "lockpick", 
  "disarm", 
  "identify_bad", 
  "identify_ego", 
  "identify_art", 
  "breathe_elements", 
  "breathe_frost", 
  "breathe_elec", 
  "breathe_acid", 
  "breathe_gas", 
  "breathe_fire", 
  "breathe_confu", 
  "breathe_disen", 
  "breathe_chaos", 
  "breathe_shards", 
  "breathe_sound", 
  "breathe_light", 
  "breathe_dark", 
  "breathe_nether", 
  "breathe_nexus", 
  "breathe_time", 
  "breathe_inertia", 
  "breathe_gravity", 
  "breathe_plasma", 
  "breathe_force", 
  "summon_monster", 
  "summon_angel", 
  "summon_undead", 
  "summon_animal", 
  "summon_spider", 
  "summon_hound", 
  "summon_hydra", 
  "summon_demon", 
  "summon_dragon", 
  "summon_gr_undead", 
  "summon_gr_dragon", 
  "summon_gr_demon", 
  "summon_wraith", 
  "summon_unique", 
  "wield", 
  "cursed", 
  "pseudo_id", 
  "hungry", 
  "notice", 
  "ambient_day", 
  "ambient_nite", 
  "ambient_dng1", 
  "ambient_dng2", 
  "ambient_dng3", 
  "ambient_dng4", 
  "ambient_dng5", 
  "mon_create_trap", 
  "mon_shriek", 
  "mon_cast_fear", 
  "hit_good", 
  "hit_great", 
  "hit_superb", 
  "hit_hi_great", 
  "hit_hi_superb", 
  "cast_spell", 
  "pray_prayer",
  "kill_unique",
  "kill_king",
  "drain_stat",
  "multiply"
};


/*
 * Arrays[TEMP_MAX] used for various things
 */
int temp_n = 0;
u16b *temp_g;
byte *temp_y;
byte *temp_x;

/**
 * Array[MAX_Y_REGION][MAX_X_REGION] of region terrain grids 
 */
char (*region_terrain)[MAX_X_REGION];

/* Dummy array */
byte *dummy;

/**
 * Array[ARENA_HGT][ARENA_WID] of cave grid info flags
 */
bitflag (*cave_info)[ARENA_WID][CAVE_SIZE];

/**
 * Array[ARENA_HGT][ARENA_WID] of cave grid feature codes
 */
byte (*cave_feat)[ARENA_WID];


/**
 * Array[ARENA_HGT][ARENA_WID] of cave grid object indexes
 *
 * Note that this array yields the index of the top object in the stack of
 * objects in a given grid, using the "next_o_idx" field in that object to
 * indicate the next object in the stack, and so on, using zero to indicate
 * "nothing".  This array replicates the information contained in the object
 * list, for efficiency, providing extremely fast determination of whether
 * any object is in a grid, and relatively fast determination of which objects
 * are in a grid.
 */
s16b (*cave_o_idx)[ARENA_WID];

/**
 * Array[ARENA_HGT][ARENA_WID] of cave grid monster indexes
 *
 * Note that this array yields the index of the monster or player in a grid,
 * where negative numbers are used to represent the player, positive numbers
 * are used to represent a monster, and zero is used to indicate "nobody".
 * This array replicates the information contained in the monster list and
 * the player structure, but provides extremely fast determination of which,
 * if any, monster or player is in any given grid.
 */
s16b (*cave_m_idx)[ARENA_WID];

/**
 * Array[ARENA_HGT][ARENA_WID] of cave grid flow "cost" values
 * Used to simulate character noise.
 */
byte (*cave_cost)[ARENA_WID];

/**
 * Array[ARENA_HGT][ARENA_WID] of cave grid flow "when" stamps.
 * Used to store character scent trails.
 */
byte (*cave_when)[ARENA_WID];

/**
 * Current scent age marker.  Counts down from 250 to 0 and then loops.
 */
int scent_when = 250;


/*
 * Centerpoints of the last flow (noise) rebuild and the last flow update.
 */
int flow_center_y;
int flow_center_x;
int update_center_y;
int update_center_x;

/**
 * Flow cost at the center grid of the current update.
 */
int cost_at_center = 0;

/**
 * Array[z_info->t_max] of traps
 */
trap_type *trap_list;

/**
 * Array[z_info->o_max] of dungeon objects
 */
object_type *o_list;

/**
 * Array[z_info->m_max] of dungeon monsters
 */
monster_type *m_list;

/**
 * Array[z_info->m_max] of monster lore
 */
monster_lore *l_list;

/**
 * Hack -- Array[MAX_Q_IDX] of quests
 */
quest *q_list;

/**
 * Array (no max length!) of generated locations
 */
gen_loc *gen_loc_list;

/**
 * Array[MAX_CHUNKS] of pointers to world chunks
 */
chunk_ref *chunk_list;

/**
 * Array[RANDNAME_NUM_TYPES][num_names] of random names
 */
const char *** name_sections;

/**
 * The size of "alloc_kind_table" (at most z_info->k_max * 4)
 */
s16b alloc_kind_size;

/**
 * The array[alloc_kind_size] of entries in the "kind allocator table"
 */
alloc_entry *alloc_kind_table;

/**
 * The size of the "alloc_ego_table"
 */
s16b alloc_ego_size;

/**
 * The array[alloc_ego_size] of entries in the "ego allocator table"
 */
alloc_entry *alloc_ego_table;

/**
 * The size of "alloc_race_table" (at most z_info->r_max)
 */
s16b alloc_race_size;

/**
 * The array[alloc_race_size] of entries in the "race allocator table"
 */
alloc_entry *alloc_race_table;

/**
 * The total of all final monster generation probabilities 
 */
u32b alloc_race_total;

/*
 * Specify attr/char pairs for visual special effects for project()
 */
byte gf_to_attr[GF_MAX][BOLT_MAX];
wchar_t gf_to_char[GF_MAX][BOLT_MAX];


/*
 * Specify attr/char pairs for visual special effects
 * Be sure to use "index & 0xFF" to avoid illegal access
 */
byte misc_to_attr[256];
char misc_to_char[256];


/**
 * Specify color for inventory item text display (by tval)
 * Be sure to use "index & 0x7F" to avoid illegal access
 */
byte tval_to_attr[128];




/*** Player information ***/

/**
 * Pointer to the player tables (sex, race, class, magic)
 */
player_sex *sp_ptr;
player_race *rp_ptr;
player_class *cp_ptr;
player_magic *mp_ptr;

/**
 * The player other record (static)
 */
static player_other player_other_body;

/**
 * Pointer to the player other record
 */
player_other *op_ptr = &player_other_body;

/**
 * The player info record (static)
 */
static player_type player_type_body;

/**
 * Pointer to the player info record
 */
player_type *p_ptr = &player_type_body;

/**
 * The character generates both directed (extra) noise (by doing noisy 
 * things) and ambiant noise (the combination of directed and innate 
 * noise).  Directed noise can immediately wake up monsters in LOS. 
 * Ambient noise determines how quickly monsters wake up and how often 
 * they get new information on the current character position.
 *
 * Each player turn, more noise accumulates.  Every time monster 
 * temporary conditions are processed, all non-innate noise is cleared.
 */
int add_wakeup_chance = 0;
u32b total_wakeup_chance = 0;

/**
 * Structure (not array) of size limits
 */
maxima *z_info;

/**
 * The region arrays
 */
region_type *region_info;

/**
 * The vault generation arrays
 */
vault_type *v_info;

/**
 * The landmark arrays
 */
landmark_type *l_info;

/**
 * The terrain feature arrays
 */
feature_type *f_info;

/**
 * The trap kind arrays
 */
trap_kind *trap_info;

/**
 * The object kind arrays
 */
object_kind *k_info;

/**
 * The artifact arrays
 */
artifact_type *a_info;

/**
 * The set item arrays
 */
set_type *set_info;

/**
 * The ego-item arrays
 */
ego_item_type *e_info;

/**
 * The monster race arrays
 */
monster_race *r_info;
monster_base *rb_info;
monster_pain *pain_messages;

/**
 * The player race arrays
 */
player_race *p_info;


/**
 * The player class arrays
 */
player_class *c_info;


/**
 * The player history arrays
 */
hist_type *h_info;

/**
 * The shop owner arrays
 */
owner_type *b_info;

/**
 * The spell arrays
 */
spell_type *s_info;

/**
 * The object flavor arrays
 */
flavor_type *flavor_info;


/**
 * The hints array
 */
struct hint *hints;

/**
 * Array of pit types
 */
struct pit_profile *pit_info;
 
/**
 * Hack -- The special Angband "System Suffix"
 * This variable is used to choose an appropriate "pref-xxx" file
 */
const char *ANGBAND_SYS = "xxx";

/**
 * Hack -- The special Angband "Graphics Suffix"
 * This variable is used to choose an appropriate "graf-xxx" file
 */
const char *ANGBAND_GRAF = "old";

/**
 * Path name: The main "lib" directory
 * This variable is not actually used anywhere in the code
 */
char *ANGBAND_DIR;

/**
 * High score files (binary)
 * These files may be portable between platforms
 */
char *ANGBAND_DIR_APEX;

/**
 * Bone files for player ghosts (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_BONE;

/**
 * Binary image files for the "*_info" arrays (binary)
 * These files are not portable between platforms
 */
char *ANGBAND_DIR_DATA;

/**
 * Textual template files for the "*_info" arrays (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_EDIT;

/**
 * Script files
 * These files are portable between platforms.
 */
char *ANGBAND_DIR_SCRIPT;

/**
 * Various extra files (ascii)
 * These files may be portable between platforms
 */
char *ANGBAND_DIR_FILE;

/**
 * Help files (normal) for the online help (ascii)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_HELP;

/**
 * Miscellanious text files, also contains any spoilers.
 * These files are portable between platforms
 */
char *ANGBAND_DIR_INFO;

/**
 * Standard "preference" files (ascii)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_PREF;

/**
 * Savefiles for current characters (binary)
 * These files are portable between platforms
 */
char *ANGBAND_DIR_SAVE;

/**
 * User "preference" files (ascii)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_USER;

/**
 * Various extra files (binary)
 * These files are rarely portable between platforms
 */
char *ANGBAND_DIR_XTRA;

/** 
 * Various xtra/ subdirectories.
 */
char *ANGBAND_DIR_XTRA_FONT;
char *ANGBAND_DIR_XTRA_GRAF;
char *ANGBAND_DIR_XTRA_SOUND;
char *ANGBAND_DIR_XTRA_HELP;
char *ANGBAND_DIR_XTRA_ICON;


/**
 * Total Hack -- allow all items to be listed (even empty ones)
 * This is only used by "do_cmd_inven_e()" and is cleared there.
 */
bool item_tester_full;


/**
 * Here is a "pseudo-hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
byte item_tester_tval;


/**
 * Here is a "hook" used during calls to "get_item()" and
 * "show_inven()" and "show_equip()", and the choice window routines.
 */
bool (*item_tester_hook)(const object_type*);



/**
 * Current "comp" function for ang_sort()
 */
bool (*ang_sort_comp)(const void *u, const void *v, int a, int b);

/**
 * Current "swap" function for ang_sort()
 */
void (*ang_sort_swap)(void *u, void *v, int a, int b);


/**
 * Hack -- function hook to restrict "get_mon_num_prep()" function
 */
bool (*get_mon_num_hook)(int r_idx);

/**
 * Hack -- function hook to restrict "get_obj_num_prep()" function
 */
bool (*get_obj_num_hook)(int k_idx);

/**
 * Hack - the destination file for text_out_to_file.
 */
ang_file *text_out_file = NULL;


/**
 * Hack -- function hook to output (colored) text to the
 * screen or to a file.
 */
void (*text_out_hook)(byte a, const char *str);


/**
 * Hack -- Where to wrap the text when using text_out().  Use the default
 * value (for example the screen width) when 'text_out_wrap' is 0.
 */
int text_out_wrap = 0;


/**
 * Hack -- Indentation for the text when using text_out().
 */
int text_out_indent = 0;


/**
 * Hack -- Padding after wrapping
 */
int text_out_pad = 0;


/**
 * Hack - the destination row for dump_line_screen.
 */
int dump_row = 0;

/**
 * Hack - the destination file for dump_line_file.
 */
ang_file *dump_out_file = NULL;

/**
 * Hack - the destination address for  dump_line_mem.
 */
char_attr *dump_ptr = NULL;

/**
 * Hack -- function hook for new line dump functions.
 */
void (*dump_line_hook)(char_attr *this_line);

/** 
 * Array for character screen/dump
 */
char_attr_line *dumpline;

/** 
 * Arrays for display player subwindows
 */
char_attr_line *pline0;
char_attr_line *pline1;

/**
 * The "highscore" file descriptor, if available.
 */
ang_file *highscore_fd;

/**
 * The type of object the item generator should make, if specified. -LM-
 */
byte required_tval = 0;

/** The bones file a restored player ghost should use to collect extra 
 * flags, a sex, and a unique name.  This also indicates that there is 
 * a ghost active.  -LM-
 */
byte bones_selector;

/**
 * The player ghost template index. -LM-
 */
int r_ghost;

/** 
 * The player ghost name is stored here for quick reference by the 
 * description function.  -LM-
 */
char ghost_name[80];

/**
 * The type (if any) of the player ghost's personalized string, and 
 * the string itself. -LM-
 */
int ghost_string_type = 0;
char ghost_string[80];

/**
 * Variable to insure that ghosts say their special message only once.  
 * This variable is deliberately not saved, so reloaded ghosts may speak 
 * again. -LM-
 */
bool ghost_has_spoken = FALSE;


/*
 * Autosave-related global variables adopted from Zangband. -LM- 
 */
bool is_autosave = FALSE;		/* Is the save an autosave */
bool autosave;				/* Timed autosave */
s16b autosave_freq;			/* Autosave frequency */


/*
 * Two variables that limit rogue stealing and creation of traps.  
 * One array that limits the number of runes of any type.
 * Mana reserve for runes of mana.
 */
byte number_of_thefts_on_level;
byte num_trap_on_level;
byte num_runes_on_level[RUNE_TAIL];
int mana_reserve = 0;

/**
 * Artifact counting variables and arrays
 */
int *artifact_normal, *artifact_special;
int artifact_normal_cnt, artifact_special_cnt;



/** XXX Mega-Hack - See main-win.c */
bool angband_keymap_flag = TRUE;

/**
 * Sound hook (for playing FX).
 */
void (*sound_hook)(int sound);


/*
 * For autoinscriptions.
 */
autoinscription *inscriptions = 0;
u16b inscriptions_count = 0;

/* Delay in centiseconds before moving to allow another keypress */
/* Zero means normal instant movement. */
u16b lazymove_delay = 0;


