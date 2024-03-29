/**
 * \file gen-surface.c 
 * \brief Surface terrain generation
 *
 * Code for creation of the terrain of Beleriand.
 *
 * Copyright (c) 2019
 * Nick McConnell, Leon Marrick, Ben Harrison, James E. Wilson, 
 * Robert A. Koeneke
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
#include "cave.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-make.h"
#include "monster.h"
#include "player-util.h"
#include "project.h"


/**
 * ------------------------------------------------------------------------
 * Various surface helper routines
 * ------------------------------------------------------------------------ */
#define HIGHLAND_TREE_CHANCE 30

/**
 * Make a point_set of all the squares in a standard chunk-size piece of
 * another (or the same) chunk.
 */
static struct point_set *make_chunk_point_set(struct chunk *c,
											  struct loc top_left)
{
	struct loc grid, b = loc_sum(top_left, loc(CHUNK_SIDE - 1, CHUNK_SIDE - 1));
	struct point_set *new = point_set_new(CHUNK_SIDE * CHUNK_SIDE);
	assert((b.x < c->width) && (b.y < c->height));
	for (grid.y = 0; grid.y < CHUNK_SIDE; grid.y++) {
		for (grid.x = 0; grid.x < CHUNK_SIDE; grid.x++) {
			add_to_point_set(new, loc_sum(grid, top_left));
		}
	}
	return new;
}

/**
 * Make a CHUNK_SIDE-long border between two biomes
 *
 * This border can be applied either to a straight edge or a diagonal
 */
static void make_biome_border(int edge[])
{
	int i;

	/* Start within CHUNK_SIDE / 10 of straight */
	edge[0] = CHUNK_SIDE / 10 - randint0(CHUNK_SIDE / 5);
	for (i = 1; i < CHUNK_SIDE; i++) {
		/* Move maximum of 1 in either direction */
		edge[i] = edge[i - 1] + 1 - randint0(3);
	}
}

/**
 * Make a point set at the given direction corner of a 22x22 chunk
 */
static struct point_set *make_corner_point_set(struct chunk *c,
											   struct loc top_left, int dir)
{
	int y, x;
	int edge[CHUNK_SIDE];
	struct point_set *new = point_set_new(CHUNK_SIDE * CHUNK_SIDE);

	assert(dir == DIR_NE || dir == DIR_SE || dir == DIR_SW || dir == DIR_NW);

	/* Get border deviations */
	make_biome_border(edge);

	/* Allocate points */
	if (dir == DIR_NE) {
		for (y = 0; y < CHUNK_SIDE; y++) {
			for (x = y + edge[y]; x < CHUNK_SIDE; x++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				if (x >= 0) add_to_point_set(new, grid);
			}
		}
	} else if (dir == DIR_SE) {
		for (y = 0; y < CHUNK_SIDE; y++) {
			for (x = CHUNK_SIDE - (y + edge[y]); x < CHUNK_SIDE; x++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				if (x >= 0) add_to_point_set(new, grid);
			}
		}
	} else if (dir == DIR_SW) {
		for (y = 0; y < CHUNK_SIDE; y++) {
			for (x = 0; x < y + edge[y]; x++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				if (x < CHUNK_SIDE) add_to_point_set(new, grid);
			}
		}
	} else {
		assert(dir == DIR_NW);
		for (y = 0; y < CHUNK_SIDE; y++) {
			for (x = 0; x < CHUNK_SIDE - (y + edge[y]); x++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				if (x < CHUNK_SIDE) add_to_point_set(new, grid);
			}
		}
	}
	return new;
}

/**
 * Make a point set at the given direction edge of a 22x22 chunk
 */
static struct point_set *make_edge_point_set(struct chunk *c,
											 struct loc top_left, int dir)
{
	int y, x;
	int edge[CHUNK_SIDE];
	struct point_set *new = point_set_new(CHUNK_SIDE * CHUNK_SIDE);

	assert(dir == DIR_E || dir == DIR_S || dir == DIR_W || dir == DIR_N);

	/* Get border deviations */
	make_biome_border(edge);

	/* Allocate points */
	if (dir == DIR_E) {
		for (y = 0; y < CHUNK_SIDE; y++) {
			for (x = CHUNK_SIDE + edge[y]; x < CHUNK_SIDE; x++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				add_to_point_set(new, grid);
			}
		}
	} else if (dir == DIR_S) {
		for (x = 0; x < CHUNK_SIDE; x++) {
			for (y = CHUNK_SIDE + edge[x]; y < CHUNK_SIDE; y++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				add_to_point_set(new, grid);
			}
		}
	} else if (dir == DIR_W) {
		for (y = 0; y < CHUNK_SIDE; y++) {
			for (x = 0; x < edge[y]; x++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				if (x < CHUNK_SIDE) add_to_point_set(new, grid);
			}
		}
	} else {
		assert(dir == DIR_N);
		for (x = 0; x < CHUNK_SIDE; x++) {
			for (y = 0; y < edge[y]; y++) {
				struct loc grid = loc_sum(loc(x, y), top_left);
				if (y < CHUNK_SIDE) add_to_point_set(new, grid);
			}
		}
	}
	return new;
}

/**
 * Make a point set at the given direction edge of a 22x22 chunk
 */
static struct point_set *match_edge_point_set(struct chunk *c,
											  struct loc top_left,
											  int gen_loc_idx, int dir)
{
	int y, x, count;
	struct point_set *new = point_set_new(CHUNK_SIDE * CHUNK_SIDE);
	struct connector *join = gen_loc_list[gen_loc_idx].join;

	assert(dir == DIR_E || dir == DIR_S || dir == DIR_W || dir == DIR_N);

	switch (dir) {
		case DIR_E: {
			while (join) {
				if (join->grid.x == 0) {
					add_to_point_set(new, loc(CHUNK_SIDE - 1, join->grid.y));
				}
				join = join->next;
			}
			for (y = 0; y < CHUNK_SIDE; y++) {
				if (!point_set_contains(new, loc(CHUNK_SIDE - 1, y))) {
					if (count) {
						int half = count / 2, mid = count % 2, i, j, len = 0;
						for (i = y - 1; i > y - 1 - half; i--) {
							if (!one_in_(3)) len++;
							for (j = CHUNK_SIDE - 2; j > CHUNK_SIDE - 2 - len;
								 j--) {
								add_to_point_set(new, loc(j, i));
								add_to_point_set(new, loc(j, i - count + 1));
								count--;
							}
						}
						if (mid) {
							for (j = CHUNK_SIDE - 2; j > CHUNK_SIDE - 2 - len;
								 j--) {
								add_to_point_set(new, loc(j, y - half - 1));
							}
						}
					}
					count = 0;
					continue;
				}
				count++;
			}
			break;
		}
		case DIR_S: {
			while (join) {
				if (join->grid.y == 0) {
					add_to_point_set(new, loc(join->grid.x, CHUNK_SIDE - 1));
				}
				join = join->next;
			}
			for (x = 0; x < CHUNK_SIDE; x++) {
				if (!point_set_contains(new, loc(x, CHUNK_SIDE - 1))) {
					if (count) {
						int half = count / 2, mid = count % 2, i, j, len = 0;
						for (i = x - 1; i > x - 1 - half; i--) {
							if (!one_in_(3)) len++;
							for (j = CHUNK_SIDE - 2; j > CHUNK_SIDE - 2 - len;
								 j--) {
								add_to_point_set(new, loc(i, j));
								add_to_point_set(new, loc(i - count + 1, j));
								count--;
							}
						}
						if (mid) {
							for (j = CHUNK_SIDE - 2; j > CHUNK_SIDE - 2 - len;
								 j--) {
								add_to_point_set(new, loc(x - half - 1, j));
							}
						}
					}
					count = 0;
					continue;
				}
				count++;
			}
			break;
		}
		case DIR_W: {
			while (join) {
				if (join->grid.x == CHUNK_SIDE - 1) {
					add_to_point_set(new, loc(0, join->grid.y));
				}
				join = join->next;
			}
			for (y = 0; y < CHUNK_SIDE; y++) {
				if (!point_set_contains(new, loc(0, y))) {
					if (count) {
						int half = count / 2, mid = count % 2, i, j, len = 0;
						for (i = y - 1; i > y - 1 - half; i--) {
							if (!one_in_(3)) len++;
							for (j = 1; j < 1 + len; j++) {
								add_to_point_set(new, loc(j, i));
								add_to_point_set(new, loc(j, i - count + 1));
								count--;
							}
						}
						if (mid) {
							for (j = 1; j > 1 + len; j--) {
								add_to_point_set(new, loc(j, y - half - 1));
							}
						}
					}
					count = 0;
					continue;
				}
				count++;
			}
			break;
		}
		case DIR_N: {
			while (join) {
				if (join->grid.y == CHUNK_SIDE - 1) {
					add_to_point_set(new, loc(join->grid.x, 0));
				}
				join = join->next;
			}
			for (x = 0; x < CHUNK_SIDE; x++) {
				if (!point_set_contains(new, loc(x, 0))) {
					if (count) {
						int half = count / 2, mid = count % 2, i, j, len = 0;
						for (i = x - 1; i > x - 1 - half; i--) {
							if (!one_in_(3)) len++;
							for (j = 1; j > 1 + len; j--) {
								add_to_point_set(new, loc(i, j));
								add_to_point_set(new, loc(i - count + 1, j));
								count--;
							}
						}
						if (mid) {
							for (j = 1; j > 1 + len; j--) {
								add_to_point_set(new, loc(x - half - 1, j));
							}
						}
					}
					count = 0;
					continue;
				}
				count++;
			}
			break;
		}
		default: ;
	}
	return new;
}

