/**
 * \file generate.c
 * \brief Dungeon generation.
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2013 Erik Osheim, Nick McConnell
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
 *
 * This is the top level dungeon generation file, which contains room profiles
 * (for determining what rooms are available and their parameters), cave
 * profiles (for determining the level generation function and parameters for
 * different styles of levels), initialisation functions for template rooms and
 * vaults, and the main level generation function (which calls the level
 * builders from gen-cave.c).
 *
 * See the "vault.txt" file for more on vault generation.
 * See the "room_template.txt" file for more room templates.
 */

#include "angband.h"
#include "cave.h"
#include "datafile.h"
#include "game-event.h"
#include "game-input.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "math.h"
#include "mon-make.h"
#include "mon-move.h"
#include "mon-spell.h"
#include "monster.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "object.h"
#include "player-history.h"
#include "player-quest.h"
#include "player-util.h"
#include "trap.h"
#include "z-queue.h"
#include "z-type.h"

struct pit_profile *pit_info;
struct vault *vaults;
static struct cave_profile *cave_profiles;
struct dun_data *dun;
struct room_template *room_templates;

static const struct {
	const char *name;
	cave_builder builder;
} cave_builders[] = {
	#define DUN(a, b) { a, b##_gen },
	#include "list-dun-profiles.h"
	#undef DUN
};

static const struct {
	const char *name;
	int max_height;
	int max_width;
	room_builder builder;
} room_builders[] = {
	#define ROOM(a, b, c, d) { a, b, c, build_##d },
	#include "list-rooms.h"
	#undef ROOM
};

static const char *room_flags[] = {
	"NONE",
	#define ROOMF(a, b) #a,
	#include "list-room-flags.h"
	#undef ROOMF
	NULL
};


/**
 * ------------------------------------------------------------------------
 * Parsing functions for dungeon_profile.txt
 * ------------------------------------------------------------------------ */
static enum parser_error parse_profile_name(struct parser *p) {
	struct cave_profile *h = parser_priv(p);
	struct cave_profile *c = mem_zalloc(sizeof *c);
	size_t i;

	c->name = string_make(parser_getstr(p, "name"));
	for (i = 0; i < N_ELEMENTS(cave_builders); i++)
		if (streq(c->name, cave_builders[i].name))
			break;

