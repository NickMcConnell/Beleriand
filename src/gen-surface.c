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
 * Match a point set at the given direction edge of a 22x22 chunk to an
 * existing one on the adjacent edge
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
 * Rivers
 * ------------------------------------------------------------------------ */
/**
 * Map a course from the corner of a square grid to a non-adjacent edge.
 *
 * We start in the top left corner and chart a course to the bottom, then
 * do a symmetry transform to get the directions right.
 */
static int map_corner_to_edge(size_t side, enum direction start_corner,
							   enum direction finish_edge, size_t finish,
							   uint16_t **course)
{
	size_t y = 0, x = 0;
	int y_dist = side - 1;
	int x_dist = finish;
	int rotate = 0;
	bool reflect = false;
	int count = 0;
	uint16_t **temp;

	/* Calculate rotation, reflection and adjustment to the finishing point
	 * CAREFULLY to get it right. Note that symmetry_transform() does
	 * rotations first, then reflections, and all eight symmetries of the
	 * square need to be considered individually. */
	if (start_corner == DIR_NW) {
		if (finish_edge == DIR_S) {
			/* No transform needed */
		} else if (finish_edge == DIR_E) {
			rotate = 1;
			reflect = true;
		} else {
			quit_fmt("Incorrect edge direction in map_corner_to_edge()");
		}
	} else if (start_corner == DIR_NE) {
		if (finish_edge == DIR_S) {
			reflect = true;
			x_dist = side - 1 - finish;
		} else if (finish_edge == DIR_W) {
			rotate = 1;
		} else {
			quit_fmt("Incorrect edge direction in map_corner_to_edge()");
		}
	} else if (start_corner == DIR_SE) {
		if (finish_edge == DIR_W) {
			rotate = 3;
			reflect = true;
			x_dist = side - 1 - finish;
		} else if (finish_edge == DIR_N) {
			rotate = 2;
			x_dist = side - 1 - finish;
		} else {
			quit_fmt("Incorrect edge direction in map_corner_to_edge()");
		}
	} else if (start_corner == DIR_SW) {
		if (finish_edge == DIR_N) {
			rotate = 2;
			reflect = true;
		} else if (finish_edge == DIR_E) {
			rotate = 3;
			x_dist = side - 1 - finish;
		} else {
			quit_fmt("Incorrect edge direction in map_corner_to_edge()");
		}
	} else {
		quit_fmt("Incorrect corner direction in map_corner_to_edge()");
	}

	/* This is too simplistic, as it can never wind back and forward. */
	course[y][x] = ++count;
	while ((y_dist > 0) || (x_dist > 0)) {
		if (randint0(y_dist + x_dist) < y_dist) {
			/* y move */
			y++;
			y_dist--;
			course[y][x] = ++count;
		} else if (randint0(y_dist + x_dist) < x_dist) {
			/* x move */
			x++;
			x_dist--;
			course[y][x] = ++count;
		} else {
			if (one_in_(2)) {
				/* Both move, y first */
				if (y_dist > 0) {
					y++;
					y_dist--;
					course[y][x] = ++count;
				}
				if (x_dist > 0) {
					x++;
					x_dist--;
					course[y][x] = ++count;
				}
			} else {
				/* Both move, x first */
				if (x_dist > 0) {
					x++;
					x_dist--;
					course[y][x] = ++count;
				}
				if (y_dist > 0) {
					y++;
					y_dist--;
					course[y][x] = ++count;
				}
			}
		}
	}

	/* Allocate temporary course array, copy data */
	temp = mem_zalloc(side * sizeof(uint16_t*));
	for (y = 0; y < side; y++) {
		temp[y] = mem_zalloc(side * sizeof(uint16_t));
	}
	//memcpy(temp, course, side * side * sizeof(uint16_t));
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			temp[y][x] = course[y][x];
		}
	}

	/* Do the symmetry transform */
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			struct loc grid = loc(x, y);
			symmetry_transform(&grid, 0, 0, side, side, rotate, reflect);
			course[grid.y][grid.x] = temp[y][x];
		}
	}

	/* Free the temporary array */
	for (y = 0; y < side; y++) {
		mem_free(temp[y]);
	}
	mem_free(temp);
	return count;
}

/**
 * Map a course from the corner of a square grid to the opposite corner.
 */
static int map_corner_to_corner(size_t side, enum direction start_corner,
								 uint16_t **course)
{
	size_t y = 0, x = 0;
	int y_dist = side - 1;
	int x_dist = side - 1;
	bool reflect = false;
	int count = 0;
	uint16_t **temp;

	/* Reflect if necessary */
	if ((start_corner == DIR_NE) || (start_corner == DIR_SW)) reflect = true;

	/* Start in the top left corner, chart a course to the bottom right.
	 * Slightly less simplistic than corner to edge, but not much. */
	course[y][x] = ++count;
	while ((y_dist > 0) || (x_dist > 0)) {
		if (one_in_(side / 5) && (y_dist > 0)) {
			/* y move */
			y++;
			y_dist--;
			course[y][x] = ++count;
		} else if (one_in_(side / 5) && (x_dist > 0)) {
			/* x move */
			x++;
			x_dist--;
			course[y][x] = ++count;
		} else {
			if (one_in_(2)) {
				/* Both move, y first */
				if (y_dist > 0) {
					y++;
					y_dist--;
				}
				course[y][x] = ++count;
				if (x_dist > 0) {
					x++;
					x_dist--;
				}
				course[y][x] = ++count;
			} else {
				/* Both move, x first */
				if (x_dist > 0) {
					x++;
					x_dist--;
				}
				course[y][x] = ++count;
				if (y_dist > 0) {
					y++;
					y_dist--;
				}
				course[y][x] = ++count;
			}
		}
		course[y][x] = ++count;
	}

	/* Allocate temporary course array, copy data */
	temp = mem_zalloc(side * sizeof(uint16_t*));
	for (y = 0; y < side; y++) {
		temp[y] = mem_zalloc(side * sizeof(uint16_t));
	}
	//memcpy(temp, course, side * side * sizeof(uint16_t));
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			temp[y][x] = course[y][x];
		}
	}

	/* Do the symmetry transform */
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			struct loc grid = loc(x, y);
			symmetry_transform(&grid, 0, 0, side, side, 0, reflect);
			course[grid.y][grid.x] = temp[y][x];
		}
	}

	/* Free the temporary array */
	for (y = 0; y < side; y++) {
		mem_free(temp[y]);
	}
	mem_free(temp);
	return count;
}

/**
 * Map a course from the edge of a square grid to the opposite edge.
 */