/**
 * Return a point set of points a given point set that aren't in a smaller
 * point set contained (maybe partially) in the larger one
 */
static struct point_set *point_set_subtract(struct point_set *big,
											struct point_set *small)
{
	int i;
	struct point_set *new = point_set_new(1);
	for (i = 0; i < point_set_size(big); i++) {
		if (!point_set_contains(small, big->pts[i])) {
			add_to_point_set(new, big->pts[i]);
		}
	}
	return new;
}

/**
 * Make a randomish point_set of grids contained in a given point_set.
 */
static struct point_set *make_random_point_set(struct chunk *c,
											   struct point_set *big, int size,
											   struct loc grid, char *base_feat,
											   int num_base_feats)
{
	int tries = size * 2;
	struct point_set *new = point_set_new(size);
	add_to_point_set(new, grid);
	size--;
	while (size && tries) {
		/* Choose a random step */
		int i, step = randint1(8);
		if (step > DIR_NONE) step++;
		grid = loc_sum(grid, ddgrid[step]);

		/* Check bounds */
		if (!point_set_contains(big, grid)) {
			break;
		}

		/* Check if already acquired */
		if (point_set_contains(new, grid)) {
			tries--;
			continue;
		}

		/* Check base feat */
		for (i = 0; i < num_base_feats; i++) {
			if (square_feat(c, grid)->fidx == base_feat[i]) break;
		}
		if (i == num_base_feats) {
			tries--;
			continue;
		}

		add_to_point_set(new, grid);
		size--;
	}
	return new;
}

/**
 * Make a formation - a randomish group of terrain squares. -NRM-
 */
static int make_formation(struct chunk *c, struct point_set *big,
						  char base_feat[], int num_base_feats,
						  char form_feat[], int num_form_feats, int size)
{
	struct loc grid = point_set_random(big);
	struct point_set *form = make_random_point_set(c, big, size, grid,
												   base_feat, num_base_feats);
	int i, num = point_set_size(form);
	for (i = 0; i < num; i++) {
		square_set_feat(c, form->pts[i], form_feat[randint0(num_form_feats)]);
	}
	point_set_dispose(form);
	return num;
}

/**
 * Helper struct for handling where biomes meet
 */
struct biome_tweak {
	int dir1;
	enum biome_type biome1;
	int idx1;
	int dir2;
	enum biome_type biome2;
	int idx2;
};

/**
 * Get the major biome type for a chunk, and edge tweaks
 * Return false if it's the standard biome with no tweaks, true otherwise
 */
static bool get_biome_tweaks(int y_pos, int x_pos, struct biome_tweak *tweak)
{
	enum biome_type standard, east, south, west, north;
	bool right = ((x_pos % CPM) == CPM - 1);
	bool bottom = ((y_pos % CPM) == CPM - 1);
	bool left = ((x_pos % CPM) == 0);
	bool top = ((y_pos % CPM) == 0);
	int upper1, lower1, upper2, lower2;

	/* Return if it's not in the edge of the square mile */
	if (!right && !bottom && !left && !top) return false;

	/* Get the biome for the square mile the chunk is in and its neighbours */
	standard = square_miles[y_pos / CPM][x_pos / CPM].biome;
	east = square_miles[y_pos / CPM][x_pos / CPM + 1].biome;
	south = square_miles[y_pos / CPM + 1][x_pos / CPM].biome;
	west = square_miles[y_pos / CPM][x_pos / CPM - 1].biome;
	north = square_miles[y_pos / CPM - 1][x_pos / CPM].biome;

	/* Check each edge and set the other biomes if needed */
	if (right) {
		bool gen_e = gen_loc_find(x_pos + 1, y_pos, 0, &lower1, &upper1);
		if (east != standard) {
			tweak->biome1 = east;
			if (bottom) {
				bool gen_s = gen_loc_find(x_pos, y_pos + 1, 0, &lower2,
										  &upper2);
				if (east == south) {
					/* Full corner */
					tweak->dir1 = DIR_NONE;
				} else {
					/* Two separate edges */
					tweak->dir1 = DIR_E;
					tweak->idx1 = gen_e ? upper1 : -1;
					tweak->biome2 = south;
					tweak->dir2 = DIR_S;
					tweak->idx2 = gen_s ? upper2 : -1;
				}
			} else if (top) {
				bool gen_n = gen_loc_find(x_pos, y_pos - 1, 0, &lower2,
										  &upper2);
				if (east == north) {
					/* Full corner */
					tweak->dir1 = DIR_NONE;
				} else {
					/* Two separate edges */
					tweak->dir1 = DIR_E;
					tweak->idx1 = gen_e ? upper1 : -1;
					tweak->biome2 = north;
					tweak->dir2 = DIR_N;
					tweak->idx2 = gen_n ? upper2 : -1;
				}
			} else if ((south == east) && ((y_pos % CPM) == CPM - 2)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_SE;
			} else if ((north == east) && ((y_pos % CPM) == CPM + 1)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_NE;
			} else {
				/* Single edge */
				tweak->dir1 = DIR_E;
				tweak->idx1 = gen_e ? upper1 : -1;
			}
			return true;
		}
	}
	if (bottom) {
		bool gen_s = gen_loc_find(x_pos, y_pos + 1, 0, &lower1, &upper1);
		if (south != standard) {
			tweak->biome1 = south;
			if (left) {
				bool gen_w = gen_loc_find(x_pos - 1, y_pos, 0, &lower2,
										  &upper2);
				if (west == south) {
					/* Full corner */
					tweak->dir1 = DIR_NONE;
				} else {
					/* Two separate edges */
					tweak->dir1 = DIR_S;
					tweak->idx1 = gen_s ? upper1 : -1;
					tweak->biome2 = west;
					tweak->dir2 = DIR_W;
					tweak->idx2 = gen_w ? upper2 : -1;
				}
			} else if ((south == east) && ((x_pos % CPM) == CPM - 2)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_SE;
			} else if ((south == west) && ((x_pos % CPM) == CPM + 1)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_SW;
			} else {
				/* Single edge */
				tweak->dir1 = DIR_S;
				tweak->idx1 = gen_s ? upper1 : -1;
			}
			return true;
		}
	}
	if (left) {
		bool gen_w = gen_loc_find(x_pos - 1, y_pos, 0, &lower1, &upper1);
		if (west != standard) {
			tweak->biome1 = west;
			if (top) {
				bool gen_n = gen_loc_find(x_pos, y_pos - 1, 0, &lower2, &upper2);
				if (west == north) {
					/* Full corner */
					tweak->dir1 = DIR_NONE;
				} else {
					/* Two separate edges */
					tweak->dir1 = DIR_W;
					tweak->idx1 = gen_w ? upper1 : -1;
					tweak->biome2 = north;
					tweak->dir2 = DIR_N;
					tweak->idx2 = gen_n ? upper2 : -1;
				}
			} else if ((south == west) && ((y_pos % CPM) == CPM - 2)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_SW;
			} else if ((north == west) && ((y_pos % CPM) == CPM + 1)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_NW;
				return true;
			} else {
				/* Single edge */
				tweak->dir1 = DIR_W;
					tweak->idx1 = gen_w ? upper1 : -1;
			}
			return true;
		}
	}
	if (top) {
		bool gen_n = gen_loc_find(x_pos, y_pos - 1, 0, &lower1, &upper1);
		if (north != standard) {
			tweak->biome1 = north;
			if ((north == east) && ((y_pos % CPM) == CPM - 2)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_NE;
			} else if ((north == west) && ((y_pos % CPM) == CPM + 1)) {
				/* Corner smoothing one away from the corner */
				tweak->dir1 = DIR_NW;
			} else {
				/* Single edge */
				tweak->dir1 = DIR_N;
				tweak->idx1 = gen_n ? upper1 : -1;
			}
			return true;
		}
	}

	/* On an edge, but no change of biome */
	return false;
}