	if (i == N_ELEMENTS(cave_builders))
		return PARSE_ERROR_NO_BUILDER_FOUND;
	c->builder = cave_builders[i].builder;
	c->next = h;
	parser_setpriv(p, c);
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_params(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->block_size = parser_getint(p, "block");
	c->dun_rooms = parser_getint(p, "rooms");
	c->dun_unusual = parser_getint(p, "unusual");
	c->max_rarity = parser_getint(p, "rarity");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_tunnel(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->tun.rnd = parser_getint(p, "rnd");
	c->tun.chg = parser_getint(p, "chg");
	c->tun.con = parser_getint(p, "con");
	c->tun.pen = parser_getint(p, "pen");
	c->tun.jct = parser_getint(p, "jct");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_streamer(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->str.den = parser_getint(p, "den");
	c->str.rng = parser_getint(p, "rng");
	c->str.mag = parser_getint(p, "mag");
	c->str.mc  = parser_getint(p, "mc");
	c->str.qua = parser_getint(p, "qua");
	c->str.qc  = parser_getint(p, "qc");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_room(struct parser *p) {
	struct cave_profile *c = parser_priv(p);
	struct room_profile *r = c->room_profiles;
	size_t i;

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	/* Go to the last valid room profile, then allocate a new one */
	if (!r) {
		c->room_profiles = mem_zalloc(sizeof(struct room_profile));
		r = c->room_profiles;
	} else {
		while (r->next)
			r = r->next;
		r->next = mem_zalloc(sizeof(struct room_profile));
		r = r->next;
	}

	/* Now read the data */
	r->name = string_make(parser_getsym(p, "name"));
	for (i = 0; i < N_ELEMENTS(room_builders); i++)
		if (streq(r->name, room_builders[i].name))
			break;

	if (i == N_ELEMENTS(room_builders))
		return PARSE_ERROR_NO_ROOM_FOUND;
	r->builder = room_builders[i].builder;
	r->rating = parser_getint(p, "rating");
	r->height = parser_getint(p, "height");
	r->width = parser_getint(p, "width");
	r->level = parser_getint(p, "level");
	r->pit = (parser_getint(p, "pit") == 1);
	r->rarity = parser_getint(p, "rarity");
	r->cutoff = parser_getint(p, "cutoff");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_min_level(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->min_level = parser_getint(p, "min");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_alloc(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->alloc = parser_getint(p, "alloc");
	return PARSE_ERROR_NONE;
}

static struct parser *init_parse_profile(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_profile_name);
	parser_reg(p, "params int block int rooms int unusual int rarity", parse_profile_params);
	parser_reg(p, "tunnel int rnd int chg int con int pen int jct", parse_profile_tunnel);
	parser_reg(p, "streamer int den int rng int mag int mc int qua int qc", parse_profile_streamer);
	parser_reg(p, "room sym name int rating int height int width int level int pit int rarity int cutoff", parse_profile_room);
	parser_reg(p, "min-level int min", parse_profile_min_level);
	parser_reg(p, "alloc int alloc", parse_profile_alloc);
	return p;
}

static errr run_parse_profile(struct parser *p) {
	return parse_file_quit_not_found(p, "dungeon_profile");
}

static errr finish_parse_profile(struct parser *p) {
	struct cave_profile *n, *c = parser_priv(p);
	int i, num;

	z_info->profile_max = 0;
	/* Count the list */
	while (c) {
		struct room_profile *r = c->room_profiles;
		c->n_room_profiles = 0;

		z_info->profile_max++;
		c = c->next;
		while (r) {
			c->n_room_profiles++;
			r = r->next;
		}
	}

	/* Allocate the array and copy the records to it */
	cave_profiles = mem_zalloc(z_info->profile_max * sizeof(*c));
	num = z_info->profile_max - 1;
	for (c = parser_priv(p); c; c = n) {
		struct room_profile *r_new = NULL;

		/* Main record */
		memcpy(&cave_profiles[num], c, sizeof(*c));
		n = c->next;
		if (num < z_info->profile_max - 1)
			cave_profiles[num].next = &cave_profiles[num + 1];
		else
			cave_profiles[num].next = NULL;

		/* Count the room profiles */
		if (c->room_profiles) {
			struct room_profile *r = c->room_profiles;
			c->n_room_profiles = 0;

			while (r) {
				c->n_room_profiles++;
				r = r->next;
			}
		}

		/* Now allocate the room profile array */
		if (c->room_profiles) {
			struct room_profile *r_temp, *r_old = c->room_profiles;

			/* Allocate space and copy */
			r_new = mem_zalloc(c->n_room_profiles * sizeof(*r_new));
			for (i = 0; i < c->n_room_profiles; i++) {
				memcpy(&r_new[i], r_old, sizeof(*r_old));
				r_old = r_old->next;
				if (!r_old) break;
			}

			/* Make next point correctly */
			for (i = 0; i < c->n_room_profiles; i++)
				if (r_new[i].next)
					r_new[i].next = &r_new[i + 1];

			/* Tidy up */
			r_old = c->room_profiles;
			r_temp = r_old;
			while (r_temp) {
				r_temp = r_old->next;
				mem_free(r_old);
				r_old = r_temp;
			}
		}
		cave_profiles[num].room_profiles = r_new;
		cave_profiles[num].n_room_profiles = c->n_room_profiles;

		mem_free(c);
		num--;
	}

	parser_destroy(p);
	return 0;
}

static void cleanup_profile(void)
{
	int i, j;
	for (i = 0; i < z_info->profile_max; i++) {
		for (j = 0; j < cave_profiles[i].n_room_profiles; j++)
			string_free((char *) cave_profiles[i].room_profiles[j].name);
		mem_free(cave_profiles[i].room_profiles);
		string_free((char *) cave_profiles[i].name);
	}
	mem_free(cave_profiles);
}

static struct file_parser profile_parser = {
	"dungeon_profile",
	init_parse_profile,
	run_parse_profile,
	finish_parse_profile,
	cleanup_profile
};


/**
 * ------------------------------------------------------------------------
 * Parsing functions for room_template.txt
 * ------------------------------------------------------------------------ */
static enum parser_error parse_room_name(struct parser *p) {
	struct room_template *h = parser_priv(p);
	struct room_template *t = mem_zalloc(sizeof *t);

	t->name = string_make(parser_getstr(p, "name"));
	t->next = h;
	parser_setpriv(p, t);
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_type(struct parser *p) {
	struct room_template *t = parser_priv(p);

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	t->typ = parser_getuint(p, "type");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_rating(struct parser *p) {
	struct room_template *t = parser_priv(p);

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	t->rat = parser_getint(p, "rating");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_height(struct parser *p) {
	struct room_template *t = parser_priv(p);
	size_t i;

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	t->hgt = parser_getuint(p, "height");

	/* Make sure rooms are no higher than the room profiles allow. */
	for (i = 0; i < N_ELEMENTS(room_builders); i++)
		if (streq("room template", room_builders[i].name))
			break;
	if (i == N_ELEMENTS(room_builders))
		return PARSE_ERROR_NO_ROOM_FOUND;
	if (t->hgt > room_builders[i].max_height)
		return PARSE_ERROR_VAULT_TOO_BIG;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_width(struct parser *p) {
	struct room_template *t = parser_priv(p);
	size_t i;

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	t->wid = parser_getuint(p, "width");

	/* Make sure rooms are no wider than the room profiles allow. */
	for (i = 0; i < N_ELEMENTS(room_builders); i++)
		if (streq("room template", room_builders[i].name))
			break;
	if (i == N_ELEMENTS(room_builders))
		return PARSE_ERROR_NO_ROOM_FOUND;
	if (t->wid > room_builders[i].max_width)
		return PARSE_ERROR_VAULT_TOO_BIG;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_doors(struct parser *p) {
	struct room_template *t = parser_priv(p);

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	t->dor = parser_getuint(p, "doors");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_tval(struct parser *p) {
	struct room_template *t = parser_priv(p);
	int tval;

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	tval = tval_find_idx(parser_getsym(p, "tval"));
	if (tval < 0)
		return PARSE_ERROR_UNRECOGNISED_TVAL;
	t->tval = tval;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_room_flags(struct parser *p) {
	struct room_template *t = parser_priv(p);
	char *s, *st;

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	s = string_make(parser_getstr(p, "flags"));
	st = strtok(s, " |");
	while (st && !grab_flag(t->flags, ROOMF_SIZE, room_flags, st)) {
		st = strtok(NULL, " |");
	}
	mem_free(s);

	return st ? PARSE_ERROR_INVALID_FLAG : PARSE_ERROR_NONE;
}

static enum parser_error parse_room_d(struct parser *p) {
	struct room_template *t = parser_priv(p);

	if (!t)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	t->text = string_append(t->text, parser_getstr(p, "text"));
	return PARSE_ERROR_NONE;
}

static struct parser *init_parse_room(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_room_name);
	parser_reg(p, "type uint type", parse_room_type);
	parser_reg(p, "rating int rating", parse_room_rating);
	parser_reg(p, "rows uint height", parse_room_height);
	parser_reg(p, "columns uint width", parse_room_width);
	parser_reg(p, "doors uint doors", parse_room_doors);
	parser_reg(p, "tval sym tval", parse_room_tval);
	parser_reg(p, "flags str flags", parse_room_flags);
	parser_reg(p, "D str text", parse_room_d);
	return p;
}

static errr run_parse_room(struct parser *p) {
	return parse_file_quit_not_found(p, "room_template");
}

static errr finish_parse_room(struct parser *p) {
	room_templates = parser_priv(p);
	parser_destroy(p);
	return 0;
}

static void cleanup_room(void)
{
	struct room_template *t, *next;
	for (t = room_templates; t; t = next) {
		next = t->next;
		mem_free(t->name);
		mem_free(t->text);
		mem_free(t);
	}
}

static struct file_parser room_parser = {
	"room_template",
	init_parse_room,
	run_parse_room,
	finish_parse_room,
	cleanup_room
};


/**
 * ------------------------------------------------------------------------
 * Parsing functions for vault.txt
 * ------------------------------------------------------------------------ */
static enum parser_error parse_vault_name(struct parser *p) {
	struct vault *h = parser_priv(p);
	struct vault *v = mem_zalloc(sizeof *v);

	v->name = string_make(parser_getstr(p, "name"));
	v->next = h;
	parser_setpriv(p, v);
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_type(struct parser *p) {
	struct vault *v = parser_priv(p);

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->typ = string_make(parser_getstr(p, "type"));
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_rating(struct parser *p) {
	struct vault *v = parser_priv(p);

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->rat = parser_getint(p, "rating");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_rows(struct parser *p) {
	struct vault *v = parser_priv(p);
	size_t i;

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->hgt = parser_getuint(p, "height");

	/* Make sure vaults are no higher than the room profiles allow. */
	for (i = 0; i < N_ELEMENTS(room_builders); i++)
		if (streq(v->typ, room_builders[i].name))
			break;
	if (i == N_ELEMENTS(room_builders))
		return PARSE_ERROR_NO_ROOM_FOUND;
	if (v->hgt > room_builders[i].max_height)
		return PARSE_ERROR_VAULT_TOO_BIG;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_columns(struct parser *p) {
	struct vault *v = parser_priv(p);
	size_t i;

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->wid = parser_getuint(p, "width");

	/* Make sure vaults are no wider than the room profiles allow. */
	for (i = 0; i < N_ELEMENTS(room_builders); i++)
		if (streq(v->typ, room_builders[i].name))
			break;
	if (i == N_ELEMENTS(room_builders))
		return PARSE_ERROR_NO_ROOM_FOUND;
	if (v->wid > room_builders[i].max_width)
		return PARSE_ERROR_VAULT_TOO_BIG;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_min_depth(struct parser *p) {
	struct vault *v = parser_priv(p);

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->min_lev = parser_getuint(p, "min_lev");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_max_depth(struct parser *p) {
	struct vault *v = parser_priv(p);
	int max_lev;

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	max_lev = parser_getuint(p, "max_lev");
	v->max_lev = max_lev ? max_lev : z_info->max_depth;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_flags(struct parser *p) {
	struct vault *v = parser_priv(p);
	char *s, *st;

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	s = string_make(parser_getstr(p, "flags"));
	st = strtok(s, " |");
	while (st && !grab_flag(v->flags, ROOMF_SIZE, room_flags, st)) {
		st = strtok(NULL, " |");
	}
	mem_free(s);

	return st ? PARSE_ERROR_INVALID_FLAG : PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_d(struct parser *p) {
	struct vault *v = parser_priv(p);
	const char *desc;

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	desc = parser_getstr(p, "text");
	if (strlen(desc) != v->wid)
		return PARSE_ERROR_VAULT_DESC_WRONG_LENGTH;
	else
		v->text = string_append(v->text, desc);
	return PARSE_ERROR_NONE;
}

struct parser *init_parse_vault(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_vault_name);
	parser_reg(p, "type str type", parse_vault_type);
	parser_reg(p, "rating int rating", parse_vault_rating);
	parser_reg(p, "rows uint height", parse_vault_rows);
	parser_reg(p, "columns uint width", parse_vault_columns);
	parser_reg(p, "min-depth uint min_lev", parse_vault_min_depth);
	parser_reg(p, "max-depth uint max_lev", parse_vault_max_depth);
	parser_reg(p, "flags str flags", parse_vault_flags);
	parser_reg(p, "D str text", parse_vault_d);
	return p;
}

static errr run_parse_vault(struct parser *p) {
	return parse_file_quit_not_found(p, "vault");
}

static errr finish_parse_vault(struct parser *p) {
	vaults = parser_priv(p);
	parser_destroy(p);
	return 0;
}

static void cleanup_vault(void)
{
	struct vault *v, *next;
	for (v = vaults; v; v = next) {
		next = v->next;
		mem_free(v->name);
		mem_free(v->typ);
		mem_free(v->text);
		mem_free(v);
	}
}

static struct file_parser vault_parser = {
	"vault",
	init_parse_vault,
	run_parse_vault,
	finish_parse_vault,
	cleanup_vault
};

static void run_template_parser(void) {
	/* Initialize room info */
	event_signal_message(EVENT_INITSTATUS, 0,
						 "Initializing arrays... (dungeon profiles)");
	if (run_parser(&profile_parser))
		quit("Cannot initialize dungeon profiles");

	/* Initialize room info */
	event_signal_message(EVENT_INITSTATUS, 0,
						 "Initializing arrays... (room templates)");
	if (run_parser(&room_parser))
		quit("Cannot initialize room templates");

	/* Initialize vault info */
	event_signal_message(EVENT_INITSTATUS, 0,
						 "Initializing arrays... (vaults)");
	if (run_parser(&vault_parser))
		quit("Cannot initialize vaults");
}


/**
 * Free the template arrays
 */
static void cleanup_template_parser(void)
{
	cleanup_parser(&profile_parser);
	cleanup_parser(&room_parser);
	cleanup_parser(&vault_parser);
}


/**
 * ------------------------------------------------------------------------
 * Level profile routines
 * ------------------------------------------------------------------------ */

/**
 * Return the number of room builders available.
 */
int get_room_builder_count(void)
{
	return (int) N_ELEMENTS(room_builders);
}
/**
 * Convert the name of a room builder into its index.  Return -1 if the
 * name does not match any of the room builders.
 */
int get_room_builder_index_from_name(const char *name)
{
	int i = 0;

	while (1) {
		if (i >= (int) N_ELEMENTS(room_builders)) {
			return -1;
		}
		if (streq(name, room_builders[i].name)) {
			return i;
		}
		++i;
	}
}

/**
 * Get the name of a room builder given its index.  Return NULL if the index
 * is out of bounds (less than one or greater than or equal to
 * get_room_builder_count()).
 */
const char *get_room_builder_name_from_index(int i)
{
	return (i >= 0 && i < (int) get_room_builder_count()) ?
		room_builders[i].name : NULL;
}

/**
 * Find a cave_profile by name
 * \param name is the name of the cave_profile being looked for
 */
static const struct cave_profile *find_cave_profile(const char *name)
{
	int i;

	for (i = 0; i < z_info->profile_max; i++) {
		const struct cave_profile *profile;

		profile = &cave_profiles[i];
		if (streq(name, profile->name))
			return profile;
	}

	/* Not there */
	return NULL;
}

/**
 * Convert the name of a level profile into its index in the cave_profiles
 * list.  Return -1 if the name does not match any of the profiles.
 */
int get_level_profile_index_from_name(const char *name)
{
	const struct cave_profile *p = find_cave_profile(name);

	return (p) ? (int) (p - cave_profiles) : -1;
}

/**
 * Get the name of a level profile given its index.  Return NULL if the index
 * is out of bounds (less than one or greater than or equal to
 * z_info->profile_max).
 */
const char *get_level_profile_name_from_index(int i)
{
	return (i >= 0 && i < z_info->profile_max) ?
		cave_profiles[i].name : NULL;
}

/**
 * Do d_m's prime check for labyrinths
 * \param depth is the depth where we're trying to generate a labyrinth
 */
static bool labyrinth_check(int depth)
{
	/* There's a base 2 in 100 to accept the labyrinth */
	int chance = 2;

	/* If we're too shallow then don't do it */
	if (depth < 13) return false;

	/* Certain numbers increase the chance of having a labyrinth */
	if (depth % 3 == 0) chance += 1;
	if (depth % 5 == 0) chance += 1;
	if (depth % 7 == 0) chance += 1;
	if (depth % 11 == 0) chance += 1;
	if (depth % 13 == 0) chance += 1;

	/* Only generate the level if we pass a check */
	if (randint0(100) >= chance) return false;

	/* Successfully ran the gauntlet! */
	//B return true;
	return false;
}

/**
 * Choose a cave profile
 * \param p is the player
 */
static const struct cave_profile *choose_profile(struct player *p)
{
	const struct cave_profile *profile = NULL;
	int moria_alloc = find_cave_profile("moria")->alloc;
	int labyrinth_alloc = find_cave_profile("labyrinth")->alloc;

	/* A bit of a hack, but worth it for now NRM */
	if (p->noscore & NOSCORE_JUMPING) {
		char name[30] = "";

		/* Cancel the query */
		p->noscore &= ~(NOSCORE_JUMPING);

		/* Ask debug players for the profile they want */
		if (get_string("Profile name (eg classic): ", name, sizeof(name)))
			profile = find_cave_profile(name);

		/* If no valid profile name given, fall through */
		if (profile) return profile;
	}

	/* Make the profile choice */
	if (labyrinth_check(p->depth) &&
		(labyrinth_alloc > 0 || labyrinth_alloc == -1)) {
		profile = find_cave_profile("labyrinth");
	} else if ((p->depth >= 10) && (p->depth < 40) && one_in_(40) &&
			   (moria_alloc > 0 || moria_alloc == -1)) {
		profile = find_cave_profile("moria");
	} else {
		int total_alloc = 0;
		size_t i;

		/*
		 * Use PowerWyrm's selection algorithm from
		 * get_random_monster_object() so the selection can be done in
		 * one pass and without auxiliary storage (at the cost of more
		 * calls to randint0()).  The mth valid profile out of n valid
		 * profiles appears with probability, alloc(m) /
		 * sum(i = 0 to m, alloc(i)) * product(j = m + 1 to n - 1,
		 * 1 - alloc(j) / sum(k = 0 to j, alloc(k))) which is equal to
		 * alloc(m) / sum(i = 0 to m, alloc(i)) * product(j = m + 1 to
		 * n - 1, sum(k = 0 to j - 1, alloc(k)) / sum(l = 0 to j,
		 * alloc(l))) which, by the canceling of successive numerators
		 * and denominators is alloc(m) / sum(l = 0 to n - 1, alloc(l)).
		 */
		for (i = 0; i < z_info->profile_max; i++) {
			struct cave_profile *test_profile = &cave_profiles[i];
			if (test_profile->alloc <= 0 ||
				p->depth < test_profile->min_level) continue;
			total_alloc += test_profile->alloc;
			if (randint0(total_alloc) < test_profile->alloc) {
				profile = test_profile;
			}
		}
		if (!profile) {
			profile = find_cave_profile("classic");
		}
	}

	/* Return the profile or fail horribly */
	if (profile)
		return profile;
	else
		quit("Failed to find cave profile!");

	return NULL;
}

/**
 * ------------------------------------------------------------------------
 * Helper routines for generation
 * ------------------------------------------------------------------------ */
/**
 * Get information for constructing stairs in the correct places
 */
static void get_join_info(struct player *p, struct dun_data *dd)
{
	int y, x;
	int y_offset = p->grid.y / CHUNK_SIDE;
	int x_offset = p->grid.x / CHUNK_SIDE;

	/* Search across all the chunks on the level */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			struct chunk_ref ref = { 0 };
			int y0 = y - y_offset;
			int x0 = x - x_offset;
			int lower, upper;
			bool exists;
			struct gen_loc *location;
			struct connector *join;

			/* Get the location data */
			ref.region = chunk_list[p->last_place].region;
			ref.z_pos = p->depth;
			ref.y_pos = chunk_list[p->last_place].y_pos + y0;
			ref.x_pos = chunk_list[p->last_place].x_pos + x0;

			/* See if the location up has been generated before */
			exists = gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos - 1, &lower,
								  &upper);

			/* Get the joins from the location up */
			if (exists) {
				location = &gen_loc_list[lower];
				join = location->join;
				while (join) {
					if (join->feat == FEAT_MORE) {
						struct connector *new = mem_zalloc(sizeof *new);
						new->grid.y = join->grid.y + y * CHUNK_SIDE;
						new->grid.x = join->grid.x + x * CHUNK_SIDE;
						new->feat = FEAT_LESS;
						new->next = dd->join;
						dd->join = new;
					}
					join = join->next;
				}
			} else {
				/*
				 * When there isn't a location above but there is one two levels
				 * up, remember where the down staircases are so up staircases
				 * on this level won't conflict with them if the level above is
				 * ever generated.
				 */
				exists = gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos - 2,
									  &lower, &upper);
				if (exists) {
					location = &gen_loc_list[lower];
					for (join = location->join; join; join = join->next) {
						if (join->feat == FEAT_MORE) {
							struct connector *nc = mem_zalloc(sizeof(*nc));
							struct loc offset = loc(x * CHUNK_SIDE,
													y * CHUNK_SIDE);
							nc->grid = loc_sum(join->grid, offset);
							nc->feat = FEAT_MORE;
							nc->next = dd->one_off_above;
							dd->one_off_above = nc;
						}
					}
				}
			}
			
			/* See if the location down has been generated before */
			exists = gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos + 1, &lower,
								  &upper);

			/* Get the joins from the location down */
			if (exists) {
				location = &gen_loc_list[lower];
				join = location->join;
				while (join) {
					if (join->feat == FEAT_LESS) {
						struct connector *new = mem_zalloc(sizeof *new);
						new->grid.y = join->grid.y + y * CHUNK_SIDE;
						new->grid.x = join->grid.x + x * CHUNK_SIDE;
						new->feat = FEAT_MORE;
						new->next = dd->join;
						dd->join = new;
					}
					join = join->next;
				}
			} else {
				/* Same logic as above for looking one past the next level */
				exists = gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos + 2,
									  &lower, &upper);
				if (exists) {
					location = &gen_loc_list[lower];
					for (join = location->join; join; join = join->next) {
						if (join->feat == FEAT_LESS) {
							struct connector *nc = mem_zalloc(sizeof(*nc));
							struct loc offset = loc(x * CHUNK_SIDE,
													y * CHUNK_SIDE);
							nc->grid = loc_sum(join->grid, offset);
							nc->feat = FEAT_LESS;
							nc->next = dd->one_off_above;
							dd->one_off_above = nc;
						}
					}
				}
			}
		}
	}
}

/**
 * Release the dynamically allocated resources in a dun_data structure.
 */
static void cleanup_dun_data(struct dun_data *dd)
{
	int i;

	if (!dd) return;

	//cave_connectors_free(dun->join);
	//cave_connectors_free(dun->one_off_above);
	//cave_connectors_free(dun->one_off_below);
	mem_free(dun->cent);
	mem_free(dun->ent_n);
	for (i = 0; i < z_info->level_room_max; ++i) {
		mem_free(dun->ent[i]);
	}
	mem_free(dun->ent);
	if (dun->ent2room) {
		for (i = 0; dun->ent2room[i]; ++i) {
			mem_free(dun->ent2room[i]);
		}
		mem_free(dun->ent2room);
	}
	mem_free(dun->door);
	mem_free(dun->wall);
	mem_free(dun->tunn);
}


/**
 * ------------------------------------------------------------------------
 * Main level generation functions
 * ------------------------------------------------------------------------ */
/**
 * Generate a dungeon level.
 *
 * \param p is the current player struct, in practice the global player
 * \param y_offset
 * \param x_offset
 * \return a pointer to the new level
 */
static struct chunk *cave_generate(struct player *p, u32b seed)
{
	const char *error = "no generation";
	int y, x, y_offset, x_offset, tries = 0;
	struct chunk *chunk = NULL;
	struct connector *dun_join, *one_off_above, *one_off_below;

	for (tries = 0; tries < 100 && error; tries++) {
		struct dun_data dun_body;
		struct quest *quest = NULL; //B quests not functional yet

		error = NULL;

		/* Mark the dungeon as being unready (to avoid artifact loss, etc) */
		character_dungeon = false;

		/* Allocate global data (freed when we leave the loop) */
		dun = &dun_body;
		dun->cent = mem_zalloc(z_info->level_room_max * sizeof(struct loc));
		dun->ent_n = mem_zalloc(z_info->level_room_max * sizeof(*dun->ent_n));
		dun->ent = mem_zalloc(z_info->level_room_max * sizeof(*dun->ent));
		dun->ent2room = NULL;
		dun->door = mem_zalloc(z_info->level_door_max * sizeof(struct loc));
		dun->wall = mem_zalloc(z_info->wall_pierce_max * sizeof(struct loc));
		dun->tunn = mem_zalloc(z_info->tunn_grid_max * sizeof(struct loc));
		dun->join = NULL;
		dun->one_off_above = NULL;
		dun->one_off_below = NULL;
		dun->curr_join = NULL;
		dun->nstair_room = 0;

		/* Get connector info */
		dun->persist = true;
		get_join_info(p, dun);

		/* Set the RNG to give reproducible results */
		//B This needs work, as we really want reproducible results for
		//B terrain only.  Options are to use Rand_quick only for terrain,
		//B or to clear out monsters and objects on re-generation (and
		//B maybe add some back?).  Messy.
		Rand_quick = true;
		if (!seed) {
			seed = randint0(0x10000000);
		}
		Rand_value = seed;

		/* Choose a profile and build the level */
		dun->profile = choose_profile(p);
		event_signal_string(EVENT_GEN_LEVEL_START, dun->profile->name);
		chunk = dun->profile->builder(p, ARENA_SIDE, ARENA_SIDE);
		if (!chunk) {
			error = "Failed to find builder";
			cleanup_dun_data(dun);
			event_signal_flag(EVENT_GEN_LEVEL_END, false);
			if (seed) {
				quit_fmt("Generation seed failure!");
			} else {
				continue;
			}
		}

		Rand_quick = false;

		/* Ensure quest monsters */
		if (quest && !quest->complete) {
			struct monster_race *race = quest->race;
			struct monster_group_info info = { 0, 0, 0 };
			struct loc grid;

			/* Pick a location and place the monster(s) */
			while (race->cur_num < quest->max_num) {
				find_empty(chunk, &grid);
				place_new_monster(chunk, grid, race, true, true, info,
								  ORIGIN_DROP);
			}
		}

		/* Clear generation flags, add connecting info */
		for (y = 0; y < chunk->height; y++) {
			for (x = 0; x < chunk->width; x++) {
				struct loc grid = loc(x, y);

				sqinfo_off(square(chunk, grid)->info, SQUARE_WALL_INNER);
				sqinfo_off(square(chunk, grid)->info, SQUARE_WALL_OUTER);
				sqinfo_off(square(chunk, grid)->info, SQUARE_WALL_SOLID);
				sqinfo_off(square(chunk, grid)->info, SQUARE_MON_RESTRICT);

				/* Regenerate levels that overflow the maximum monster limit */
				if (mon_max >= z_info->monster_max) {
					if (seed) {
						quit_fmt("Generation seed failure!");
					} else {
						error = "too many monsters";
					}
				}

				if (error) {
					if (OPT(p, cheat_room)) {
						msg("Generation restarted: %s.", error);
					}

					/* Clear the monsters */
					delete_temp_monsters();

					/* Free the chunk */
					chunk_wipe(chunk);
					event_signal_flag(EVENT_GEN_LEVEL_END, false);

					//B Probably need to clear stuff out of chunk_list, etc here
				}
			}
		}
		one_off_above = dun->one_off_above;
		one_off_below = dun->one_off_below;
		dun_join = dun->join;
		cleanup_dun_data(dun);
	}

	if (error) quit_fmt("cave_generate() failed 100 times!");

	/* Chunk it */
	y_offset = p->grid.y / CHUNK_SIDE;
	x_offset = p->grid.x / CHUNK_SIDE;
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			struct chunk_ref ref = { 0 };
			int y0 = y - y_offset;
			int x0 = x - x_offset;
			int lower, upper;
			bool reload;
			struct gen_loc *location;
			struct loc grid;

			/* Get the location data */
			ref.region = chunk_list[p->last_place].region;
			ref.z_pos = p->depth;
			ref.y_pos =	chunk_list[p->last_place].y_pos + y0;
			ref.x_pos =	chunk_list[p->last_place].x_pos + x0;

			/* Should have been generated before */
			reload = gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos, &lower,
								  &upper);

			/* Access the old place in the gen_loc_list */
			if (reload) {
				location = &gen_loc_list[upper];
			} else {
				quit("Location failure!");
			}

			/* Write the seed */
			location->seed = seed;

			/* Now write the connectors */
			for (grid.y = y * CHUNK_SIDE; grid.y < (y + 1) * CHUNK_SIDE;
				 grid.y++) {
				for (grid.x = x * CHUNK_SIDE; grid.x < (x + 1) * CHUNK_SIDE;
					 grid.x++) {
					int feat = square(chunk, grid)->feat;
					if (feat_is_stair(feat)	|| feat_is_fall(feat)) {
						/* Check previous join info for conflicts */
						struct connector *join = dun_join, *new;
						while (join) {
							if (loc_eq(grid, join->grid)) break;
							join = join->next;
						}
						if (join) continue;
						join = one_off_above;
						while (join) {
							if (loc_eq(grid, join->grid)) break;
							join = join->next;
						}
						join = one_off_below;
						if (join) continue;
						while (join) {
							if (loc_eq(grid, join->grid)) break;
							join = join->next;
						}
						if (join) continue;

						/* Write the join */
						new = mem_zalloc(sizeof *new);
						new->grid.y = grid.y - y * CHUNK_SIDE;
						new->grid.x = grid.x - x * CHUNK_SIDE;
						new->feat = feat;
						sqinfo_copy(new->info, square(chunk, grid)->info);
						new->next = location->join;
						location->join = new;
					}
				}
			}
		}
	}
	cave_connectors_free(dun_join);
	cave_connectors_free(one_off_above);
	cave_connectors_free(one_off_below);

	set_monster_place_current();
	return chunk;
}

/**
 * Prepare the level the player is about to enter, either by generating
 * or reloading.
 *
 * Note that this function is only called when the player is changing level
 * vertically, so that an entire 3x3-chunk arena is needed.
 *
 * \param c is the level we're going to end up with, in practice the global cave
 * \param p is the current player struct, in practice the global player
*/
void prepare_next_level(struct player *p)
{
	int i, y, x;
	struct chunk *chunk = NULL, *p_chunk = NULL;

	/* First turn */
	if (turn == 1) {
		/* Make an arena to build into */
		chunk = chunk_new(ARENA_SIDE, ARENA_SIDE);

		for (y = 0; y < 3; y++) {
			for (x = 0; x < 3; x++) {
				struct chunk_ref ref = { 0 };

				/* Get the location data */
				ref.z_pos = chunk_list[p->place].z_pos;
				ref.y_pos = chunk_list[p->place].y_pos;
				ref.x_pos = chunk_list[p->place].x_pos;
				ref.region = find_region(ref.y_pos, ref.x_pos);
				chunk_list[p->place].region = ref.region;
				chunk_adjacent_data(&ref, 0, y, x);

				/* Generate a new chunk */
				chunk_fill(chunk, &ref, y, x);
			}
		}
		player_place(chunk, p, loc(ARENA_SIDE / 2, ARENA_SIDE / 2));

		/* Allocate new known level */
		p->cave = chunk_new(chunk->height, chunk->width);
		p->cave->objects = mem_realloc(p->cave->objects, (chunk->obj_max + 1)
									   * sizeof(struct object*));
		p->cave->obj_max = chunk->obj_max;
		for (i = 0; i <= p->cave->obj_max; i++) {
			p->cave->objects[i] = NULL;
		}
	} else {
		/* No existing level */
		if (p->place == MAX_CHUNKS) {
			int y_offset = p->grid.y / CHUNK_SIDE;
			int x_offset = p->grid.x / CHUNK_SIDE;
			u32b seed;

			/* The assumption here is that dungeon levels are always generated
			 * all at once, and there are no, for example, long tunnels of
			 * generation at the same z-level.  If that assumption becomes
			 * wrong, this code will have to change. */
			bool completely_new = false;

			/* Deal with location data */
			for (y = 0; y < 3; y++) {
				for (x = 0; x < 3; x++) {
					struct chunk_ref ref = { 0 };
					int y0 = y - y_offset;
					int x0 = x - x_offset;
					int lower, upper;
					bool reload;

					/* Get the location data */
					ref.region = chunk_list[p->last_place].region;
					ref.z_pos = p->depth;
					ref.y_pos = chunk_list[p->last_place].y_pos + y0;
					ref.x_pos = chunk_list[p->last_place].x_pos + x0;

					/* See if we've been generated before */
					reload = gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos,
										  &lower, &upper);

					/* New gen_loc, or seed loading and checking */
					if (!reload) {
						gen_loc_make(ref.x_pos, ref.y_pos, ref.z_pos, upper);
						completely_new = true;
					} else if (!y && !x) {
						assert(gen_loc_list[upper].seed);
						seed = gen_loc_list[upper].seed;
					} else {
						assert(seed == gen_loc_list[upper].seed);
					}

					/* Store the chunk reference */
					(void) chunk_store(1, 1, ref.region, ref.z_pos, ref.y_pos,
									   ref.x_pos, false);

					/* Is this where the player is? */
					if ((y0 == 0) && (x0 == 0)) {
						p->place = chunk_find(ref);
						assert(p->place != MAX_CHUNKS);
					}
				}
			}

			/* Generate */
			if (completely_new) {
				chunk = cave_generate(p, 0);
			} else {
				/* Re-generate */
				chunk = cave_generate(p, seed);
			}

			/* Allocate new known level */
			p->cave = chunk_new(chunk->height, chunk->width);
			p->cave->objects = mem_realloc(p->cave->objects,
										   (chunk->obj_max + 1)
										   * sizeof(struct object*));
			p->cave->obj_max = chunk->obj_max;
			for (i = 0; i <= p->cave->obj_max; i++) {
				p->cave->objects[i] = NULL;
			}
		} else {
			/* Otherwise load up the chunks */
			int centre = chunk_get_centre();
			assert(centre != MAX_CHUNKS);

			/* Silly games */
			chunk_wipe(cave);
			chunk = chunk_new(ARENA_SIDE, ARENA_SIDE);
			cave = chunk;
			chunk_wipe(p->cave);
			p_chunk = chunk_new(ARENA_SIDE, ARENA_SIDE);
			p->cave = p_chunk;

			for (y = 0; y < 3; y++) {
				for (x = 0; x < 3; x++) {
					int chunk_idx;
					struct chunk_ref ref = { 0 };

					/* Get the location data */
					ref.region = chunk_list[centre].region;
					ref.z_pos = p->depth;
					ref.y_pos = chunk_list[centre].y_pos + y - 1;
					ref.x_pos = chunk_list[centre].x_pos + x - 1;

					/* Load it */
					chunk_idx = chunk_find(ref);
					if ((chunk_idx != MAX_CHUNKS) &&
						chunk_list[chunk_idx].chunk) {
						chunk_read(chunk_idx, y, x);
					} else {
						quit("Failed to find chunk!");
					}
				}
			}
			player_place(chunk, p, p->grid);
		}
	}

	/* Generated a new level */
	cave = chunk;
	event_signal_flag(EVENT_GEN_LEVEL_END, true);

	/* Validate the dungeon (we could use more checks here) */
	chunk_validate_objects(chunk);

	/* Light it if requested */
	if (p->upkeep->light_level) {
		wiz_light(chunk, p, false);
		p->upkeep->light_level = false;
	}

	/* Record details for where we came from, if possible */
	if (chunk_list[p->place].adjacent[DIR_UP] == p->last_place) {
		chunk_list[p->last_place].adjacent[DIR_DOWN] = p->place;
	} else if (chunk_list[p->place].adjacent[DIR_DOWN] == p->last_place) {
		chunk_list[p->last_place].adjacent[DIR_UP] = p->place;
	}

	/* Apply illumination */
	cave_illuminate(cave, is_daytime());

	/* The dungeon is ready */
	character_dungeon = true;
}

/**
 * The generate module, which initialises template rooms and vaults
 * Should it clean up?
 */
struct init_module generate_module = {
	.name = "generate",
	.init = run_template_parser,
	.cleanup = cleanup_template_parser
};