static int map_edge_to_opposite(size_t side, enum direction start_edge,
								 size_t start, size_t finish, uint16_t **course)
{
	size_t y = 0, x = start;
	int y_dist = side - 1;
	int x_dist = finish - start;
	int rotate = 0;
	int count = 0;
	uint16_t **temp;

	/* Rotate, reverse direction if necessary */
	if (start_edge == DIR_E) {
		rotate = 1;
	} else if (start_edge == DIR_S) {
		x = finish;
		x_dist = -x_dist;
	} else if (start_edge == DIR_W) {
		rotate = 1;
		x = finish;
		x_dist = -x_dist;
	}

	/* Start at the top, chart a course to the bottom.  Moderately sensible. */
	course[y][x] = ++count;
	while ((y_dist > 0) || (x_dist != 0)) {
		/* Just always progress in the main direction */
		if (y_dist > 0) {
			y++;
			y_dist--;
			course[y][x] = ++count;
		}
		/* If we have some room to play with, allow occasional bad x moves */
		if (y_dist > ABS(x_dist) - 1) {
			if (one_in_(y_dist + side)) {
				/* Bad move */
				if (x_dist < 0) {
					x++;
					x_dist--;
					course[y][x] = ++count;
				} else if (x_dist > 0) {
					x--;
					x_dist++;
					course[y][x] = ++count;
				} else if (one_in_(2) && (x < side - 1)) {
					x++;
					x_dist--;
					course[y][x] = ++count;
				} else if (x > 0) {
					x--;
					x_dist++;
					course[y][x] = ++count;
				}
			} else if (one_in_(y_dist - ABS(x_dist))) {
				/* Good move */
				if (x_dist < 0) {
					x--;
					x_dist++;
					course[y][x] = ++count;
				} else if (x_dist > 0) {
					x++;
					x_dist--;
					course[y][x] = ++count;
				}
			}
		} else {
			/* We have no latitude, head straight for the finish */
			if (x_dist > 0) {
				x++;
				x_dist--;
				course[y][x] = ++count;
			} else if (x_dist < 0) {
				x--;
				x_dist++;
				course[y][x] = ++count;
			}
		}
	}

	/* Allocate temporary course array, copy data */
	temp = mem_zalloc(side * sizeof(uint16_t*));
	for (y = 0; y < side; y++) {
		temp[y] = mem_zalloc(side * sizeof(uint16_t));
	}
	//memcpy(temp, course, side * side * sizeof(uint16_t));
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			temp[y][x] = course[y][x];
		}
	}

	/* Do the symmetry transform */
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			struct loc grid = loc(x, y);
			symmetry_transform(&grid, 0, 0, side, side, rotate, false);
			course[grid.y][grid.x] = temp[y][x];
		}
	}

	/* Free the temporary array */
	for (y = 0; y < side; y++) {
		mem_free(temp[y]);
	}
	mem_free(temp);
	return count;
}

/**
 * Map a course from the edge of a square grid to an adjacent edge.
 */
static int map_edge_to_adjacent(size_t side, enum direction start_edge,
								 size_t start, enum direction finish_edge,
								 size_t finish, uint16_t **course)
{
	size_t y = 0, x = start;
	int y_dist = finish;
	int x_dist = side - 1 - start;
	int rotate = 0;
	bool reflect = false;
	int count = 0;
	uint16_t **temp;

	/* Rotate and adjust start and finish points if necessary. Given there are
	 * only four basic configurations (cutting off the four corners) we can
	 * choose not to use reflects at all. */
	if (start_edge == DIR_N) {
		if (finish_edge == DIR_E) {
			/* All set */
		} else if (finish_edge == DIR_W) {
			rotate = 3;
			y_dist = start;
			x = finish;
			x_dist = side - 1 - finish;
		} else {
			quit_fmt("Incorrect finish edge in map_edge_to_adjacent()");
		}
	} else if (start_edge == DIR_E) {
		if (finish_edge == DIR_S) {
			rotate = 1;
			y_dist = side - 1 - finish;
		} else if (finish_edge == DIR_N) {
			y_dist = start;
			x = finish;
			x_dist = side - 1 - finish;
		} else {
			quit_fmt("Incorrect finish edge in map_edge_to_adjacent()");
		}
	} else if (start_edge == DIR_S) {
		if (finish_edge == DIR_W) {
			rotate = 2;
			x = side - 1 - start;
			x_dist = start;
			y_dist = side - 1 - finish;
		} else if (finish_edge == DIR_E) {
			rotate = 1;
			y_dist = start;
			x = finish;
			x_dist = side - 1 - finish;
		} else {
			quit_fmt("Incorrect finish edge in map_edge_to_adjacent()");
		}
	} else if (start_edge == DIR_W) {
		if (finish_edge == DIR_N) {
			rotate = 3;
			y_dist = finish;
			x = side - 1 - start;
			x_dist = start;
		} else if (finish_edge == DIR_S) {
			rotate = 2;
			y_dist = side - 1 - start;
			x = side - 1 - finish;
			x_dist = finish;
		} else {
			quit_fmt("Incorrect finish edge in map_edge_to_adjacent()");
		}
	} else {
		quit_fmt("Incorrect start edge in map_corner_to_edge()");
	}

	/* Start at the top, chart a course to the right edge.
	 * Very like corner to corner. */
	course[y][x] = ++count;
	while ((y_dist > 0) || (x_dist > 0)) {
		if (one_in_((finish + side - start) / 10) && (y_dist > 0)) {
			y++;
			y_dist--;
			course[y][x] = ++count;
		} else if (one_in_((finish + side - start) / 10) && (x_dist > 0)) {
			x++;
			x_dist--;
			course[y][x] = ++count;
		} else {
			if (y_dist > x_dist) {
				y++;
				y_dist--;
				course[y][x] = ++count;
			} else if (x_dist > y_dist) {
				x++;
				x_dist--;
				course[y][x] = ++count;
			} else {
				if (y_dist > 0) {
					y++;
					y_dist--;
					course[y][x] = ++count;
				}
				if (x_dist > 0) {
					x++;
					x_dist--;
					course[y][x] = ++count;
				}
			}
		}
	}

	/* Allocate temporary course array, copy data */
	temp = mem_zalloc(side * sizeof(uint16_t*));
	for (y = 0; y < side; y++) {
		temp[y] = mem_zalloc(side * sizeof(uint16_t));
	}
	//memcpy(temp, course, side * side * sizeof(uint16_t));
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			temp[y][x] = course[y][x];
		}
	}

	/* Do the symmetry transform */
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			struct loc grid = loc(x, y);
			symmetry_transform(&grid, 0, 0, side, side, rotate, reflect);
			course[grid.y][grid.x] = temp[y][x];
		}
	}

	/* Free the temporary array */
	for (y = 0; y < side; y++) {
		mem_free(temp[y]);
	}
	mem_free(temp);
	return count;
}