/**
 * ------------------------------------------------------------------------
 * Surface generation
 * ------------------------------------------------------------------------ */
static void make_piece(struct chunk *c, enum biome_type terrain,
					   struct point_set *piece)
{
	int i, form_grids, size = point_set_size(piece);
	struct surface_profile *s;
	struct area_profile *area;

	/* Get the correct surface profile */
	for (i = 0; i < z_info->surface_max; i++) {
		if (surface_profiles[i].code == terrain) break;
	}
	assert(i < z_info->surface_max);
	s = &surface_profiles[i];

	/* Basic terrain */
	for (i = 0; i < size; i++) {
		int feat = s->base_feats[randint0(s->num_base_feats)];
		square_set_feat(c, piece->pts[i], feat);
	}

	/* Make areas */
	area = s->areas;
	while (area) {
		int areas = 0;
		for (i = 0; i < area->attempts; i++) {
			if (one_in_(size / area->frequency)) areas++;
		}

		/* Try fairly hard */
		for (i = 0; i < 50 && areas; i++) {
			int a = randcalc(area->size, 0, RANDOMISE);
			int b = randcalc(area->size, 0, RANDOMISE);
			struct loc grid;
			bool made_area = false;

			/* Try for an area */
			grid = point_set_random(piece);
			made_area = generate_starburst_room(c, piece, grid.y - b,
												grid.x - a, grid.y + b,
												grid.x + a, false, area->feat,
												true);

			/* Success ? */
			if (made_area)
				areas--;
		}
		area = area->next;
	}

	/* Place some formations */
	if (!s->formations.proportion) return;
	form_grids = size / s->formations.proportion;
	if (one_in_(2)) {
		form_grids -= randint0(form_grids / 2);
	} else {
		form_grids += randint0(form_grids / 2);
	}
	while (form_grids > 0) {
		form_grids -= make_formation(c, piece, s->base_feats,
									 s->num_base_feats, s->formations.feats,
									 s->formations.num_feats,
									 randcalc(s->formations.size, 0,
											  RANDOMISE));
	}
}

void surface_gen(struct chunk *c, struct chunk_ref *ref, int y_coord,
				 int x_coord, struct connector *first)
{
	struct loc top_left = loc(x_coord * CHUNK_SIDE, y_coord * CHUNK_SIDE);
	struct point_set *chunk = make_chunk_point_set(c, top_left);
	enum biome_type standard;
	struct biome_tweak tweak = { DIR_NONE, 0, -1, DIR_NONE, 0, -1 };

	/* Get the standard biome based on region.txt */
	standard = square_miles[ref->y_pos / CPM][ref->x_pos / CPM].biome;
	tweak.biome1 = standard;
	tweak.biome2 = standard;

	/* Check for tweaks, and generate accordingly */
	if (get_biome_tweaks(ref->y_pos, ref->x_pos, &tweak)) {
		if (tweak.dir1 == DIR_NONE) {
			/* Whole chunk is the tweaked biome */
			make_piece(c, tweak.biome1, chunk);
		} else if ((tweak.dir1 == DIR_NE) || (tweak.dir1 == DIR_SE) ||
				   (tweak.dir1 == DIR_SW) || (tweak.dir1 == DIR_NW)) {
			/* Corner effect */
			struct point_set *tweaked = make_corner_point_set(c, top_left,
															  tweak.dir1);
			struct point_set *remainder = point_set_subtract(chunk, tweaked);
			make_piece(c, tweak.biome1, tweaked);
			make_piece(c, standard, remainder);
			point_set_dispose(tweaked);
			point_set_dispose(remainder);
		} else {
			/* An edge effect, or two separate edge effects */
			struct point_set *first, *remainder1;
			if (tweak.idx1 >= 0) {
				first = match_edge_point_set(c, top_left, tweak.idx1,
											 tweak.dir1);
			} else {
				first = make_edge_point_set(c, top_left, tweak.dir1);
			}
			remainder1 = point_set_subtract(chunk, first);
			make_piece(c, tweak.biome1, first);

			if (tweak.dir2 != DIR_NONE) {
				struct point_set *second, *remainder2;
				if (tweak.idx2 >= 0) {
					second = match_edge_point_set(c, top_left, tweak.idx2,
												  tweak.dir2);
				} else {
					second = make_edge_point_set(c, top_left, tweak.dir2);
				}
				remainder2 = point_set_subtract(remainder1, first);

				/* Possibly this will overlap first, which I think is OK */
				make_piece(c, tweak.biome2, second);
				make_piece(c, standard, remainder2);
				point_set_dispose(second);
				point_set_dispose(remainder2);
			} else {
				make_piece(c, standard, remainder1);
			}
			point_set_dispose(first);
			point_set_dispose(remainder1);
		}
	} else {
		make_piece(c, standard, chunk);
	}
	point_set_dispose(chunk);
}

#if 0 //B for reference
/**
 * Number and type of "vaults" in wilderness levels 
 * These need to be set at the start of each wilderness generation routine.
 */
int num_wild_vaults;


/**
 * Set the number of wilderness vaults
 */
static void set_num_vaults(struct chunk *c)
{
	int max = 2;
	num_wild_vaults = 0;

	if (player->depth > 10)
		num_wild_vaults += randint0(max);
	if (player->depth > 20)
		num_wild_vaults += randint0(max);
	if (player->depth > 30)
		num_wild_vaults += randint0(max);
	if (player->depth > 40)
		num_wild_vaults += randint0(max);

	if (no_vault(player->place))
		num_wild_vaults = 0;
}

/**
 * Makes "paths to nowhere" from interplace paths toward the middle of the
 * current place.  Adapted from tunnelling code.
 */
static void path_to_nowhere(struct chunk *c, struct loc start,
							struct loc target, struct loc *pathend, int *num)
{
	struct loc direction, grid, end = start;
	int i, j;


	bool done = false;

	/* make sure targets are in fully in bounds, reflect back in if not */
	target.y += ABS(target.y - 1) - (target.y - 1)
		- ABS(c->height - 2 - target.y) + (c->height - 2 - target.y);
	target.x += ABS(target.x - 1) - (target.x - 1)
		- ABS(c->width - 2 - target.x) + (c->width - 2 - target.x);

