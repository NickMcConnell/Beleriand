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
#include "mon-make.h"
#include "mon-move.h"
#include "mon-spell.h"
#include "monster.h"
#include "obj-knowledge.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "object.h"
#include "player-history.h"
#include "player-quest.h"
#include "player-util.h"
#include "trap.h"
#include "z-queue.h"
#include "z-type.h"

/*
 * Array of pit types
 */
struct vault *vaults;
struct surface_profile *surface_profiles;
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
 * Parsing functions for surface_profile.txt
 * ------------------------------------------------------------------------ */
static enum parser_error parse_surface_name(struct parser *p) {
	struct surface_profile *h = parser_priv(p);
	struct surface_profile *s = mem_zalloc(sizeof *s);
	//size_t i;

	s->name = string_make(parser_getstr(p, "name"));
	s->base_feats = mem_zalloc(sizeof(char));
	s->base_feats[0] = '\0';
	s->next = h;
	parser_setpriv(p, s);
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_code(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	char code = parser_getchar(p, "code");

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	s->code = code;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_feat(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct area_profile *a;
	struct formation_profile *f;
	int feat = lookup_feat(parser_getstr(p, "feat"));

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	if (s->formations.name) {
		f = &s->formations;
		f->feats = mem_realloc(f->feats, (f->num_feats + 2) * sizeof(char));
		f->feats[f->num_feats] = feat;
		f->num_feats++;
		f->feats[f->num_feats] = '\0';
	} else if (s->areas) {
		a = s->areas;
		while (a->next) {
			a = a->next;
		}
		a->feat = feat;
	} else {
		s->base_feats = mem_realloc(s->base_feats,
									(s->num_base_feats + 2) * sizeof(char));
		s->base_feats[s->num_base_feats] = feat;
		s->num_base_feats++;
		s->base_feats[s->num_base_feats] = '\0';
	}
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_area(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct area_profile *a;

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	/* Go to the last valid area profile, then allocate a new one */
	a = s->areas;
	if (!a) {
		s->areas = mem_zalloc(sizeof(struct area_profile));
		a = s->areas;
	} else {
		while (a->next) {
			a = a->next;
		}
		a->next = mem_zalloc(sizeof(struct area_profile));
		a = a->next;
	}

	a->name = string_make(parser_getstr(p, "name"));
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_frequency(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct area_profile *a;

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	/* Go to the last valid area profile */
	a = s->areas;
	assert(a);
	while (a->next) {
		a = a->next;
	}
	a->frequency = parser_getint(p, "frequency");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_attempts(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct area_profile *a;

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	/* Go to the last valid area profile */
	a = s->areas;
	assert(a);
	while (a->next) {
		a = a->next;
	}
	a->attempts = parser_getint(p, "num");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_size(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct formation_profile *f;
	struct area_profile *a;

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	if (s->formations.name) {
		f = &s->formations;
		f->size = parser_getrand(p, "size");
	} else {
		/* Go to the last valid area profile */
		a = s->areas;
		assert(a);
		while (a->next) {
			a = a->next;
		}
		a->size = parser_getrand(p, "size");
	}
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_formation(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct formation_profile *f;

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	f = &s->formations;
	f->name = string_make(parser_getstr(p, "name"));
	f->feats = mem_zalloc(sizeof(char));
	f->feats[0] = '\0';
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_surface_proportion(struct parser *p) {
	struct surface_profile *s = parser_priv(p);
	struct formation_profile *f;

	if (!s)
		return PARSE_ERROR_MISSING_RECORD_HEADER;

	f = &s->formations;
	f->proportion = parser_getint(p, "proportion");
	return PARSE_ERROR_NONE;
}


static struct parser *init_parse_surface(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_surface_name);
	parser_reg(p, "code char code", parse_surface_code);
	parser_reg(p, "feat str feat", parse_surface_feat);
	parser_reg(p, "area str name", parse_surface_area);
	parser_reg(p, "frequency int frequency", parse_surface_frequency);
	parser_reg(p, "attempts int num", parse_surface_attempts);
	parser_reg(p, "size rand size", parse_surface_size);
	parser_reg(p, "formation str name", parse_surface_formation);
	parser_reg(p, "proportion int proportion", parse_surface_proportion);
	return p;
}

static errr run_parse_surface(struct parser *p) {
	return parse_file_quit_not_found(p, "surface_profile");
}

static errr finish_parse_surface(struct parser *p) {
	struct surface_profile *n, *s = parser_priv(p);
	int num;

	z_info->surface_max = 0;
	/* Count the list */
	while (s) {
		z_info->surface_max++;
		s = s->next;
	}

	/* Allocate the array and copy the records to it */
	surface_profiles = mem_zalloc(z_info->surface_max * sizeof(*s));
	num = z_info->surface_max - 1;
	for (s = parser_priv(p); s; s = n) {
		/* Main record */
		memcpy(&surface_profiles[num], s, sizeof(*s));
		n = s->next;
		if (num < z_info->surface_max - 1)
			surface_profiles[num].next = &surface_profiles[num + 1];
		else
			surface_profiles[num].next = NULL;

		mem_free(s);
		num--;
	}

	parser_destroy(p);
	return 0;
}

static void cleanup_surface(void)
{
	int i;
	for (i = 0; i < z_info->surface_max; i++) {
		struct area_profile *a = surface_profiles[i].areas, *n;
		while (a) {
			n = a->next;
			string_free((char *) a->name);
			mem_free(a);
			a = n;
		}
		string_free((char *) surface_profiles[i].name);
		string_free(surface_profiles[i].base_feats);
		string_free((char *) surface_profiles[i].formations.name);
		string_free(surface_profiles[i].formations.feats);
	}
	mem_free(surface_profiles);
}

static struct file_parser surface_parser = {
	"surface_profile",
	init_parse_surface,
	run_parse_surface,
	finish_parse_surface,
	cleanup_surface
};


/**
 * ------------------------------------------------------------------------
 * Parsing functions for dungeon_profile.txt
 * ------------------------------------------------------------------------ */
static enum parser_error parse_dungeon_name(struct parser *p) {
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

static enum parser_error parse_dungeon_params(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->block_size = parser_getint(p, "block");
	c->dun_rooms = parser_getint(p, "rooms");
	c->dun_unusual = parser_getint(p, "unusual");
	c->max_rarity = parser_getint(p, "rarity");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_dungeon_tunnel(struct parser *p) {
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

static enum parser_error parse_dungeon_streamer(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->str.den = parser_getint(p, "den");
	c->str.rng = parser_getint(p, "rng");
	c->str.qua = parser_getint(p, "qua");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_dungeon_room(struct parser *p) {
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
	r->height = parser_getint(p, "height");
	r->width = parser_getint(p, "width");
	r->level = parser_getint(p, "level");
	r->rarity = parser_getint(p, "rarity");
	r->cutoff = parser_getint(p, "cutoff");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_dungeon_alloc(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->alloc = parser_getint(p, "alloc");
	return PARSE_ERROR_NONE;
}

static struct parser *init_parse_dungeon(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_dungeon_name);
	parser_reg(p, "params int block int rooms int unusual int rarity", parse_dungeon_params);
	parser_reg(p, "tunnel int rnd int chg int con int pen int jct", parse_dungeon_tunnel);
	parser_reg(p, "streamer int den int rng int qua", parse_dungeon_streamer);
	parser_reg(p, "room sym name int height int width int level int rarity int cutoff", parse_dungeon_room);
	parser_reg(p, "alloc int alloc", parse_dungeon_alloc);
	return p;
}

static errr run_parse_dungeon(struct parser *p) {
	return parse_file_quit_not_found(p, "dungeon_profile");
}

static errr finish_parse_dungeon(struct parser *p) {
	struct cave_profile *n, *c = parser_priv(p);
	int i, num;

	z_info->dungeon_max = 0;
	/* Count the list */
	while (c) {
		struct room_profile *r = c->room_profiles;
		c->n_room_profiles = 0;

		z_info->dungeon_max++;
		while (r) {
			c->n_room_profiles++;
			r = r->next;
		}
		c = c->next;
	}

	/* Allocate the array and copy the records to it */
	cave_profiles = mem_zalloc(z_info->dungeon_max * sizeof(*c));
	num = z_info->dungeon_max - 1;
	for (c = parser_priv(p); c; c = n) {
		struct room_profile *r_new = NULL;

		/* Main record */
		memcpy(&cave_profiles[num], c, sizeof(*c));
		n = c->next;
		if (num < z_info->dungeon_max - 1)
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

static void cleanup_dungeon(void)
{
	int i, j;
	for (i = 0; i < z_info->dungeon_max; i++) {
		for (j = 0; j < cave_profiles[i].n_room_profiles; j++)
			string_free((char *) cave_profiles[i].room_profiles[j].name);
		mem_free(cave_profiles[i].room_profiles);
		string_free((char *) cave_profiles[i].name);
	}
	mem_free(cave_profiles);
}

static struct file_parser dungeon_parser = {
	"dungeon_profile",
	init_parse_dungeon,
	run_parse_dungeon,
	finish_parse_dungeon,
	cleanup_dungeon
};

#if 0
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

static enum parser_error parse_profile_rooms(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->dun_rooms = parser_getint(p, "rooms");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_streamer(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->str.den = parser_getint(p, "den");
	c->str.rng = parser_getint(p, "rng");
	c->str.qua = parser_getint(p, "qua");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_profile_alloc(struct parser *p) {
	struct cave_profile *c = parser_priv(p);

	if (!c)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	c->alloc = parser_getint(p, "alloc");
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
	r->hardness = parser_getint(p, "hardness");
	r->level = parser_getint(p, "level");
	r->random = parser_getint(p, "random");
	return PARSE_ERROR_NONE;
}

static struct parser *init_parse_profile(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_profile_name);
	parser_reg(p, "rooms int rooms", parse_profile_rooms);
	parser_reg(p, "streamer int den int rng int qua", parse_profile_streamer);
	parser_reg(p, "alloc int alloc", parse_profile_alloc);
	parser_reg(p, "room sym name int hardness int level int random", parse_profile_room);
	return p;
}

static errr run_parse_profile(struct parser *p) {
	return parse_file_quit_not_found(p, "dungeon_profile");
}

static errr finish_parse_profile(struct parser *p) {
	struct cave_profile *n, *c = parser_priv(p);
	int num;

	/* Count the list */
	z_info->dungeon_max = 0;
	while (c) {
		z_info->dungeon_max++;
		c = c->next;
	}

	/* Allocate the array and copy the records to it */
	cave_profiles = mem_zalloc(z_info->dungeon_max * sizeof(*c));
	num = z_info->dungeon_max - 1;
	for (c = parser_priv(p); c; c = n) {
		/* Main record */
		memcpy(&cave_profiles[num], c, sizeof(*c));
		n = c->next;
		if (num < z_info->dungeon_max - 1) {
			cave_profiles[num].next = &cave_profiles[num + 1];
		} else {
			cave_profiles[num].next = NULL;
		}

		if (c->room_profiles) {
			struct room_profile *r_old = c->room_profiles;
			struct room_profile *r_new;
			int i;

			/* Count the room profiles */
			cave_profiles[num].n_room_profiles = 0;
			while (r_old) {
				++cave_profiles[num].n_room_profiles;
				r_old = r_old->next;
			}

			/* Now allocate the room profile array */
			r_new = mem_zalloc(cave_profiles[num].n_room_profiles
				* sizeof(*r_new));

			r_old = c->room_profiles;
			for (i = 0; i < cave_profiles[num].n_room_profiles; i++) {
				struct room_profile *r_temp = r_old;

				/* Copy from the linked list to the array */
				memcpy(&r_new[i], r_old, sizeof(*r_old));

				/* Set the next profile pointer correctly. */
				if (r_new[i].next) {
					r_new[i].next = &r_new[i + 1];
				}

				/* Tidy up and advance to the next profile. */
				r_old = r_old->next;
				mem_free(r_temp);
			}

			cave_profiles[num].room_profiles = r_new;
		} else {
			cave_profiles[num].n_room_profiles = 0;
			cave_profiles[num].room_profiles = NULL;
		}

		mem_free(c);
		num--;
	}

	parser_destroy(p);
	return 0;
}

static void cleanup_profile(void)
{
	int i, j;
	for (i = 0; i < z_info->dungeon_max; i++) {
		for (j = 0; j < cave_profiles[i].n_room_profiles; j++)
			string_free((char *) cave_profiles[i].room_profiles[j].name);
		mem_free(cave_profiles[i].room_profiles);
		string_free((char *) cave_profiles[i].name);
	}
	mem_free(cave_profiles);
}

static struct file_parser dungeon_parser = {
	"dungeon_profile",
	init_parse_profile,
	run_parse_profile,
	finish_parse_profile,
	cleanup_profile
};
#endif

/**
 * ------------------------------------------------------------------------
 * Parsing functions for vault.txt
 * ------------------------------------------------------------------------ */
static enum parser_error parse_vault_name(struct parser *p) {
	struct vault *h = parser_priv(p);
	struct vault *v = mem_zalloc(sizeof *v);

	v->name = string_make(parser_getstr(p, "name"));
	v->next = h;
	v->index = z_info->v_max;
	z_info->v_max++;
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

static enum parser_error parse_vault_depth(struct parser *p) {
	struct vault *v = parser_priv(p);

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->depth = parser_getuint(p, "depth");
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_vault_rarity(struct parser *p) {
	struct vault *v = parser_priv(p);

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	v->rarity = parser_getuint(p, "rarity");
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
	size_t i;

	if (!v)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	desc = parser_getstr(p, "text");
	if (!v->wid) {
		size_t wid = strlen(desc);

		if (wid > 255) {
			return PARSE_ERROR_VAULT_TOO_BIG;
		}
		v->wid = (uint8_t)wid;
	}


	if (strlen(desc) != v->wid) {
		return PARSE_ERROR_VAULT_DESC_WRONG_LENGTH;
	} else {
		if (v->hgt == 255) {
			return PARSE_ERROR_VAULT_TOO_BIG;
		}
		v->text = string_append(v->text, desc);
		v->hgt++;
		/* Note if there is a forge in the vault */
		if (strchr(desc, '0')) v->forge = true;
	}

	/* Make sure vaults are no higher or wider than the room profiles allow. */
	for (i = 0; i < N_ELEMENTS(room_builders); i++) {
		if (streq(v->typ, room_builders[i].name)) {
			break;
		}
	}
	if (i == N_ELEMENTS(room_builders)) {
		return PARSE_ERROR_NO_ROOM_FOUND;
	}
	if (v->wid > room_builders[i].max_width) {
		return PARSE_ERROR_VAULT_TOO_BIG;
	}
	if (v->hgt > room_builders[i].max_height) {
		return PARSE_ERROR_VAULT_TOO_BIG;
	}

	return PARSE_ERROR_NONE;
}

struct parser *init_parse_vault(void) {
	struct parser *p = parser_new();
	z_info->v_max = 0;
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_vault_name);
	parser_reg(p, "type str type", parse_vault_type);
	parser_reg(p, "depth uint depth", parse_vault_depth);
	parser_reg(p, "rarity uint rarity", parse_vault_rarity);
	parser_reg(p, "flags str flags", parse_vault_flags);
	parser_reg(p, "D str text", parse_vault_d);
	return p;
}

static errr run_parse_vault(struct parser *p) {
	return parse_file_quit_not_found(p, "vault");
}

static errr finish_parse_vault(struct parser *p) {
	uint32_t rarity_denom = 1;
	struct vault *v;

	vaults = parser_priv(p);
	parser_destroy(p);

	/*
	 * For use in random_vault(), convert rarities from the 1 per value
	 * specified in vault.txt to use a fixed denominator that is the
	 * smallest integer positive integer divisible by all the rarities.
	 */
	for (v = vaults; v; v = v->next) {
		if (v->rarity > 0) {
			/*
			 * Find the greatest common divisor of rarity_denom and
			 * v->rarity using the division-based version of
			 * Euclid's algorithm.
			 */
			uint32_t g = rarity_denom;
			uint32_t b = v->rarity;

			while (b) {
				uint32_t t = b;

				b = g % b;
				g = t;
			}

			/*
			 * Update rarity_denom with the factors from v->rarity
			 * not already present.
			 */
			if (rarity_denom > 4294967295 / (v->rarity / g)) {
				plog("Smallest integer divisible by all vault rarities is too large.");
				return PARSE_ERROR_OUT_OF_BOUNDS;
			}
			rarity_denom *= v->rarity / g;
		}
	}
	/*
	 * Avoid the potential of overflow in random_vault() as it accumulates
	 * the rarities of possible vaults.
	 */
	if (rarity_denom > 4294967295 / z_info->v_max) {
		plog("Product of number of vaults and smallest integer divisible by all vault rarities is too large.");
		return PARSE_ERROR_OUT_OF_BOUNDS;
	}
	for (v = vaults; v; v = v->next) {
		if (v->rarity > 0) {
			v->rarity = rarity_denom / v->rarity;
		}
	}

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
	/* Initialize surface info */
	event_signal_message(EVENT_INITSTATUS, 0,
						 "Initializing arrays... (surface profiles)");
	if (run_parser(&surface_parser))
		quit("Cannot initialize surface profiles");

	/* Initialize dungeon info */
	event_signal_message(EVENT_INITSTATUS, 0,
						 "Initializing arrays... (dungeon profiles)");
	if (run_parser(&dungeon_parser))
		quit("Cannot initialize dungeon profiles");

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
	cleanup_parser(&surface_parser);
	cleanup_parser(&dungeon_parser);
	cleanup_parser(&vault_parser);
}


/**
 * Find a cave_profile by name
 * \param name is the name of the cave_profile being looked for
 */
static const struct cave_profile *find_cave_profile(const char *name)
{
	int i;

	for (i = 0; i < z_info->dungeon_max; i++) {
		const struct cave_profile *profile;

		profile = &cave_profiles[i];
		if (streq(name, profile->name))
			return profile;
	}

	/* Not there */
	return NULL;
}

/**
 * Choose a cave profile (only one for most levels currently - NRM)
 * \param p is the player
 */
static const struct cave_profile *choose_profile(struct player *p)
{
	const struct cave_profile *profile = NULL;

	/* Make the profile choice */
	if (p->depth == 0) {
		profile = find_cave_profile("gates");
	} else if (p->depth == z_info->dun_depth) {
		profile = find_cave_profile("throne");
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
		for (i = 0; i < z_info->dungeon_max; i++) {
			struct cave_profile *test_profile = &cave_profiles[i];
			if (test_profile->alloc <= 0) continue;
			total_alloc += test_profile->alloc;
			if (randint0(total_alloc) < test_profile->alloc) {
				profile = test_profile;
			}
		}
		if (!profile) {
			profile = find_cave_profile("cave");
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
	int y_coord = p->grid.y / CHUNK_SIDE;
	int x_coord = p->grid.x / CHUNK_SIDE;

	/* Search across all the chunks on the level */
	for (y = 0; y < ARENA_CHUNKS; y++) {
		for (x = 0; x < ARENA_CHUNKS; x++) {
			struct chunk_ref ref = { 0 };
			int y0 = y - y_coord;
			int x0 = x - x_coord;
			int lower, upper;
			bool exists;
			struct gen_loc *location;
			struct connector *join;

			/* Get the location data */
			ref.z_pos = p->depth;
			ref.y_pos = chunk_list[p->last_place].y_pos + y0;
			ref.x_pos = chunk_list[p->last_place].x_pos + x0;
			ref.region = find_region(ref.y_pos, ref.x_pos);

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

	mem_free(dd->cent);
	mem_free(dd->ent_n);
	for (i = 0; i < z_info->level_room_max; ++i) {
		mem_free(dd->ent[i]);
	}
	mem_free(dd->ent);
	if (dd->ent2room) {
		for (i = 0; dd->ent2room[i]; ++i) {
			mem_free(dd->ent2room[i]);
		}
		mem_free(dd->ent2room);
	}
	mem_free(dd->door);
	mem_free(dd->wall);
	mem_free(dd->tunn);
}

/**
 * Generate a random level.
 *
 * \param p is the current player struct, in practice the global player
 * \return a pointer to the new level
 */
static struct chunk *cave_generate(struct player *p, uint32_t seed)
{
	const char *error = "no generation";
	int y, x, y_coord, x_coord, tries = 0;
	struct chunk *chunk = NULL;
	struct connector *dun_join = NULL, *one_off_above = NULL, *one_off_below = NULL;
	struct loc centre = p->grid;

	/* Generate */
	for (tries = 0; tries < 100 && error; tries++) {
		struct dun_data dun_body;
		bool forge_made = p->unique_forge_made;

		error = NULL;

		/* Mark the dungeon as being unready (to avoid artifact loss, etc) */
		character_dungeon = false;

		/* Allocate global data (will be freed when we leave the loop) */
		dun = &dun_body;
		dun->cent = mem_zalloc(z_info->level_room_max * sizeof(struct loc));
		dun->cent_n = 0;
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
		chunk = dun->profile->builder(p);
		if (!chunk) {
			error = "Failed to build level";
			cleanup_dun_data(dun);
			p->unique_forge_made = forge_made;
			event_signal_flag(EVENT_GEN_LEVEL_END, false);
			seed = 0;
			continue;
		}

		Rand_quick = false;

		/* Clear generation flags */
		for (y = 0; y < chunk->height; y++) {
			for (x = 0; x < chunk->width; x++) {
				struct loc grid = loc(x, y);

				sqinfo_off(square(chunk, grid)->info, SQUARE_WALL_INNER);
				sqinfo_off(square(chunk, grid)->info, SQUARE_WALL_OUTER);
				sqinfo_off(square(chunk, grid)->info, SQUARE_WALL_SOLID);
			}
		}

		/* Regenerate levels that overflow their maxima */
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
			uncreate_artifacts(chunk);
			uncreate_greater_vaults(chunk, p);
			chunk_wipe(chunk);
			delete_temp_monsters();
			p->unique_forge_made = forge_made;
			event_signal_flag(EVENT_GEN_LEVEL_END, false);
		}

		one_off_above = dun->one_off_above;
		one_off_below = dun->one_off_below;
		dun_join = dun->join;
		cleanup_dun_data(dun);
	}

	if (error) quit_fmt("cave_generate() failed 100 times!");

	/* Clear stair creation */
	//p->upkeep->create_stair = FEAT_NONE;

	/* Chunk it */
	y_coord = centre.y / CHUNK_SIDE;
	x_coord = centre.x / CHUNK_SIDE;
	for (y = 0; y < ARENA_CHUNKS; y++) {
		for (x = 0; x < ARENA_CHUNKS; x++) {
			struct chunk_ref ref = { 0 };
			int y0 = y - y_coord;
			int x0 = x - x_coord;
			int lower, upper;
			bool reload;
			struct gen_loc *location = NULL;
			struct loc grid;

			/* Get the location data */
			ref.z_pos = p->depth;
			ref.y_pos =	chunk_list[p->last_place].y_pos + y0;
			ref.x_pos =	chunk_list[p->last_place].x_pos + x0;
			ref.region = find_region(ref.y_pos, ref.x_pos);

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
					if (feat_is_stair(feat)	|| feat_is_chasm(feat)) {
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
	connectors_free(dun_join);
	connectors_free(one_off_above);
	connectors_free(one_off_below);

	set_monster_place_current();
	return chunk;
}

/**
 * Prepare a new level for the player to enter
 * This can happen for three reasons:
 *   1. It's the first turn of the game
 *   2. The player is changing z-level and a new level needs to be generated
 *   3. The player is changing z-level and an old level needs to be reloaded
 *
 * \param p is the current player struct, in practice the global player
*/
void prepare_next_level(struct player *p)
{
	int i, x, y;
	struct chunk *chunk = NULL, *p_chunk = NULL;

	/* First turn */
	if (turn == 1) {
		/* Make an arena to build into */
		chunk = chunk_new(ARENA_SIDE, ARENA_SIDE);

		for (y = - ARENA_CHUNKS / 2; y <= ARENA_CHUNKS / 2; y++) {
			for (x = - ARENA_CHUNKS / 2; x <= ARENA_CHUNKS / 2; x++) {
				struct chunk_ref ref = { 0 };

				/* Get the location data */
				ref.z_pos = chunk_list[p->place].z_pos;
				ref.y_pos = chunk_list[p->place].y_pos;
				ref.x_pos = chunk_list[p->place].x_pos;
				chunk_offset_data(&ref, 0, y, x);

				/* Generate a new chunk */
				(void) chunk_fill(chunk, &ref, y + ARENA_CHUNKS / 2,
								  x + ARENA_CHUNKS / 2);
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
			int y_coord = p->grid.y / CHUNK_SIDE;
			int x_coord = p->grid.x / CHUNK_SIDE;
			uint32_t seed;

			/* The assumption here is that dungeon levels are always generated
			 * all at once, and there are no, for example, long tunnels of
			 * generation at the same z-level.  If that assumption becomes
			 * wrong, this code will have to change. */
			bool completely_new = false;

			/* Deal with location data */
			for (y = 0; y < ARENA_CHUNKS; y++) {
				for (x = 0; x < ARENA_CHUNKS; x++) {
					struct chunk_ref ref = { 0 };
					int y0 = y - y_coord;
					int x0 = x - x_coord;
					int lower, upper;
					bool reload;

					/* Get the location data */
					ref.z_pos = p->depth;
					ref.y_pos = chunk_list[p->last_place].y_pos + y0;
					ref.x_pos = chunk_list[p->last_place].x_pos + x0;
					ref.region = find_region(ref.y_pos, ref.x_pos);

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
					ref.gen_loc_idx = upper;
					(void) chunk_store(ARENA_CHUNKS / 2, ARENA_CHUNKS / 2,
									   ref.region, ref.z_pos, ref.y_pos,
									   ref.x_pos, ref.gen_loc_idx, false);

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
			struct chunk_ref centre = chunk_list[p->place];
			assert(centre.chunk);

			/* Silly games */
			chunk_wipe(cave);
			chunk = chunk_new(ARENA_SIDE, ARENA_SIDE);
			cave = chunk;
			chunk_wipe(p->cave);
			p_chunk = chunk_new(ARENA_SIDE, ARENA_SIDE);
			p->cave = p_chunk;

			/* Dungeon levels may not be centred on the player */
			if (p->depth) {
				centre.y_pos += ARENA_CHUNKS / 2 - player->grid.y / CHUNK_SIDE;
				centre.x_pos += ARENA_CHUNKS / 2 - player->grid.x / CHUNK_SIDE;
			}

			for (y = 0; y < ARENA_CHUNKS; y++) {
				for (x = 0; x < ARENA_CHUNKS; x++) {
					int chunk_idx;
					struct chunk_ref ref = { 0 };

					/* Get the location data */
					ref.z_pos = p->depth;
					ref.y_pos = centre.y_pos + y - ARENA_CHUNKS / 2;
					ref.x_pos = centre.x_pos + x - ARENA_CHUNKS / 2;
					ref.region = find_region(ref.y_pos, ref.x_pos);

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

	/* Generate a new level */
	cave = chunk;
	event_signal_flag(EVENT_GEN_LEVEL_END, true);

	/* Validate the dungeon (we could use more checks here) */
	chunk_validate_objects(chunk);

	/* Record details for where we came from, if possible */
	if (chunk_list[p->place].adjacent[DIR_UP] == p->last_place) {
		chunk_list[p->last_place].adjacent[DIR_DOWN] = p->place;
	} else if (chunk_list[p->place].adjacent[DIR_DOWN] == p->last_place) {
		chunk_list[p->last_place].adjacent[DIR_UP] = p->place;
	}

	/* Apply illumination */
	cave_illuminate(cave, is_daytime());

	/* Note any forges generated, done here in case generation fails earlier */
	for (y = 0; y < cave->height; y++) {
		for (x = 0; x < cave->width; x++) {
			if (square_isforge(cave, loc(x, y))) {
				/* Reset the time since the last forge when an interesting room
				 * with a forge is generated */
				player->forge_drought = 0;
				player->forge_count++;
			}
		}
	}

	/* The dungeon is ready */
	character_dungeon = true;
}

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
 * z_info->dungeon_max).
 */
const char *get_level_profile_name_from_index(int i)
{
	return (i >= 0 && i < z_info->dungeon_max) ?
		cave_profiles[i].name : NULL;
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