/**
 * Find the next river mile up- or downstream from this one
 */
static struct river_mile *next_river_mile(struct river_mile *r_mile, bool up,
										  bool second)
{
	assert(r_mile->stretch);
	if (up) {
		if (r_mile->upstream) {
			/* There's an obvious one */
			return r_mile->upstream;
		} else {
			/* Pick the first incoming stretch */
			struct river_stretch *stretch = r_mile->stretch->in1;

			/* Change if necessary */
			if (second) {
				stretch = r_mile->stretch->in2;
			}

			/* Find the last mile of this stretch */
			if (stretch) {
				struct river_mile *up_mile = stretch->miles;
				while (up_mile->downstream) {
					up_mile = up_mile->downstream;
				}
				return up_mile;
			}
		}
	} else  {
		if (r_mile->downstream) {
			/* There's an obvious one */
			return r_mile->downstream;
		} else {
			/* Pick the first outgoing stretch */
			struct river_stretch *stretch = r_mile->stretch->out1;

			/* Change if necessary */
			if (second) {
				stretch = r_mile->stretch->out2;
			}

			/* Just need the first mile */
			return stretch->miles;
		}
	}
	return NULL;
}

/**
 * Find the chunk where a river crosses a given square mile boundary.
 *
 * This function only checks cardinal directions, and needs to be used twice
 * for finding rivers coming in (technically) diagonally.
 */
static void find_river_chunk(struct square_mile *sq_mile, struct loc *int_chunk,
							 struct loc *ext_chunk, enum direction dir)
{
	size_t i;
	bool vertical = (dir == DIR_N) || (dir == DIR_S);

	/* Coordinates of this square mile in the square_miles array */
	int x = sq_mile->map_grid.x, y = sq_mile->map_grid.y;

	/* Coordinates of the chunk in the top left corner */
	int tl_x = x * CPM, tl_y = y * CPM;

	/* Only cardinal directions */
	assert(dir % 2 == 0);
		
	/* Check along the boundary for adjacent river pieces already marked */
	for (i = 0; i < CPM; i++) {
		int lower, upper;
		if (vertical) {
			/* Bottom edge of mile above, or top edge of mile below */
			int use_y = (dir == DIR_N) ? tl_y - 1 : tl_y + CPM;
			bool found = gen_loc_find(tl_x + i, use_y, 0, &lower, &upper);
			if (found) {
				struct gen_loc location = gen_loc_list[upper];
				if (location.river_piece) {
					/* Chunk in the adjacent square mile */
					*ext_chunk = loc(tl_x + i, use_y);

					/* Chunk in the current square mile */
					use_y = (dir == DIR_N) ? tl_y : tl_y + CPM - 1;
					*int_chunk = loc(tl_x + i, use_y);
				}
			}
		} else {
			/* Right edge of mile left, or left edge of mile right */
			int use_x = (dir == DIR_W) ? tl_x - 1 : tl_x + CPM;
			bool found = gen_loc_find(use_x, tl_y + i, 0, &lower, &upper);
			if (found) {
				struct gen_loc location = gen_loc_list[upper];
				if (location.river_piece) {
					/* Chunk in the adjacent square mile */
					*ext_chunk = loc(use_x, tl_y + i);

					/* Chunk in the current square mile */
					use_x = (dir == DIR_W) ? tl_x : tl_x + CPM - 1;
					*int_chunk = loc(use_x, tl_y + i);
				}
			}
		}
	}
}

/**
 * Find any adjacent chunks to this square mile with river edges already set
 */
static void square_mile_river_borders(struct square_mile *sq_mile,
									  enum direction start_dir,
									  struct loc *start,
									  struct loc *start_adj,
									  enum direction finish_dir,
									  struct loc *finish,
									  struct loc *finish_adj)
{
	enum direction dir;

	/* Start */
	if (start_dir % 2 == 0) {
		/* Cardinal direction, simple check */
		find_river_chunk(sq_mile, start, start_adj, start_dir);
	} else {
		/* Diagonal, check cardinal direction anti-clockwise */
		dir = cycle[chome[start_dir] + 1];
		find_river_chunk(sq_mile, start, start_adj, dir);

		/* Check clockwise if necessary, note that only one should occur */
		if (start->x < 0) {
			dir = cycle[chome[start_dir] - 1];
			find_river_chunk(sq_mile, start, start_adj, dir);
		}
	}

	/* Finish */
	if (finish_dir % 2 == 0) {
		/* Cardinal direction, simple check */
		find_river_chunk(sq_mile, finish, finish_adj, finish_dir);
	} else {
		/* Diagonal, check cardinal direction anti-clockwise */
		dir = cycle[chome[finish_dir] + 1];
		find_river_chunk(sq_mile, finish, finish_adj, dir);

		/* Check clockwise if necessary, note that only one should occur */
		if (finish->x < 0) {
			dir = cycle[chome[finish_dir] - 1];
			find_river_chunk(sq_mile, finish, finish_adj, dir);
		}
	}
}

/**
 * Map the course of a river (or road?) across a square grid.
 *
 * \param side is the side length of the grid
 * \param start_dir is the direction where the course starts
 * \param start is the starting point outside the start side, if known
 * \param finish_dir is the direction where the course finishes
 * \param finish is the finishing point outside the finish side, if known
 * \param course is an array showing which grids are included
 */