	/* Start */
	correct_dir(&direction, start, target);
	grid = loc_sum(start, direction);
	if (square_in_bounds_fully(c, grid)) {
		/* Take a step */
		if (!square_ismark(c, grid))
			square_set_feat(c, grid, FEAT_ROAD);
		end = grid;
	} else {
		/* No good, just finish at the start */
		assert(square_in_bounds_fully(c, start));
		end = start;
		done = true;
	}

	/* 100 steps should be enough */
	for (i = 0; i < 50 && !done; i++) {
		/* Try one randomish step... */
		adjust_dir(&direction, grid, target);
		grid = loc_sum(grid, direction);
		if (square_in_bounds_fully(c, grid)) {
			if (!square_ismark(c, grid))
				square_set_feat(c, grid, FEAT_ROAD);
			end = grid;
		} else {
			break;
		}

		/* ...and one good one */
		correct_dir(&direction, grid, target);
		grid = loc_sum(grid, direction);
		if (square_in_bounds_fully(c, grid)) {
			if (!square_ismark(c, grid))
				square_set_feat(c, grid, FEAT_ROAD);
			end = grid;
		} else {
			break;
		}

		/* Near enough is good enough */
		if ((ABS(grid.x - target.x) < 3) && (ABS(grid.y - target.y) < 3))
			break;
	}

	/* Store the end */
	for (j = MAX_PATHS - 1; j >= 0; j--) {
		/* This is the first one, record it and finish */
		if (j == 0) {
			pathend[j] = end;
		}

		/* Continue until we find where this grid is in x order */
		if (pathend[j].x == 0)
			continue;
		if (pathend[j].x < end.x) {
			pathend[j + 1] = pathend[j];
			continue;
		}

		/* Found it, record */
		pathend[j + 1] = end;
		(*num)++;
		break;
	}
}

/**
 * Move the path if it might land in a river
 */
static void river_move(struct chunk *c, int *xp)
{
	int x = (*xp), diff;

	diff = x - c->width / 2;
	if (ABS(diff) < 10)
		x = (diff < 0) ? (x - 10) : (x + 10);

	(*xp) = x;
	return;
}

/**
 * Check whether the is space for a clearing with the given corners.
 */
static bool check_clearing_space(struct chunk *c, struct loc top_left,
		struct loc bottom_right)
{
	struct loc grid;

	/* Require corners to be in bounds */
	if (!square_in_bounds(c, top_left)) return false;
	if (!square_in_bounds(c, bottom_right)) return false;

	/*
	 * Check every square within the bounds (starburst rooms go up to
	 * but do not include the bounds)
	 */
	for (grid.y = top_left.y + 1; grid.y < bottom_right.y; grid.y++) {
		for (grid.x = top_left.x + 1; grid.x < bottom_right.x; grid.x++) {
			if (square_ismark(c, grid)) return false;
		}
	}

	return true;
}

/**
 * Check whether there is space for a wilderness vault with the given corners
 */
static bool check_vault_space(struct chunk *c, struct loc avoid,
							  struct loc top_left, struct loc bottom_right)
{
	struct loc grid;

	/* Require corners to be in bounds */
	if (!square_in_bounds_fully(c, top_left)) return false;
	if (!square_in_bounds_fully(c, bottom_right)) return false;

	/* Check every square */
	for (grid.y = top_left.y; grid.y < bottom_right.y; grid.y++) {
		for (grid.x = top_left.x; grid.x < bottom_right.x; grid.x++) {
			if (distance(grid, avoid) < 20) return false;
			if (square_ismark(c, grid)) return false;
			if (square(c, grid)->mon) return false;
			if (square_isplayertrap(c, grid)) return false;
			if (square_object(c, grid)) return false;
			if (square_iswebbed(c, grid)) return false;
		}
	}

	return true;
}

/**
 * Help make_formation():  test if adding blocking terrain to a grid could
 * disconnect regions.  The test is local and only tests for conditions that
 * are sufficient but not guaranteed (since that requires looking at the
 * entire cave) to disconnect regions.
 * \param c Is the chunk to check.
 * \param grid Is the grid to check.  Assumed to be fully in bounds.
 */
static bool potentially_disconnects(struct chunk *c, struct loc grid)
{
	int i, btonb_count = 0;
	struct loc neighbor;
	bool last_nonblock;

	neighbor = loc_sum(grid, ddgrid[clockwise_ddd[7]]);
	last_nonblock = square_ispassable(c, neighbor)
			|| square_isrubble(c, neighbor);
	for (i = 0; i < 8; ++i) {
		neighbor = loc_sum(grid, ddgrid[clockwise_ddd[i]]);
		if (square_ispassable(c, neighbor)
				|| square_isrubble(c, neighbor)) {
			if (!last_nonblock) {
				++btonb_count;
			}
			last_nonblock = true;
		} else {
			last_nonblock = false;
		}
	}
	/*
	 * If no neighbors are nonblocking or all the nonblocking neighbors
	 * will still be a contiguous group when the center is blocking, then
	 * there's no possibility of a disconnect.
	 */
	return btonb_count > 1;
}

/**
 * Make a formation - a randomish group of terrain squares. -NRM-
 * Care probably needed with declaring feat[].
 *
 * As of FAangband 0.2.2, wilderness "vaults" are now made here.  These
 * are less structured than cave vaults or webs; in particular other
 * formations or even "vaults" can bleed into them.
 *
 */
static int make_formation(struct chunk *c, struct player *p,struct loc grid,
						  int base_feat[], int num_base_feats, int *feat,
						  const char *name, int prob)
{
	int step, j, jj, i = 0, total = 0;
	int *all_feat = mem_zalloc(prob * sizeof(*all_feat));
	struct loc tgrid = grid;

	/* Need to make some wilderness vaults */
	if (num_wild_vaults) {
		struct vault *v;
		struct loc top_left, bottom_right;
		bool good_place = true;

		/* Greater vault? */
		bool greater = randint0(100 - p->depth) < 9;

		if (greater) {
			v = random_vault(p->depth, format("%s greater", name),
							 "Wilderness greater");
		} else {
			v = random_vault(p->depth, format("%s lesser", name),
							 "Wilderness lesser");
		}
		if (v) {
			/* Check to see if it will fit here (only avoid edges) */
			top_left = loc(grid.x - v->wid / 2, grid.y - v->hgt / 2);
			bottom_right = loc_sum(top_left, loc(v->wid, v->hgt));
			good_place = check_vault_space(c, p->grid, top_left, bottom_right);

			/* We've found a place */
			if (good_place) {
				/* Build the vault (never lit, icky) */
				if (!build_vault(c, grid, v)) {
					mem_free(all_feat);
					return 0;
				}

				/* Message */
				if (OPT(p, cheat_room))
					msg("%s. ", v->name);

				/* One less to make */
				num_wild_vaults--;

				/* Takes up some space */
				mem_free(all_feat);
				return (v->hgt * v->wid);
			}
		}
	}

	/* Extend the array of terrain types to length prob */
	jj = 0;
	for (j = 0; j < prob - 1; j++) {
		if (feat[jj] == FEAT_NONE)
			jj = 0;
		all_feat[j] = feat[jj];
		jj++;
	}

	/* Make a formation */
	while (i != (prob - 1)) {
		bool base = false;
		for (j = 0; j < num_base_feats; j++) {
			if (square_feat(c, tgrid)->fidx == base_feat[j]) {
				base = true;
				break;
			}
		}

		/* Avoid paths, stay in bounds */
		if (!square_in_bounds_fully(c, tgrid)
			|| !base
			|| square_ismark(c, tgrid)
			|| square_isvault(c, tgrid)
			|| loc_eq(tgrid, p->grid)
			|| (!feat_is_monster_walkable(all_feat[i])
				&& square_monster(c, tgrid))
			|| (!feat_is_object_holding(all_feat[i])
				&& square_object(c, tgrid))
			|| (!feat_is_passable(all_feat[i])
				&& all_feat[i] != FEAT_RUBBLE
				&& potentially_disconnects(c, tgrid))) {
			break;
		}

		/* Check for treasure */
		if ((all_feat[i] == FEAT_MAGMA) && (one_in_(90))) {
			all_feat[i] = FEAT_MAGMA_K;
		} else if ((all_feat[i] == FEAT_QUARTZ) && (one_in_(40))) {
			all_feat[i] = FEAT_QUARTZ_K;
		}

		/* Set the feature */
		square_set_feat(c, tgrid, all_feat[i]);
		square_mark(c, tgrid);

		/* Choose a random step for next feature, try to keep going */
		step = randint0(8) + 1;
		if (step > 4)
			step++;
		for (j = 0; j < 100; j++) {
			tgrid = loc_sum(tgrid, ddgrid[step]);
			/* Make sure we're staying in the sub-chunk being filled */
			if (!(tgrid.x % CHUNK_SIDE) || !(tgrid.y % CHUNK_SIDE)) break;
			if (!square_ismark(c, tgrid)) break;
		}

		/* Count */
		total++;

		/* Pick the next terrain, or finish */
		i = randint0(prob);
	}

	mem_free(all_feat);
	return (total);
}

