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
		gen_e = gen_e && (gen_loc_list[upper1].seed != 0);
		if (east != standard) {
			tweak->biome1 = east;
			if (bottom) {
				bool gen_s = gen_loc_find(x_pos, y_pos + 1, 0, &lower2,
										  &upper2);
				gen_s = gen_s && (gen_loc_list[upper2].seed != 0);
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
				gen_n = gen_n && (gen_loc_list[upper2].seed != 0);
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
		gen_s = gen_s && (gen_loc_list[upper1].seed != 0);
		if (south != standard) {
			tweak->biome1 = south;
			if (left) {
				bool gen_w = gen_loc_find(x_pos - 1, y_pos, 0, &lower2,
										  &upper2);
				gen_w = gen_w && (gen_loc_list[upper2].seed != 0);
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
		gen_w = gen_w && (gen_loc_list[upper1].seed != 0);
		if (west != standard) {
			tweak->biome1 = west;
			if (top) {
				bool gen_n = gen_loc_find(x_pos, y_pos - 1, 0, &lower2, &upper2);
				gen_n = gen_n && (gen_loc_list[upper2].seed != 0);
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
		gen_n = gen_n && (gen_loc_list[upper1].seed != 0);
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
	struct formation_profile *form;

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
	form = s->formations;
	while (form) {
		if (!form->proportion) {
			form = form->next;
			continue;
		}
		form_grids = size / form->proportion;
		if (one_in_(2)) {
			form_grids -= randint0(form_grids / 2);
		} else {
			form_grids += randint0(form_grids / 2);
		}
		while (form_grids > 0) {
			form_grids -= make_formation(c, piece, s->base_feats,
										 s->num_base_feats, form->feats,
										 form->num_feats,
										 randcalc(form->size, 0, RANDOMISE));
		}
		form = form->next;
	}
}

void surface_gen(struct chunk *c, struct chunk_ref *ref, int y_coord,
				 int x_coord, struct connector *first_conn)
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

	/* Re-activate the complex RNG now terrain generation is done */
	Rand_quick = false;

	//TODO generate monsters, perhaps objects
}