static int map_course(size_t side, enum direction start_dir, struct loc *start,
					   enum direction finish_dir, struct loc *finish,
					   uint16_t **course)
{
	int start_point = -1, finish_point = -1;
	int num = 0;
	bool source = (start_dir == DIR_NONE);
	bool sink = (finish_dir == DIR_NONE);

	/* Choose a start point where necessary */
	if (start->x < 0) {
		/* Pick a random point along the border (not needed for diagonals) */
		start_point = randint0(side);

		/* Check if we are continuing from upstream */
		if (source) {
			/* Go straight across, truncate later */
			assert(!sink);
			start_dir = opposite_dir(finish_dir);
		}

		/* Record start */
		switch (start_dir) {
			case DIR_N: *start = loc(start_point, 0); break;
			case DIR_NE: *start = loc(side - 1, 0); break;
			case DIR_E: *start = loc(side - 1, start_point); break;
			case DIR_SE: *start = loc(side - 1, side - 1); break;
			case DIR_S: *start = loc(start_point, side - 1); break;
			case DIR_SW: *start = loc(0, side - 1); break;
			case DIR_W: *start = loc(0, start_point); break;
			case DIR_NW: *start = loc(0, 0); break;
			default:quit_fmt("No start in map_course().");
		}
	} else {
		/* Pick the correct coordinate */
		assert(start_dir % 2 == 0);
		if ((start_dir == DIR_N) || (start_dir == DIR_S)) {
			start_point = start->x % side;
		} else {
			start_point = start->y % side;
		}
	}

	/* Now choose a finish point where necessary */
	if (finish->x < 0) {
		/* Pick a random point along the border (not needed for diagonals) */
		finish_point = randint0(side);

		/* Check if we are continuing downstream */
		if (sink) {
			/* Go straight across, truncate later */
			assert(!source);
			finish_dir = opposite_dir(start_dir);
		}

		/* Record finish */
		switch (finish_dir) {
			case DIR_N: *finish = loc(finish_point, 0); break;
			case DIR_NE: *finish = loc(side - 1, 0); break;
			case DIR_E: *finish = loc(side - 1, finish_point); break;
			case DIR_SE: *finish = loc(side - 1, side - 1); break;
			case DIR_S: *finish = loc(finish_point, side - 1); break;
			case DIR_SW: *finish = loc(0, side - 1); break;
			case DIR_W: *finish = loc(0, finish_point); break;
			case DIR_NW: *finish = loc(0, 0); break;
			default:quit_fmt("No finish in map_course().");
		}
	} else {
		/* Pick the correct coordinate */
		assert(finish_dir % 2 == 0);
		if ((finish_dir == DIR_N) || (finish_dir == DIR_S)) {
			finish_point = finish->x % side;
		} else {
			finish_point = finish->y % side;
		}
	}


	/* Now do the actual course */
	if (start_dir % 2) {
		/* Start is a corner */
		if (finish_dir % 2) {
			/* Corner to corner */
			assert(finish_dir == opposite_dir(start_dir));
			num = map_corner_to_corner(side, start_dir, course);
		} else {
			/* Corner to non-adjacent edge */
			assert((cycle[chome[finish_dir] + 1] != start_dir) &&
				   (cycle[chome[finish_dir] - 1] != start_dir));
			num = map_corner_to_edge(side, start_dir, finish_dir, finish_point,
									 course);
		}
	} else {
		/* Start is an edge */
		if (finish_dir % 2) {
			/* Edge to corner - do corner to edge in reverse */
			assert((cycle[chome[finish_dir] + 1] != start_dir) &&
				   (cycle[chome[finish_dir] - 1] != start_dir));
			num = map_corner_to_edge(side, finish_dir, start_dir, start_point,
									 course);
		} else if (finish_dir == opposite_dir(start_dir)) {
			/* Edge to opposite edge */
			num = map_edge_to_opposite(side, start_dir, start_point,
									   finish_point, course);
		} else {
			/* Edge to adjacent edge */
			num = map_edge_to_adjacent(side, start_dir, start_point, finish_dir,
									   finish_point, course);
		}
	}

	/* Truncate if necessary */
	if (source) {
		size_t y, x;
		int max = 0, newnum = num / 2;

		assert(start->x < 0);

		/* Scrub out the first half of the course */
		for (y = 0; y < side; y++) {
			for (x = 0; x < side; x++) {
				if (course[y][x] <= newnum) {
					course[y][x] = 0;
				} else if (course[y][x] > newnum) {
					course[y][x] -= newnum;
					if (course[y][x] > max) {
						max = course[y][x];
						*start = loc(x, y);
					}
				}
			}
		}
		num -= newnum;
		start_dir = DIR_NONE;
	} else if (sink) {
		size_t y, x;
		int max = 0, newnum = num / 2;

		assert(finish->x < 0);

		/* Scrub out the second half of the course */
		for (y = 0; y < side; y++) {
			for (x = 0; x < side; x++) {
				if (course[y][x] > newnum) {
					course[y][x] = 0;
				} else if (course[y][x] > 0) {
					if (course[y][x] > max) {
						max = course[y][x];
						*finish = loc(x, y);
					}
				}
			}
		}
		num = newnum;
		finish_dir = DIR_NONE;
	}

	return num;
}

/**
 * Get the horizontal direction of travel from a grid to another grid given
 * their local coordinates in an array of squares of side x side grids.
 *
 * \param here is the grid the direction is relative to
 * \param from is the grid in that direction
 * \param side is the maximum coordinate within a square of grids
 */
static int grid_direction(struct loc here, struct loc from, int side)
{
	enum direction dir;
	struct loc offset = loc_diff(from, here);
	if (ABS(offset.x) == (side - 1)) offset.x = -1;
	if (ABS(offset.y) == (side - 1)) offset.y = -1;
	for (dir = DIR_HOR_MIN; dir < DIR_HOR_MAX; dir++) {
		if (loc_eq(offset, ddgrid[dir])) break;
	}
	assert(dir < DIR_HOR_MAX);
	assert(dir != DIR_NONE);
	return dir;
}

/**
 * Test if a grid could be immediately outside an array of squares of
 * side x side grids in the given direction.
 *
 * \param grid is grid being tested
 * \param dir is the direction - must be cardinal
 * \param side is the maximum coordinate within a square of grids
 */
static bool grid_outside(struct loc grid, enum direction dir, int side)
{
	int coord;
	assert(dir % 2 == 0);
	for (coord = 0; coord < side; coord++) {
		if ((dir == DIR_N) && loc_eq(grid, loc(coord, side - 1))) return true;
		if ((dir == DIR_E) && loc_eq(grid, loc(0, coord))) return true;
		if ((dir == DIR_S) && loc_eq(grid, loc(coord, 0))) return true;
		if ((dir == DIR_W) && loc_eq(grid, loc(side - 1, coord))) return true;
	}
	return false;
}

/**
 * Get the river width at a particular river mile.
 */
static int get_river_width(struct river_mile *r_mile)
{
	struct river_mile *upstream = next_river_mile(r_mile, true, false);
	int count = 0;
	while (upstream) {
		count++;
		upstream = next_river_mile(upstream, true, false);
	}
	return 1 + (count / WIDEN_RATIO);
}

/**
 *
 */
static bool grid_in_square(int side, struct loc grid)
{
	return ((grid.x >= 0) && (grid.x < side) && (grid.y >= 0) &&
			(grid.y < side));
}

/**
 * Widen the course of a river in the given diagonal direction to the given
 * width.
 *
 * This algorithm adds the diagonal grid and the two adjacent cardinal grids
 * for the given direction from each existing grid. This should result in a
 * proper widening, although it will not work very well if the diagonal gets
 * close to parallel to the river.
 *
 * This also has the problem of being truncated at the edge of the square.
 */