/**
 * Connect a grid to the main path through a mountain level
 */
static void mtn_connect(struct chunk *c, struct loc grid1, struct loc grid2)
{
	struct loc gp[512];
	int path_grids, j;

	/* Find the shortest path */
	path_grids = project_path(c, gp, 512, grid1, grid2, PROJECT_ROCK);

	/* Make the path */
	for (j = 0; j < path_grids; j++) {
		if (!square_in_bounds_fully(c, gp[j])
				|| square_feat(c, gp[j])->fidx == FEAT_ROAD
				|| square_ismark(c, gp[j]))
			break;
		square_set_feat(c, gp[j], FEAT_ROAD);
		square_mark(c, gp[j]);
	}
}

/**
 * Attempt to place a web of the required type
 */
static bool place_web(struct chunk *c, struct player *p, const char *type)
{
	struct vault *v;
	int i;
	struct loc grid, top_left, centre;

	bool no_good = false;

	/* Choose a random web, can fail */
	v = random_vault(player->depth, type, NULL);
	if (!v) return false;

	/* Look for somewhere to put it */
	for (i = 0; i < 25; i++) {
		no_good = false;

		/* Random top left corner */
		top_left.y = randint1(c->height - 1 - v->hgt);
		top_left.x = randint1(c->width - 1 - v->wid);

		/* Check to see if it will fit (only avoid big webs and edges) */
		for (grid.y = top_left.y; grid.y < top_left.y + v->hgt; grid.y++) {
			for (grid.x = top_left.x; grid.x < top_left.x + v->wid; grid.x++) {
				if (square_isfall(c, grid) || square_ispermanent(c, grid)
					|| square_isplayer(c, grid)	|| square_isvault(c, grid)) {
					no_good = true;
					break;
				}
			}
			if (no_good) {
				break;
			}
		}
	}

	/* Give up if we couldn't find anywhere */
	if (no_good) return false;

	/* Build the vault */
	centre = loc(top_left.x + v->wid / 2, top_left.y + v->hgt / 2);
	if (!build_vault(c, centre, v)) return false;

	/* Replace granite by webbed trees */
	for (grid.y = top_left.y; grid.y < top_left.y + v->hgt; grid.y++) {
		for (grid.x = top_left.x; grid.x < top_left.x + v->wid; grid.x++) {
			if (square_isgranite(c, grid)) {
				if (one_in_(2)) {
					square_set_feat(c, grid, FEAT_TREE);
				} else {
					square_set_feat(c, grid, FEAT_TREE2);
				}
				square_add_web(c, grid);
			}
		}
	}

	return true;
}

/**
 * Place some monsters, objects and traps
 */
static int populate(struct chunk *c, bool valley)
{
	int i, j;

	/* Basic "amount" */
	int k = (player->depth / 2);

	if (valley) {
		if (k > 30)
			k = 30;
	} else {
		/* Gets hairy north of the mountains */
		if (player->depth > 40)
			k += 10;
	}

	/* Pick a base number of monsters */
	i = z_info->level_monster_min + randint1(8) + k;

	/* Build the monster probability table. */
	(void) get_mon_num(player->depth, player->depth);

	/* Put some monsters in the dungeon */
	for (j = i + k; j > 0; j--) {
		/* Always have some random monsters */
		if (j < 5) {
			/* Remove all monster restrictions. */
			get_mon_num_prep(NULL);

			/* Build the monster probability table. */
			(void) get_mon_num(player->depth, player->depth);
		}

		/*
		 * Place a random monster (quickly), but not in grids marked
		 * "SQUARE_TEMP".
		 */
		(void) pick_and_place_distant_monster(c, player, 10, true, player->depth);
	}

	/* Place some traps in the dungeon. */
	alloc_objects(c, SET_BOTH, TYP_TRAP, randint1(k), player->depth, 0);

	/* Put some objects in rooms */
	alloc_objects(c, SET_BOTH, TYP_OBJECT, Rand_normal(z_info->room_item_av, 3),
				  player->depth, ORIGIN_FLOOR);

	/* Put some objects/gold in the dungeon */
	alloc_objects(c, SET_BOTH, TYP_OBJECT, Rand_normal(z_info->both_item_av, 3),
				 player->depth, ORIGIN_FLOOR);
	alloc_objects(c, SET_BOTH, TYP_GOLD, Rand_normal(z_info->both_gold_av, 3),
				 player->depth, ORIGIN_FLOOR);

	return k;
}

/**
 * Perform some sanity checks on the generated level.
 */
static bool verify_level(struct chunk *c)
{
	struct loc last_bad_bnd = loc(0, 0);
	struct loc last_bad_mon = loc(0, 0);
	struct loc last_bad_obj = loc(0, 0);
	int broken_bnd = 0, broken_mon = 0, broken_obj = 0;
	struct loc grid;

	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if ((grid.y == 0 || grid.x == 0
					|| grid.y == c->height - 1
					|| grid.x == c->width - 1)
					&& square(c, grid)->feat != FEAT_PERM) {
				++broken_bnd;
				last_bad_bnd = grid;
			}
			if (square_monster(c, grid)
					&& !square_is_monster_walkable(c, grid)) {
				++broken_mon;
				last_bad_mon = grid;
			}
			if (square_object(c, grid)
					&& !square_isobjectholding(c, grid)) {
				++broken_obj;
				last_bad_obj = grid;
			}
		}
	}

	if (broken_bnd || broken_mon || broken_obj) {
		const char *title;

		if (broken_bnd) {
			title = format("Broken Wilderness:  %d Bounding Walls; Last at (x=%d,y=%d) with Feature=%d",
				broken_bnd, last_bad_bnd.x, last_bad_bnd.y,
				(int) square(c, last_bad_bnd)->feat);
		} else if (broken_mon) {
			title = format("Broken Monster:  %d Embedded in Terrain; Last at (x=%d,y=%d) with Terrain=%d",
				broken_mon, last_bad_mon.x, last_bad_mon.y,
				(int) square(c, last_bad_mon)->feat);
		} else {
			title = format("Broken Object:  %d Embedded in Terrain; Last at (x=%d,y=%d) with Terrain=%d",
				broken_obj, last_bad_obj.x, last_bad_obj.y,
				(int) square(c, last_bad_obj)->feat);
		}
		dump_level_simple(NULL, title, c);
		msg("Restarting wilderness generation; bad level in dumpedlevel.html");
		return false;
	}
	return true;
}