//TODO RIVER Both these issues need addressing
static int widen_river_course(int side, uint16_t **course, enum direction dir,
							  int width)
{
	struct loc grid, new;
	int i, count = 1;

	/* Find the biggest label */
	for (grid.y = 0; grid.y < side; grid.y++) {
		for (grid.x = 0; grid.x < side; grid.x++) {
			count = MAX(course[grid.y][grid.x], count);
		}
	}

	/* Widen the correct number of times */
	for (i = 1; i < width; i++) {
		/* Allocate widen array */
		bool **widen = mem_zalloc(side * sizeof(bool*));
		int y;
		for (y = 0; y < side; y++) {
			widen[y] = mem_zalloc(side * sizeof(bool));
		}

		/* Pick widening grids */
		assert((dir != DIR_NONE) && (dir % 2));
		for (grid.y = 0; grid.y < side; grid.y++) {
			for (grid.x = 0; grid.x < side; grid.x++) {
				if (!course[grid.y][grid.x]) continue;

				/* Add diagonal */
				new = loc_sum(grid, ddgrid[dir]);
				if (grid_in_square(side, new)) widen[new.y][new.x] = true;

				/* Add cardinal anti-clockwise */
				new = loc_sum(grid, ddgrid[cycle[chome[dir] + 1]]);
				if (grid_in_square(side, new)) widen[new.y][new.x] = true;

				/* Add cardinal clockwise */
				new = loc_sum(grid, ddgrid[cycle[chome[dir] - 1]]);
				if (grid_in_square(side, new)) widen[new.y][new.x] = true;
			}
		}

		/* Add the widening grids */
		for (grid.y = 0; grid.y < side; grid.y++) {
			for (grid.x = 0; grid.x < side; grid.x++) {
				if (!course[grid.y][grid.x] && widen[grid.y][grid.x]) {
					course[grid.y][grid.x] = count++;
				}
			}
		}

		/* Free the widen array */
		for (y = 0; y < side; y++) {
			mem_free(widen[y]);
		}
		mem_free(widen);
	}
	return count;
}

/**
 *
 */
static struct river_piece *find_chunk_river_piece(struct loc grid)
{
	int lower, upper;
	bool found;
	if ((grid.y < 0) || (grid.y >= CPM * MAX_Y_REGION - 1) ||
		(grid.x < 0) || (grid.x >= CPM * MAX_X_REGION - 1)) return NULL;
	found = gen_loc_find(grid.x, grid.y, 0, &lower, &upper);
	if (found) return gen_loc_list[upper].river_piece;
	return NULL;
}

/**
 * Find the grid of a course labelled with a given number
 */
static struct loc find_course_index(int side, int index, uint16_t **course)
{
	int x, y;
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			if (course[y][x] == index) return loc(x, y);
		}
	}
	return loc(-1, -1);		
}

static struct loc get_external_river_connect(enum direction dir,
											 struct river_piece *piece)
{
	struct loc grid = loc(-1, -1);
	int min = CHUNK_SIDE - 1, max = 0;
	struct river_grid *rgrid = piece->grids;

	/* Find the range of adjacent grids */
	while (rgrid) {
		struct loc test = rgrid->grid;
		if (grid_outside(test, dir, CHUNK_SIDE)) {
			if ((dir == DIR_N) || (dir == DIR_S)) {
				if (test.x > max) max = test.x;
				if (test.x < min) min = test.x;
			} else {
				if (test.y > max) max = test.y;
				if (test.y < min) min = test.y;
			}
		}
		rgrid = rgrid->next;
	}

	/* Pick the grid to connect with existing external river */
	if (min < max) {
		if (dir == DIR_N) {
			grid = loc((min + max) / 2, 0);
		} else if (dir == DIR_E) {
			grid = loc(CHUNK_SIDE - 1, (min + max) / 2);
		} else if (dir == DIR_S) {
			grid = loc((min + max) / 2, CHUNK_SIDE - 1);
		} else if (dir == DIR_W) {
			grid = loc(0, (min + max) / 2);
		}
	}

	/* Check it's a valid grid */
	if ((grid.x < 0) || (grid.y < 0)) quit_fmt("Failed to connect river piece");

	return grid;
}

static void write_river_piece(uint16_t **course, struct gen_loc *location)
{
	int y, x, count = 0;

	/* Write the grids */
	for (y = 0; y < CHUNK_SIDE; y++) {
		for (x = 0; x < CHUNK_SIDE; x++) {
			if (course[y][x]) {
				struct river_grid *rgrid = mem_zalloc(sizeof(*rgrid));
				rgrid->next = location->river_piece->grids;
				rgrid->grid = loc(x, y);
				location->river_piece->grids = rgrid;
				count++;
			}
		}
	}
	location->river_piece->num_grids = count;
}

/**
 * Write pieces for each location in a mapped course across a square mile for a
 * river mile.
 *
 * For courses starting in corners, write edges in adjacent square miles
 * which are incidentally cut through although they don't technically
 * contain the river.
 */
static void write_river_pieces(struct square_mile *sq_mile,
							   struct river_mile *r_mile,
							   enum direction start_dir, struct loc start,
							   struct loc start_adj, enum direction finish_dir,
							   struct loc finish, struct loc finish_adj,
							   uint16_t **course, int num)
{
	int k;

	/* Coordinates of the chunk in the top left corner */
	struct loc tl = loc(sq_mile->map_grid.x * CPM, sq_mile->map_grid.y * CPM);

	struct loc prev_chunk = start_adj;
	struct loc current_chunk = loc_sum(find_course_index(CPM, 1, course), tl);
	struct loc in_grid = loc(-1, -1), out_grid = loc(-1, -1);
	struct loc exit_grid = loc(-1, -1);
	enum direction in_dir = DIR_NONE, out_dir = DIR_NONE, widen_dir = DIR_NONE;

	/* Get river width */
	int width = get_river_width(r_mile);

	/* Check the chunks before the start and after the end of river */
	struct river_piece *river_piece_s = find_chunk_river_piece(start_adj);
	struct river_piece *river_piece_f = find_chunk_river_piece(finish_adj);

	/* Are we putting in river as a connector between diagonal square miles? */
	bool start_connect = (start_dir % 2) && (start_dir != DIR_NONE);
	bool finish_connect = (finish_dir % 2) && (finish_dir != DIR_NONE);

	/* Get the direction for widening the river if needed */
	if (width > 1) {
		/* Always choose as perpendicular a direction as possible */
		bool right = (finish.x > start.x) ||
			((finish.x == start.x) && one_in_(2));
		bool down = (finish.y > start.y) ||
			((finish.y == start.y) && one_in_(2));
		if (right) {
			if (down) {
				widen_dir = one_in_(2) ? DIR_SW : DIR_NE;
			} else {
				widen_dir = one_in_(2) ? DIR_SE : DIR_NW;
			}
		} else {
			if (down) {
				widen_dir = one_in_(2) ? DIR_NW : DIR_SE;
			} else {
				widen_dir = one_in_(2) ? DIR_NE : DIR_SW;
			}
		}
	}

	/* Set direction for any incoming river from a set external chunk. */
	if (river_piece_s || start_connect) {
		in_dir = grid_direction(start, start_adj, CHUNK_SIDE);
		assert(in_dir % 2 == 0);

		if (river_piece_s) {
			/* There's already an external piece of river */
			in_grid = get_external_river_connect(in_dir, river_piece_s);
		} else {
			/* Make external river and remember where we come in */
			int y;
			int start_point = randint0(CHUNK_SIDE);
			int finish_point = randint0(CHUNK_SIDE);
			int lower, upper;
			bool reload;
			struct gen_loc *location;
			
			/* Allocate in-chunk course array */
			uint16_t **course1 = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t*));
			for (y = 0; y < CHUNK_SIDE; y++) {
				course1[y] = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t));
			}
			
			/* Work out the entry and exit points and directions */
			if (start_dir == DIR_NE) {
				if (start_adj.x == start.x) {
					/* North */
					in_dir = DIR_E;
					out_dir = DIR_S;
					in_grid = loc(CHUNK_SIDE - 1, start_point);
					out_grid = loc(finish_point, CHUNK_SIDE - 1);
				} else {
					/* East */
					in_dir = DIR_N;
					out_dir = DIR_W;
					in_grid = loc(start_point, 0);
					out_grid = loc(0, finish_point);
				}
			} else if (start_dir == DIR_SE) {
				if (start_adj.x == start.x) {
					/* South */
					in_dir = DIR_E;
					out_dir = DIR_N;
					in_grid = loc(CHUNK_SIDE - 1, start_point);
					out_grid = loc(finish_point, 0);
				} else {
					/* East */
					in_dir = DIR_S;
					out_dir = DIR_W;
					in_grid = loc(start_point, CHUNK_SIDE - 1);
					out_grid = loc(0, finish_point);
				}
			} else if (start_dir == DIR_SW) {
				if (start_adj.x == start.x) {
					/* South */
					in_dir = DIR_W;
					out_dir = DIR_N;
					in_grid = loc(0, start_point);
					out_grid = loc(finish_point, 0);
				} else {
					/* West */
					in_dir = DIR_S;
					out_dir = DIR_E;
					in_grid = loc(start_point, CHUNK_SIDE - 1);
					out_grid = loc(CHUNK_SIDE - 1, finish_point);
				}
			} else if (start_dir == DIR_NW) {
				if (start_adj.x == start.x) {
					/* North */
					in_dir = DIR_W;
					out_dir = DIR_S;
					in_grid = loc(0, start_point);
					out_grid = loc(finish_point, CHUNK_SIDE - 1);
				} else {
					/* West */
					in_dir = DIR_N;
					out_dir = DIR_E;
					in_grid = loc(start_point, 0);
					out_grid = loc(CHUNK_SIDE - 1, finish_point);
				}
			}

			/* Map a course across the chunk */
			(void) map_course(CHUNK_SIDE, in_dir, &in_grid, out_dir,
							  &out_grid, course1);

			/* Set in_grid and in_dir for next chunk */
			if (out_dir == DIR_N) {
				in_grid = loc(finish_point, CHUNK_SIDE - 1);
				in_dir = DIR_S;
			} else if (out_dir == DIR_E) {
				in_grid = loc(0, finish_point);
				in_dir = DIR_W;
			} else if (out_dir == DIR_S) {
				in_grid = loc(finish_point, 0);
				in_dir = DIR_N;
			} else {
				in_grid = loc(CHUNK_SIDE - 1, finish_point);
				in_dir = DIR_E;
			}

			/* Widen */
			widen_river_course(CHUNK_SIDE, course1, widen_dir, width);

			/* Get the location, confirming it hasn't been written before */
			reload = gen_loc_find(start_adj.x, start_adj.y, 0, &lower, &upper);
			if (reload) {
				quit_fmt("Trying to create existing location");
			} else {
				gen_loc_make(start_adj.x, start_adj.y, 0, upper);
				location = &gen_loc_list[upper];
				location->river_piece = mem_zalloc(sizeof(struct river_piece));
			}

			/* Write the river piece */
			write_river_piece(course1, location);

			/* Free the course array */
			for (y = 0; y < CHUNK_SIDE; y++) {
				mem_free(course1[y]);
			}
			mem_free(course1);
		}
	}

	/* Set direction for any outgoinging river to a set external chunk. */
	if (river_piece_f || finish_connect) {
		out_dir = grid_direction(finish_adj, finish, CHUNK_SIDE);
		assert(out_dir % 2 == 0);

		if (river_piece_f) {
			/* There's already an external piece of river */
			exit_grid = get_external_river_connect(out_dir, river_piece_f);
		} else {
			/* Make external river and remember where we leave */
			int y;
			int start_point = randint0(CHUNK_SIDE);
			int finish_point = randint0(CHUNK_SIDE);
			int lower, upper;
			bool reload;
			struct gen_loc *location;

			/* Allocate in-chunk course array */
			uint16_t **course1 = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t*));
			for (y = 0; y < CHUNK_SIDE; y++) {
				course1[y] = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t));
			}

			/* Work out the entry and exit points and directions */
			if (finish_dir == DIR_NE) {
				if (finish_adj.x == finish.x) {
					/* North */
					out_dir = DIR_E;
					in_dir = DIR_S;
					out_grid = loc(CHUNK_SIDE - 1, finish_point);
					in_grid = loc(start_point, CHUNK_SIDE - 1);
				} else {
					/* East */
					out_dir = DIR_N;
					in_dir = DIR_W;
					out_grid = loc(finish_point, 0);
					in_grid = loc(0, start_point);
				}
			} else if (finish_dir == DIR_SE) {
				if (finish_adj.x == finish.x) {
					/* South */
					out_dir = DIR_E;
					in_dir = DIR_N;
					out_grid = loc(CHUNK_SIDE - 1, finish_point);
					in_grid = loc(start_point, 0);
				} else {
					/* East */
					out_dir = DIR_S;
					in_dir = DIR_W;
					out_grid = loc(finish_point, CHUNK_SIDE - 1);
					in_grid = loc(0, start_point);
				}
			} else if (finish_dir == DIR_SW) {
				if (finish_adj.x == finish.x) {
					/* South */
					out_dir = DIR_W;
					in_dir = DIR_N;
					out_grid = loc(0, finish_point);
					in_grid = loc(start_point, 0);
				} else {
					/* West */
					out_dir = DIR_S;
					in_dir = DIR_E;
					out_grid = loc(finish_point, CHUNK_SIDE - 1);
					in_grid = loc(CHUNK_SIDE - 1, start_point);
				}
			} else if (finish_dir == DIR_NW) {
				if (finish_adj.x == finish.x) {
					/* North */
					out_dir = DIR_W;
					in_dir = DIR_S;
					out_grid = loc(0, finish_point);
					in_grid = loc(start_point, CHUNK_SIDE - 1);
				} else {
					/* West */
					out_dir = DIR_N;
					in_dir = DIR_E;
					out_grid = loc(finish_point, 0);
					in_grid = loc(CHUNK_SIDE - 1, start_point);
				}
			}

			/* Map a course across the chunk */
			(void) map_course(CHUNK_SIDE, in_dir, &in_grid, out_dir,
							  &out_grid, course1);

			/* Set exit_grid for final chunk */
			if (in_dir == DIR_N) {
				exit_grid = loc(start_point, CHUNK_SIDE - 1);
			} else if (in_dir == DIR_E) {
				exit_grid = loc(0, start_point);
			} else if (in_dir == DIR_S) {
				exit_grid = loc(start_point, 0);
			} else {
				exit_grid = loc(CHUNK_SIDE - 1, start_point);
			}

			/* Widen */
			widen_river_course(CHUNK_SIDE, course1, widen_dir, width);

			/* Get the location, confirming it hasn't been written before */
			reload = gen_loc_find(finish_adj.x, finish_adj.y, 0, &lower,
								  &upper);
			if (reload) {
				quit_fmt("Trying to create existing location");
			} else {
				gen_loc_make(finish_adj.x, finish_adj.y, 0, upper);
				location = &gen_loc_list[upper];
				location->river_piece = mem_zalloc(sizeof(struct river_piece));
			}

			/* Write the river piece */
			write_river_piece(course1, location);

			/* Free the course array */
			for (y = 0; y < CHUNK_SIDE; y++) {
				mem_free(course1[y]);
			}
			mem_free(course1);
		}
	}

	/* Progress along the square mile course, writing river in every chunk */
	for (k = 1; k <= num; k++) {
		struct loc next_chunk = (k < num) ?
			loc_sum(find_course_index(CPM, k + 1, course), tl) : finish_adj;
		int y;
		int lower, upper;
		bool reload;
		struct gen_loc *location;

		/* Allocate in-chunk course array */
		uint16_t **course1 = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t*));
		for (y = 0; y < CHUNK_SIDE; y++) {
			course1[y] = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t));
		}

		/* Get entry and exit directions */
		if (k > 1) {
			in_dir = grid_direction(current_chunk, prev_chunk, CPM);
		}
		if (k < num) {
			out_dir = grid_direction(current_chunk, next_chunk, CPM);
			out_grid = loc(-1, -1);
		} else {
			out_dir = finish_dir;
			out_grid = exit_grid;
		}

		//TODO RIVER handle sources and sinks

		/* Map a course across the chunk */
		(void) map_course(CHUNK_SIDE, in_dir, &in_grid, out_dir, &out_grid,
						  course1);

		/* Write new in_grid adjacent to out_grid in out_dir */
		in_grid = loc_sum(out_grid, ddgrid[out_dir]);

		/* Widen */
		widen_river_course(CHUNK_SIDE, course1, widen_dir, width);

		/* Get the location, confirming it hasn't been written before */
		reload = gen_loc_find(current_chunk.x, current_chunk.y, 0, &lower,
							  &upper);
		if (reload) {
			struct chunk_ref pref = chunk_list[player->place];
			/* Check the generated chunk is in the current playing arena */
			if ((ABS(pref.x_pos - current_chunk.x) <= ARENA_CHUNKS / 2) &&
				(ABS(pref.y_pos - current_chunk.y) <= ARENA_CHUNKS / 2)) {
				location = &gen_loc_list[upper];
				location->river_piece = mem_zalloc(sizeof(struct river_piece));
			} else {
				quit_fmt("Trying to create existing location");
			}
		} else {
			gen_loc_make(current_chunk.x, current_chunk.y, 0, upper);
			location = &gen_loc_list[upper];
			location->river_piece = mem_zalloc(sizeof(struct river_piece));
		}

		/* Write the river piece */
		write_river_piece(course1, location);

		/* Prepare for the next chunk */
		prev_chunk = current_chunk;
		current_chunk = next_chunk;

		/* Free the course array */
		for (y = 0; y < CHUNK_SIDE; y++) {
			mem_free(course1[y]);
		}
		mem_free(course1);
	}
}