/**
 * Generate a new plain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *old_plain_gen(struct player *p, int height, int width)
{
	struct loc grid;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_TREE, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE };
	int ponds[] = { FEAT_WATER, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with grass */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Place some formations */
	while (form_grids < (50 * player->depth + 1000)) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_GRASS,
									 form_feats, "Plains", player->depth + 1);
	}

	/* And some water */
	form_grids = 0;
	while (form_grids < 300) {
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_GRASS, ponds,
									 "Plains", 10);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}

/**
 * Generate a new mountain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *old_mtn_gen(struct player *p, int height, int width)
{
	struct loc grid;
	int i, j;
	int plats;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;
	int min, dist, floors = 0;
	int randpoints[20];
	struct loc pathpoints[20];
	struct loc nearest_point = { height / 2, width / 2 };
	struct loc stairs[3];

	int form_feats[] = { FEAT_PASS_RUBBLE, FEAT_RUBBLE, FEAT_GRASS,
						 FEAT_TREE, FEAT_TREE2, FEAT_ROAD, FEAT_NONE };

	bool amon_rudh = false;

	/* Make the level */
	struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with grass (lets paths work -NRM-) */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, false, false);

	/* Place 2 or 3 paths to neighbouring places, make the paths through the
	 * place, place the player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Turn grass to granite */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_GRASS) {
				square_set_feat(c, grid, FEAT_GRANITE);
			}
		}
	}

	/* Dungeon entrance */
	if (world->levels[place].down) {
		/* Set the flag */
		amon_rudh = true;

		/* Mim's cave on Amon Rudh */
		i = 3;
		while (i) {
			grid.y = randint0(c->height - 2) + 1;
			grid.x = randint0(c->width - 2) + 1;
			if ((square_feat(c, grid)->fidx == FEAT_ROAD) ||
				(square_feat(c, grid)->fidx == FEAT_GRASS)) {
				square_set_feat(c, grid, FEAT_MORE);
				square_mark(c, grid);
				i--;
				stairs[2 - i] = grid;
				if (!i && (level_topography(last_place) == TOP_CAVE))
					player_place(c, p, grid);
			}
		}
	}


	/* Make paths permanent */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_ROAD) {
				/* Hack - prepare for plateaux, connecting */
				square_mark(c, grid);
				floors++;
			}
		}
	}

	/* Pick some joining points */
	for (j = 0; j < 20; j++)
		randpoints[j] = randint0(floors);
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_ROAD)
				floors--;
			else
				continue;
			for (j = 0; j < 20; j++) {
				if (floors == randpoints[j]) {
					pathpoints[j] = grid;
				}
			}
		}
	}

	/* Find the staircases, if any */
	if (amon_rudh) {
		for (j = 0; j < 3; j++) {
			grid = stairs[j];

			/* Now join them up */
			min = c->width + c->height;
			for (i = 0; i < 20; i++) {
				dist = distance(grid, pathpoints[i]);
				if (dist < min) {
					min = dist;
					nearest_point = pathpoints[i];
				}
			}
			mtn_connect(c, grid, nearest_point);
		}
	}

	/* Make a few "plateaux" */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, y;

		/* Try for a plateau */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		y = rand_range(b, c->height - 1 - b);
		x = rand_range(a, c->width - 1 - a);
		if (!check_clearing_space(c, loc_sum(loc(x, y), loc(-a, -b)),
				loc_sum(loc(x, y), loc(a, b)))
				|| !generate_starburst_room(c, y - b, x - a,
				y + b, x + a, false, FEAT_GRASS, true)) continue;

		/* Success */
		grid = loc(x, y);
		plats--;

		/* Now join it up */
		min = c->width + c->height;
		for (i = 0; i < 20; i++) {
			dist = distance(grid, pathpoints[i]);
			if (dist < min) {
				min = dist;
				nearest_point = pathpoints[i];
			}
		}
		mtn_connect(c, grid, nearest_point);

		/* Done ? */
		if (!plats)
			break;
	}

	/* Make a few formations */
	while (form_grids < 50 * (player->depth)) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_GRANITE, FEAT_GRANITE,
									 form_feats, "Mountain", player->depth * 2);
		/* Now join it up */
		min = c->width + c->height;
		for (i = 0; i < 20; i++) {
			dist = distance(grid, pathpoints[i]);
			if (dist < min) {
				min = dist;
				nearest_point = pathpoints[i];
			}
		}
		mtn_connect(c, grid, nearest_point);

	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}
	ensure_connectedness(c, false);

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}

/**
 * Generate a new mountaintop level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct feature placement.
 */
struct chunk *old_mtntop_gen(struct player *p, int height, int width)
{
	struct loc grid, top;
	int i, j, k;
	int plats, a, b;
	int spot, floors = 0;
	bool placed = false;

	/* Make the level */
	struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with void */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create void */
			square_set_feat(c, grid, FEAT_VOID);
		}
	}

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, false, false);

	/* Make the main mountaintop */
	while (!placed) {
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		top.y = c->height / 2;
		top.x = c->width / 2;
		placed = generate_starburst_room(c, top.y - b, top.x - a, top.y + b,
										 top.x + a, false, FEAT_ROAD, false);
	}

	/* Summit */
	square_set_feat(c, top, FEAT_GRANITE);
	square_mark(c, top);
	for (i = 0; i < 8; i++) {
		square_set_feat(c, loc_sum(top, ddgrid[i]), FEAT_GRANITE);
		square_mark(c, loc_sum(top, ddgrid[i]));
	}

	/* Count the floors */
	for (grid.y = top.y - b; grid.y < top.y + b; grid.y++) {
		for (grid.x = top.x - a; grid.x < top.x + a; grid.x++) {
			if (square_feat(c, grid)->fidx == FEAT_ROAD) {
				floors++;
			}
		}
	}

	/* Choose the player place */
	spot = randint0(floors);

	/* Can we get down? */
	if (one_in_(2)) {
		grid.y = rand_range(top.y - b, top.y + b);
		if (square_feat(c, grid)->fidx != FEAT_VOID) {
			i = one_in_(2) ? 1 : -1;
			for (grid.x = top.x; grid.x != (top.x + i * (a + 1)); grid.x += i) {
				if (square_feat(c, grid)->fidx == FEAT_VOID) break;
			}
			square_set_feat(c, grid, FEAT_MORE);
		}
	}

	/* Adjust the terrain, place the player */
	for (grid.y = top.y - b; grid.y < top.y + b; grid.y++) {
		for (grid.x = top.x - a; grid.x < top.x + a; grid.x++) {
			/* Only change generated stuff */
			if (square_feat(c, grid)->fidx == FEAT_VOID)
				continue;

			/* Leave rock */
			if (square_feat(c, grid)->fidx == FEAT_GRANITE)
				continue;

			/* Leave stair */
			if (square_feat(c, grid)->fidx == FEAT_MORE)
				continue;

			/* Place the player? */
			if (square_feat(c, grid)->fidx == FEAT_ROAD) {
				floors--;
				if (floors == spot) {
					player_place(c, p, grid);
					square_mark(c, grid);
					continue;
				}
			}

			/* Place some rock */
			if (one_in_(5)) {
				square_set_feat(c, grid, FEAT_GRANITE);
				continue;
			}

			/* rubble */
			if (one_in_(8)) {
				square_set_feat(c, grid, FEAT_PASS_RUBBLE);
				continue;
			}

			/* and the odd tree */
			if (one_in_(20)) {
				square_set_feat(c, grid, FEAT_TREE2);
				continue;
			}
		}
	}

	/* Make a few "plateaux" */
	plats = randint0(4);

	/* Try fairly hard */
	for (j = 0; j < 10; j++) {
		/* Try for a plateau */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		top.y = rand_range(b, c->height - 1 - b);
		top.x = rand_range(a, c->width - 1 - a);
		if (!check_clearing_space(c, loc_sum(top, loc(-a, -b)),
				loc_sum(top, loc(a, b)))
				|| !generate_starburst_room(c, top.y - b,
				top.x - a, top.y + b, top.x + a, false,
				FEAT_ROAD, false)) continue;

		/* Success */
		plats--;

		/* Adjust the terrain a bit */
		for (grid.y = top.y - b; grid.y < top.y + b; grid.y++) {
			for (grid.x = top.x - a; grid.x < top.x + a; grid.x++) {
				/* Only change generated stuff */
				if (square_feat(c, grid)->fidx == FEAT_VOID)
					continue;

				/* Place some rock */
				if (one_in_(5)) {
					square_set_feat(c, grid, FEAT_GRANITE);
					continue;
				}

				/* rubble */
				if (one_in_(8)) {
					square_set_feat(c, grid, FEAT_PASS_RUBBLE);
					continue;
				}

				/* and the odd tree */
				if (one_in_(20)) {
					square_set_feat(c, grid, FEAT_TREE2);
					continue;
				}
			}
		}

		/* Done ? */
		if (!plats)
			break;
	}


	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);

			/* Paranoia - remake the dungeon walls */
			if ((grid.y == 0) || (grid.x == 0) ||
				(grid.y == c->height - 1) || (grid.x == c->width - 1)) {
				square_set_feat(c, grid, FEAT_PERM);
			}
		}
	}

	/* Basic "amount" */
	k = player->depth;

	/* Build the monster probability table. */
	(void) get_mon_num(player->depth, player->depth);

	/* Put some monsters in the dungeon */
	for (j = k; j > 0; j--) {
		(void) pick_and_place_distant_monster(c, player, 10, true, player->depth);
	}


	/* Put some objects in the dungeon */
	alloc_objects(c, SET_BOTH, TYP_OBJECT, Rand_normal(z_info->both_item_av, 3),
				 player->depth, ORIGIN_FLOOR);


	return c;
}

/**
 * Generate a new forest level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *old_forest_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int j;
	int plats;
	int place = p->place;
	int last_place = p->last_place;
<	int form_grids = 0;

	int form_feats[] = { FEAT_GRASS, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE	};
	int ponds[] = { FEAT_WATER, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with grass so paths work */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Now place trees */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create trees */
			if (square_feat(c, grid)->fidx == FEAT_GRASS) {
				if (randint1(player->depth + HIGHLAND_TREE_CHANCE)
					> HIGHLAND_TREE_CHANCE) {
					square_set_feat(c, grid, FEAT_TREE2);
				} else {
					square_set_feat(c, grid, FEAT_TREE);
				}
			} else {
				/* Hack - prepare for clearings */
				square_mark(c, grid);

			/* Mega hack - remove paths if emerging from Nan Dungortheb */
			//if ((last_place == q_list[2].place)
			//	&& (cave_feat[y][x] == FEAT_MORE_NORTH))
			//	cave_set_feat(y, x, FEAT_GRASS);
			}
		}
	}

	/* Make a few clearings */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, xlo, xhi, y, ylo, yhi;

		/*
		 * Try for a clearing.  Constrain the center choice so the
		 * bounding box is in bounds and the center won't be within
		 * the maximum possible extent of the walls created by
		 * make_edges() (that's to avoid a room that's entirely
		 * surrounded by those walls).
		 */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		ylo = MAX(b, 7);
		yhi = MIN(c->height - 1 - b, c->height - 8);
		y = rand_range(ylo, yhi);
		xlo = MAX(a, 10);
		xhi = MIN(c->width - 1 - a, c->height - 11);
		x = rand_range(xlo, xhi);
		made_plat = generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, true);

		/* Success ? */
		if (made_plat)
			plats--;

		/* Done ? */
		if (!plats)
			break;
	}

	/* Place some formations */
	while (form_grids < (50 * player->depth + 1000)) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_TREE, FEAT_TREE2,
									 form_feats, "Forest", player->depth + 1);
	}

	/* And some water */
	form_grids = 0;
	while (form_grids < 300) {
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_TREE, FEAT_TREE2, ponds,
									 "Forest", 10);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}

/**
 * Generate a new swamp level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *old_swamp_gen(struct player *p, int height, int width)
{
	struct loc grid;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_TREE, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with grass */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Add water */
	for (grid.y = 1; grid.y < c->height - 1; grid.y++) {
		for (grid.x = 2; grid.x < c->width - 1; grid.x++) {
			if (feat_is_permanent(square_feat(c, grid)->fidx))
				continue;
			if (loc_eq(p->grid, grid) || (randint0(100) < 50)) {
				square_set_feat(c, grid, FEAT_GRASS);
			} else {
				square_set_feat(c, grid, FEAT_WATER);
			}
		}
	}

	/* Place some formations (but not many, and less for more danger) */
	while (form_grids < 20000 / player->depth) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_WATER,
									 form_feats, "Swamp", player->depth);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}

/**
 * Generate a new desert level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *old_desert_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int j, d;
	int plats;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;

	int form_feats[] = { FEAT_GRASS, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_DUNE, FEAT_QUARTZ, FEAT_NONE };
	bool made_gate = false;

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with grass so paths work */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Dungeon entrance */
	if (world->levels[place].down) {
		/* No vaults */
		num_wild_vaults = 0;

		/* Angband! */
		for (d = 0; d < c->width; d++) {
			for (grid.y = 0; grid.y < d; grid.y++) {
				grid.x = d - grid.y;
				if (!square_in_bounds_fully(c, grid))
					continue;
				if (square_feat(c, grid)->fidx == FEAT_ROAD) {
					/* The gate of Angband */
					square_set_feat(c, grid, FEAT_MORE);
					made_gate = true;
					if (level_topography(last_place) == TOP_CAVE || turn < 10)
						player_place(c, p, grid);
					break;
				} else {
					/* The walls of Thangorodrim */
					square_set_feat(c, grid, FEAT_GRANITE);
				}
			}
			if (made_gate)
				break;
		}
	}

	/* Now place rubble, sand and magma */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create desert */
			if (square_feat(c, grid)->fidx == FEAT_GRASS) {
				if (one_in_(2))
					square_set_feat(c, grid, FEAT_DUNE);
				else if (one_in_(2))
					square_set_feat(c, grid, FEAT_PASS_RUBBLE);
				else
					square_set_feat(c, grid, FEAT_MAGMA);
			} else
				/* Prepare for clearings */
				square_mark(c, grid);
		}
	}

	/* Make a few clearings */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, xlo, xhi, y, ylo, yhi;

		/*
		 * Try for a clearing.  Constrain the center choice so the
		 * bounding box is in bounds and the center won't be within
		 * the maximum possible extent of the walls created by
		 * make_edges() (that's to avoid a room that's entirely
		 * surrounded by those walls).
		 */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		ylo = MAX(b, 7);
		yhi = MIN(c->height - 1 - b, c->height - 8);
		y = rand_range(ylo, yhi);
		xlo = MAX(a, 10);
		xhi = MIN(c->width - 1 - a, c->height - 11);
		x = rand_range(xlo, xhi);
		made_plat = generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, false);

		/* Success ? */
		if (made_plat)
			plats--;

		/* Done ? */
		if (!plats)
			break;
	}

	/* Place some formations */
	while (form_grids < 20 * player->depth) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_RUBBLE, FEAT_MAGMA,
									 form_feats, "Desert", player->depth);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}


/**
 * Generate a new river level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * Grids are marked during generation to ensure correct path placement.
 */
struct chunk *old_river_gen(struct player *p, int height, int width)
{
	struct loc grid, centre;
	int i, y1;
	int *mid;
	int place = p->place;
	int last_place = p->last_place;
	int form_grids = 0;
	int path;

	int form_feats[] = { FEAT_TREE, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);
	y1 = c->height / 2;