/**
 * Map out the course of rivers through a square mile.
 *
 * This function is called on the player first entering a square mile, and it
 * writes river edges into all the locations that it deems any river to pass
 * through, creating these locations first.
 */
void map_river_miles(struct square_mile *sq_mile)
{
	struct river_mile *r_mile;
	bool two_up = false;
	bool two_down = false;

	/* Already mapped */
	if (sq_mile->mapped) return;

	/* Check each river mile that passes through (three maximum) */
	for (r_mile = sq_mile->river_miles; r_mile; r_mile = r_mile->next) {
		/* Starting and finishing directions for the course */
		enum direction start_dir = DIR_NONE, finish_dir = DIR_NONE;

		/* Start and finish locations (in global chunk coordinates) */
		struct loc start = loc(-1, -1), finish = loc(-1, -1);

		/* Adjacent chunks to start and finish outside this square mile */
		struct loc start_adj = loc(-1, -1), finish_adj = loc(-1, -1);

		/* Coordinates of start and finish in the square mile (CPMxCPM) */
		struct loc start_local = loc(-1, -1), finish_local = loc(-1, -1);

		/* Adjacent river miles upstream and downstream */
		struct river_mile *upstream = next_river_mile(r_mile, true, two_up),
			*downstream = next_river_mile(r_mile, false, two_down);

		/* Allocate course array */
		uint16_t **course = mem_zalloc(CPM * sizeof(uint16_t*));
		int y, num = 0;
		for (y = 0; y < CPM; y++) {
			course[y] = mem_zalloc(CPM * sizeof(uint16_t));
		}

		/* Find the incoming and outgoing directions */
		if (upstream) {
			start_dir = grid_direction(sq_mile->map_grid,
									   upstream->sq_mile->map_grid, MPS);
			two_up = true;
		}
		if (downstream) {
			finish_dir = grid_direction(sq_mile->map_grid,
										downstream->sq_mile->map_grid, MPS);
			two_down = true;
		}

		/* Set starting and finishing points */
		square_mile_river_borders(sq_mile, start_dir, &start, &start_adj,
								  finish_dir, &finish, &finish_adj);

		/* Set local-to-square-mile coordinates for start and finish points
		 * if they are set */
		if ((start.x >= 0) && (start.y >= 0)) {
			start_local = loc(start.x % CPM, start.y % CPM);
		}
		if ((finish.x >= 0) && (finish.y >= 0)) {
			finish_local = loc(finish.x % CPM, finish.y % CPM);
		}

		/* Map the chunks the river crosses */
		num = map_course(CPM, start_dir, &start_local, finish_dir,
						 &finish_local, course);

		/* Update start and finish chunks */
		assert((start_local.x >= 0) && (start_local.y >= 0) &&
			   (finish_local.x >= 0) && (finish_local.y >= 0));
		if ((start.x < 0) && (start.y < 0)) {
			start.x = sq_mile->map_grid.x * CPM + start_local.x;
			start.y = sq_mile->map_grid.y * CPM + start_local.y;
		}
		if ((finish.x < 0) && (finish.y < 0)) {
			finish.x = sq_mile->map_grid.x * CPM + finish_local.x;
			finish.y = sq_mile->map_grid.y * CPM + finish_local.y;
		}
		assert((start.x >= 0) && (start.y >= 0) &&
			   (finish.x >= 0) && (finish.y >= 0));

		/* Pick chunks to add river to for ungenerated diagonals */
		if ((start_adj.x < 0) && (start_dir % 2) && (start_dir != DIR_NONE)) {
			bool clockwise = one_in_(2);
			switch (start_dir) {
				case DIR_NE: {
					start_adj = clockwise ?
						loc(start.x + 1, start.y) : loc(start.x, start.y - 1);
					break;
				}
				case DIR_SE: {
					start_adj = clockwise ?
						loc(start.x, start.y + 1) : loc(start.x + 1, start.y);
					break;
				}
				case DIR_SW: {
					start_adj = clockwise ?
						loc(start.x - 1, start.y) : loc(start.x, start.y + 1);
					break;
				}
				case DIR_NW: {
					start_adj = clockwise ?
						loc(start.x, start.y - 1) : loc(start.x - 1, start.y);
					break;
				}
				default: {
				}
			}
		}
		if ((finish_adj.x < 0) && (finish_dir % 2) && (finish_dir != DIR_NONE)){
			bool clockwise = one_in_(2);
			switch (finish_dir) {
				case DIR_NE: {
					finish_adj = clockwise ? loc(finish.x + 1, finish.y)
						: loc(finish.x, finish.y - 1);
					break;
				}
				case DIR_SE: {
					finish_adj = clockwise ? loc(finish.x, finish.y + 1)
						: loc(finish.x + 1, finish.y);
					break;
				}
				case DIR_SW: {
					finish_adj = clockwise ? loc(finish.x - 1, finish.y)
						: loc(finish.x, finish.y + 1);
					break;
				}
				case DIR_NW: {
					finish_adj = clockwise ? loc(finish.x, finish.y - 1)
						: loc(finish.x - 1, finish.y);
					break;
				}
				default: {
				}
			}
		}

		/* Write the pieces of river */
		write_river_pieces(sq_mile, r_mile, start_dir, start, start_adj,
						   finish_dir, finish, finish_adj, course, num);

		for (y = 0; y < CPM; y++) {
			mem_free(course[y]);
		}
		mem_free(course);
	}

	/* Also check adjacent square miles for "corner rivers"? */

	/* Mark as mapped */
	sq_mile->mapped = true;
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

//TODO RIVER Add current
static void make_river_piece(struct chunk *c, struct loc top_left,
							 struct river_piece *piece)
{
	int y, x;

	/* Place the grids */
	struct river_grid *rgrid = piece->grids;
	while (rgrid) {
		square_set_feat(c, loc_sum(top_left, rgrid->grid), FEAT_S_WATER);
		rgrid = rgrid->next;
	}

	/* Set deep water */
	for (y = 0; y < CHUNK_SIDE; y++) {
		for (x = 0; x < CHUNK_SIDE; x++) {
			struct loc grid = loc_sum(top_left, loc(x, y));
			if (!square_iswater(c, grid)) continue;

			/* Surrounded by all or all but one grid means deep */
			//TODO RIVER This needs to be adjusted for chunk edges
			if (count_neighbors(NULL, c, grid, square_iswater, false) >
				count_neighbors(NULL, c, grid, square_in_bounds, false) - 2) {
				square_set_feat(c, grid, FEAT_D_WATER);
			}
		}
	}
}

void surface_gen(struct chunk *c, struct chunk_ref *ref, int y_coord,
				 int x_coord, struct connector *first_conn)
{
	struct loc top_left = loc(x_coord * CHUNK_SIDE, y_coord * CHUNK_SIDE);
	struct point_set *chunk = make_chunk_point_set(c, top_left);
	struct world_region *region = &region_info[find_region(ref->y_pos,
														   ref->x_pos)];
	enum biome_type standard, mon_biome;
	struct biome_tweak tweak = { DIR_NONE, 0, -1, DIR_NONE, 0, -1 };
	struct gen_loc *location;
	int lower, upper, i;
	bool found;

	/* Get the standard biome based on region.txt */
	standard = square_miles[ref->y_pos / CPM][ref->x_pos / CPM].biome;
	tweak.biome1 = standard;
	tweak.biome2 = standard;
	mon_biome = standard;

	/* Check for tweaks, and generate accordingly */
	if (get_biome_tweaks(ref->y_pos, ref->x_pos, &tweak)) {
		if (tweak.dir1 == DIR_NONE) {
			/* Whole chunk is the tweaked biome */
			make_piece(c, tweak.biome1, chunk);
			mon_biome = tweak.biome1;
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

	/* Place river if any */
	found = gen_loc_find(ref->x_pos, ref->y_pos, 0, &lower, &upper);
	assert(found);
	location = &gen_loc_list[upper];
	if (location->river_piece) {
		make_river_piece(c, top_left, location->river_piece);
	}

	//TODO generate monsters, perhaps objects
	for (i = randint1(2); i > 0; i--) {
		pick_and_place_distant_monster(c, player, mon_biome, true,
									   region->danger);
	}
}