	/* Start with grass */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create grass */
			square_set_feat(c, grid, FEAT_GRASS);
		}
	}

	/* Place 2 or 3 paths to neighbouring places, place player -NRM- */
	alloc_paths(c, p, place, last_place);

	/* Remember the path in case it has to move */
	path = square_feat(c, p->grid)->fidx;

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, false);

	/* Place the river, start in the middle third */
	i = c->width / 3 + randint0(c->width / 3);
	mid = mem_zalloc(c->height * sizeof(int));
	for (grid.y = 1; grid.y < c->height - 1; grid.y++) {
		/* Remember the midpoint */
		mid[grid.y] = i;

		for (grid.x = i - randint0(5) - 10; grid.x < i + randint0(5) + 10;
			 grid.x++) {
			/* Make the river */
			square_set_feat(c, grid, FEAT_WATER);
			square_mark(c, grid);
		}
		/* Meander */
		i += randint0(3) - 1;
	}

	/* Mark the centre */
	centre = loc(mid[y1], y1);

	/* Special dungeon entrances */
	if (world->levels[place].down) {
		/* No vaults */
		num_wild_vaults = 0;

		/* If we're at Sauron's Isle... */
		if (world->levels[place].locality == LOC_SIRION_VALE) {
			for (grid.y = y1 - 10; grid.y < y1 + 10; grid.y++) {
				for (grid.x = mid[grid.y] - 10; grid.x < mid[grid.y] + 10;
					 grid.x++) {
					if (distance(centre, grid) < 6) {
						/* ...make Tol-In-Gaurhoth... */
						square_set_feat(c, grid, FEAT_GRASS);
					} else {
						/* ...surround it by water... */
						square_set_feat(c, grid, FEAT_WATER);
					}
					/* ...and build the tower... */
					if (distance(centre, grid) < 2)
						square_set_feat(c, grid, FEAT_GRANITE);
				}
			}
			/* ...with door and stairs */
			square_set_feat(c, loc(mid[y1], y1 + 1), FEAT_CLOSED);
			square_set_feat(c, centre, FEAT_MORE);
			if (level_topography(last_place) == TOP_CAVE) {
				player_place(c, p, centre);
			}
		} else {
			/* Must be Nargothrond */
			/* This place is hard to get into... */
			centre = loc(mid[y1] - 6, y1);
			for (grid.y = y1 - 1; grid.y < y1 + 2; grid.y++) {
				for (grid.x = mid[y1] - 15; grid.x < mid[y1] - 5; grid.x++) {
					if (loc_eq(grid, centre)) {
						square_set_feat(c, grid, FEAT_MORE);
						if (level_topography(last_place) == TOP_CAVE) {
							player_place(c, p, grid);
						}
					} else {
						square_set_feat(c, grid, FEAT_GRANITE);
					}
				}
				for (grid.x = mid[y1] - 5; grid.x < mid[y1]; grid.x++) {
					square_set_feat(c, grid, FEAT_ROAD);
				}
				for (grid.x = mid[y1] - 4; grid.x < mid[y1] + 5; grid.x++) {
					square_set_feat(c, grid, FEAT_WATER);
				}
			}
		}
	}

	/* Place some formations */
	while (form_grids < 50 * player->depth + 1000) {
		/* Choose a place */
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);

		form_grids += make_formation(c, p, grid, FEAT_GRASS, FEAT_GRASS,
									 form_feats, "River", player->depth / 2);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Move the player out of the river */
	grid = p->grid;
	while ((square_feat(c, grid)->fidx == FEAT_WATER)
		   || (square_feat(c, grid)->fidx == FEAT_GRANITE)) {
		grid.x++;
	}

	/* Place player if they had to move */
	if (!loc_eq(grid, p->grid)) {
		player_place(c, p, grid);
		square_set_feat(c, grid, path);
		for (grid.y = p->grid.y; grid.y > 0; grid.y--) {
			if (!square_ispassable(c, grid)) {
				square_set_feat(c, grid, FEAT_ROAD);
			}
		}
	}

	/* Place objects, traps and monsters */
	(void) populate(c, false);

	mem_free(mid);

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}



/**
 * Generate a new valley level. Place down slides, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 */
struct chunk *old_valley_gen(struct player *p, int height, int width)
{
	bool made_plat;

	struct loc grid;
	int i, j, k;
	int plats;
	int form_grids = 0;
	int form_feats[] = { FEAT_GRASS, FEAT_PASS_RUBBLE, FEAT_MAGMA, FEAT_GRANITE,
						 FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE };

    /* Make the level */
    struct chunk *c = cave_new(z_info->dungeon_hgt, z_info->dungeon_wid);

	/* Start with trees */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			/* Create trees */
			if (randint1(player->depth + HIGHLAND_TREE_CHANCE)
				> HIGHLAND_TREE_CHANCE) {
				square_set_feat(c, grid, FEAT_TREE2);
			} else {
				square_set_feat(c, grid, FEAT_TREE);
			}
		}
	}

	/* Set the number of wilderness vaults */
	set_num_vaults(c);

	/* Make place boundaries */
	make_edges(c, true, true);

	/* Make a few clearings */
	plats = rand_range(2, 4);

	/* Try fairly hard */
	for (j = 0; j < 50; j++) {
		int a, b, x, xlo, xhi, y, ylo, yhi;

		/*
		 * Try for a clearing.  Constrain the center choice so the
		 * bounding box is in bounds, the center won't be within the
		 * the maximum possible extent of the walls created by
		 * make_edges(), and the clearing won't intrude on the empty
		 * space created by make_edges().
		 */
		a = randint0(6) + 4;
		b = randint0(5) + 4;
		ylo = MAX(b, 10);
		yhi = c->height - 11 - b;
		y = rand_range(ylo, yhi);
		xlo = MAX(a, 10);
		xhi = MIN(c->width - 1 - a, c->height - 11);
		x = rand_range(xlo, xhi);
		made_plat = generate_starburst_room(c, y - b, x - a, y + b, x + a,
											false, FEAT_GRASS, true);

		/* Success ? */
		if (made_plat)
			plats--;

		/* Done ? */
		if (!plats)
			break;
	}

	/* Place some formations */
	while (form_grids < (40 * player->depth)) {
		grid.y = randint1(c->height - 2);
		grid.x = randint1(c->width - 2);
		form_grids += make_formation(c, p, grid, FEAT_TREE, FEAT_TREE2,
									 form_feats, NULL, player->depth + 1);
	}

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	if (!p->upkeep->path_coord) {
		/* Try to find an appropriate space. */
		j = 0;
		while (1) {
			if (j >= 100) {
				/*
				 * Give up; let the caller retry from scratch.
				 */
				wipe_mon_list(c, p);
				cave_free(c);
				return NULL;
			}
			grid.y = c->height / 2 - 10 + randint0(20);
			grid.x = c->width / 2 - 15 + randint0(30);
			if (!square_isvault(c, grid)
					&& !square_monster(c, grid)
					&& !square_object(c, grid)) {
				break;
			}
			++j;
		}
		square_set_feat(c, grid, FEAT_GRASS);
		player_place(c, p, grid);
		p->upkeep->path_coord = 0;

		/* Make sure a web can't be placed on the player */
		square_mark(c, grid);
	}

	/* Place some webs */
	k = MIN(30, player->depth / 2);
	for (i = 0; i < damroll(k / 20, 4); i++)
		place_web(c, p, "Small web");

	if (one_in_(2))
		place_web(c, p, "Medium web");

	if (one_in_(10) == 0)
		place_web(c, p, "Large web");

	/* Place objects, traps and monsters */
	k = populate(c, true);

	/* Unmark squares */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			square_unmark(c, grid);
		}
	}

	/* Maybe place a few random portals. */
	if (strstr(world->name, "Dungeon") && world->levels[p->place].down) {
		k = randint1(3) + 1;
		while (k > 0) {
			grid.y = randint1(c->height - 1);
			grid.x = randint1(c->width - 1);
			if (square_istree(c, grid)) {
				square_set_feat(c, grid, FEAT_MORE);
				k--;
			}
		}
	}

	if (!verify_level(c)) {
		wipe_mon_list(c, p);
		cave_free(c);
		return NULL;
	}

	return c;
}
#endif
