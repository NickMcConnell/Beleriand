/**
 * \file gen-river.c
 * \brief River generation
 *
 * Code for creation of the rivers of Beleriand.
 *
 * Copyright (c) 2025
 * Nick McConnell
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
#include "project.h"


/**
 * Map a slightly wandering course from one grid to another.
 *
 * \param start the starting grid
 * \param finish the finishing grid
 * \param course a square array with all entries zero
 * \param side the dimensions of the array
 */
static int map_point_to_point(struct loc start, struct loc finish,
							  uint16_t **course, int side)
{
	struct loc grid = start;
	enum direction dir = DIR_NONE;
	int count = 0;

	/* Boundary check */
	assert((start.x >= 0) && (start.x < side) && (finish.x >= 0) &&
		   (finish.x < side));

	/* Mark the start point */
	course[grid.y][grid.x] = ++count;

	/* Add points roughly in the right direction until we're there */
	while (!loc_eq(grid, finish)) {
		bool must_adjust;
		dir = rough_direction(grid, finish);

		/* Already at the finish, don't adjust, just do it */
		if (loc_eq(loc_sum(grid, ddgrid[dir]), finish)) {
			course[finish.y][finish.x] = ++count;
			break;
		}

		/* If the obvious grid is already used, adjust  */
		must_adjust = (course[grid.y + ddy[dir]][grid.x + ddx[dir]] != 0);

		/* Smallish chance of deviating, none if on the edge */
		if ((one_in_(6) || must_adjust) &&
			(grid.x > 0) && (grid.x < side - 1) &&
			(grid.y > 0) && (grid.y < side - 1)) {
			enum direction new_dir = DIR_NONE;
			if (one_in_(2)) {
				new_dir = cycle[chome[dir] + 1];
				/* Didn't work, try the other one */
				if (course[grid.y + ddy[new_dir]][grid.x + ddx[new_dir]] != 0) {
					new_dir = cycle[chome[dir] - 1];
				}
			} else {
				new_dir = cycle[chome[dir] - 1];
				/* Didn't work, try the other one */
				if (course[grid.y + ddy[new_dir]][grid.x + ddx[new_dir]] != 0) {
					new_dir = cycle[chome[dir] + 1];
				}
			}
			if (course[grid.y + ddy[new_dir]][grid.x + ddx[new_dir]] == 0) {
				dir = new_dir;
			} else if (must_adjust) {
				/* Failure */
				assert(0);
			}
		}

		/* If the direction is diagonal, make two cardinal moves */
		if (dir % 2) {
			/* Check cardinals to see if they're used yet */
			struct loc grid_clock = next_grid(grid, cycle[chome[dir] - 1]);
			struct loc grid_anti = next_grid(grid, cycle[chome[dir] + 1]);
			if (course[grid_anti.y][grid_anti.x]) {
				/* Anti-clockwise is used, clockwise first */
				grid = grid_clock;
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
				grid = next_grid(grid, cycle[chome[dir] + 1]);
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
			} else if (course[grid_clock.y][grid_clock.x]) {
				/* Clockwise is used, anti-clockwise first */
				grid = grid_anti;
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
				grid = next_grid(grid, cycle[chome[dir] - 1]);
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
			} else if (one_in_(2)) {
				/* Randomly clockwise first */
				grid = next_grid(grid, cycle[chome[dir] - 1]);
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
				grid = next_grid(grid, cycle[chome[dir] + 1]);
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
			} else {
				/* Randomly anti-clockwise first */
				grid = next_grid(grid, cycle[chome[dir] + 1]);
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
				grid = next_grid(grid, cycle[chome[dir] - 1]);
				assert(course[grid.y][grid.x] == 0);
				course[grid.y][grid.x] = ++count;
			}
		} else {
			/* Cardinal direction, single move */
			grid = next_grid(grid, dir);
			assert(course[grid.y][grid.x] == 0);
			course[grid.y][grid.x] = ++count;
		}
	}

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
			if (stretch) {
				return stretch->miles;
			}
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
									  struct loc *finish_adj, bool begin,
									  bool end)
{
	enum direction dir;

	/* Start */
	if (begin) {
		/* This river piece starts in this square mile */
	} else if (start_dir % 2 == 0) {
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
	if (end) {
		/* This river piece terminates in this square mile */
	} else if (finish_dir % 2 == 0) {
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
	int num = 0;

	/* Choose a start point where necessary */
	if (start->x < 0) {
		/* Pick a random point along the border (not needed for diagonals) */
		int start_point = randint0(side);

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
	}

	/* Choose a finish point where necessary */
	if (finish->x < 0) {
		/* Pick a random point along the border (not needed for diagonals) */
		int finish_point = randint0(side);

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
	}

	/* Do the actual course */
	num = map_point_to_point(*start, *finish, course, side);

	return num;
}

/**
 * Get the horizontal direction from a grid to another grid given
 * their local coordinates in an array of squares of side x side grids.
 *
 * \param start is the first grid
 * \param finish is the second grid
 * \param side is the maximum coordinate within a square of grids
 */
static int grid_direction(struct loc finish, struct loc start, int side)
{
	enum direction dir;
	struct loc offset = loc_diff(finish, start);
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
 * Check that a grid lies in a square of grids of a given side length
 * (noting that this is not the same usage of "square" as in struct square...)
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
	if (min <= max) {
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
	struct loc entry_grid = loc(-1, -1), exit_grid = loc(-1, -1);
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
		in_dir = grid_direction(start_adj, start, CHUNK_SIDE);
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

			/* Set entry_grid for initial chunk */
			if (out_dir == DIR_N) {
				entry_grid = loc(finish_point, CHUNK_SIDE - 1);
			} else if (out_dir == DIR_E) {
				entry_grid = loc(0, finish_point);
			} else if (out_dir == DIR_S) {
				entry_grid = loc(finish_point, 0);
			} else {
				entry_grid = loc(CHUNK_SIDE - 1, finish_point);
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
	} else if (start_dir == DIR_NONE) {
		in_grid = loc(randint0(CHUNK_SIDE / 2) + randint0(CHUNK_SIDE / 2 + 1),
					  randint0(CHUNK_SIDE / 2) + randint0(CHUNK_SIDE / 2 + 1));
	} else {
		in_dir = start_dir;
	}

	/* Set direction for any outgoing river to a set external chunk. */
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
		enum direction out_dir1 = DIR_NONE;
		int lower, upper;
		bool reload;
		struct gen_loc *location;

		/* Allocate in-chunk course array */
		uint16_t **course1 = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t*));
		for (y = 0; y < CHUNK_SIDE; y++) {
			course1[y] = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t));
		}

		/* Get entry direction */
		if (k > 1) {
			in_dir = grid_direction(prev_chunk, current_chunk, CPM);
		} else {
			in_grid = entry_grid;
		}

		/* Get exit direction */
		if (k < num) {
			out_dir1 = grid_direction(next_chunk, current_chunk, CPM);
			out_grid = loc(-1, -1);
		} else if (finish_dir == DIR_NONE) {
			out_grid = loc(randint0(CHUNK_SIDE / 2) +
						   randint0(CHUNK_SIDE / 2 + 1),
						   randint0(CHUNK_SIDE / 2) +
						   randint0(CHUNK_SIDE / 2 + 1));
		} else {
			out_grid = exit_grid;
		}

		/* Map a course across the chunk */
		(void) map_course(CHUNK_SIDE, in_dir, &in_grid, out_dir1, &out_grid,
						  course1);

		/* Write new in_grid adjacent to out_grid in out_dir1 */
		in_grid = loc_sum(out_grid, ddgrid[out_dir1]);
		in_grid.x = (in_grid.x + CHUNK_SIDE) % CHUNK_SIDE;
		in_grid.y = (in_grid.y + CHUNK_SIDE) % CHUNK_SIDE;

		/* Widen */
		widen_river_course(CHUNK_SIDE, course1, widen_dir, width);

		/* Get the location, confirming it hasn't been written before */
		reload = gen_loc_find(current_chunk.x, current_chunk.y, 0, &lower,
							  &upper);
		if (!reload) {
			gen_loc_make(current_chunk.x, current_chunk.y, 0, upper);
			location = &gen_loc_list[upper];
			location->river_piece = mem_zalloc(sizeof(struct river_piece));

			/* Write the river piece */
			write_river_piece(course1, location);
		}

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
	struct loc join = loc(-1, -1);
	int y;

	/* Already mapped */
	if (sq_mile->mapped) return;

	/* Check each river mile that passes through (two maximum) */
	for (r_mile = sq_mile->river_miles; r_mile; r_mile = r_mile->next) {
		/* Starting and finishing directions for the course */
		enum direction start_dir = DIR_NONE, finish_dir = DIR_NONE;

		/* Start and finish locations (in global chunk coordinates) */
		struct loc start = loc(-1, -1), finish = loc(-1, -1);

		/* Adjacent chunks to start and finish outside this square mile */
		struct loc start_adj = loc(-1, -1), finish_adj = loc(-1, -1);

		/* Coordinates of start and finish in the square mile (CPMxCPM) */
		struct loc start_local = loc(-1, -1), finish_local = loc(-1, -1);

		/* Rough centre in case it's needed for start and stop purposes */
		struct loc centre = loc(randint0(CPM / 2) + randint0(CPM / 2 + 1),
								randint0(CPM / 2) + randint0(CPM / 2 + 1));

		/* Adjacent river miles upstream and downstream */
		struct river_mile *upstream = next_river_mile(r_mile, true, two_up),
			*downstream = next_river_mile(r_mile, false, two_down);

		/* Does this piece begin here? */
		bool begin = (r_mile->part == RIVER_SOURCE) ||
			(r_mile->part == RIVER_EMERGE);

		/* Does this piece end here? */
		bool end = (r_mile->part == RIVER_JOIN) ||
			(r_mile->part == RIVER_UNDERGROUND) ||
			(r_mile->part == RIVER_LAKE) || (r_mile->part == RIVER_SEA);

		int num = 0;

		/* Allocate course array */
		uint16_t **course = mem_zalloc(CPM * sizeof(uint16_t*));
		for (y = 0; y < CPM; y++) {
			course[y] = mem_zalloc(CPM * sizeof(uint16_t));
		}

		/* Find the incoming and outgoing directions if any */
		if (upstream) {
			start_dir = grid_direction(upstream->sq_mile->map_grid,
									   sq_mile->map_grid, MPS);
			two_up = true;
		}
		if (downstream) {
			finish_dir = grid_direction(downstream->sq_mile->map_grid,
										sq_mile->map_grid, MPS);
			two_down = true;
		}

		/* Set starting and finishing points to match any external river */
		square_mile_river_borders(sq_mile, start_dir, &start, &start_adj,
								  finish_dir, &finish, &finish_adj, begin, end);

		/* Set local-to-square-mile coordinates for start and finish points
		 * if they are set */
		if ((start.x >= 0) && (start.y >= 0)) {
			start_local = loc(start.x % CPM, start.y % CPM);
		}
		if ((finish.x >= 0) && (finish.y >= 0)) {
			finish_local = loc(finish.x % CPM, finish.y % CPM);
		}

		/* Set starts and finshes according to what part of the river we have */
		if (r_mile->part == RIVER_SOURCE) {
			/* Place source if needed */
			assert(downstream && !upstream);
			start_local = centre;
		} else if (r_mile->part == RIVER_EMERGE) {
			/* Emerging from underground */
			assert(downstream && upstream);
			start_local = centre; //TODO RIVER do underground pieces
		} else if (r_mile->part == RIVER_UNDERGROUND) {
			/* Send underground if needed */
			assert(downstream && upstream);
			finish_local = centre; //TODO RIVER do underground pieces
		} else if (r_mile->part == RIVER_JOIN) {
			/* Set the course to finish at the joining point */
			assert(upstream && !downstream);
			assert((join.x != -1) && (join.y != -1));
			assert(loc_eq(finish_local, loc(-1, -1)));
			finish_local = join;
		} else if ((r_mile->part == RIVER_LAKE) || (r_mile->part == RIVER_SEA)){
			/* Rivers entering lakes/sea should be able just to run to the
			 * opposite side of the river mile */
			assert(upstream && !downstream);
			finish_dir = opposite_dir(start_dir);
		} else {
			/* Just a continuation */
			assert(upstream && downstream);
		}

		/* Map the chunks the river crosses */
		num = map_course(CPM, start_dir, &start_local, finish_dir,
						 &finish_local, course);

		/* Update start and finish chunks */
		assert(grid_in_square(CPM, start_local) &&
			   grid_in_square(CPM, finish_local));
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

		/* Set a joining point if necessary */
		if (r_mile->next && (r_mile->next->part == RIVER_JOIN)) {
			/* Get a random point to join, biased toward the middle */
			int index = randint1(num / 2) + randint1(num / 2);
			join = find_course_index(CPM, index, course);
			assert((join.x != -1) && (join.y != -1));
		}

		/* Free course */
		for (y = 0; y < CPM; y++) {
			mem_free(course[y]);
		}
		mem_free(course);
	}

	/* Mark as mapped */
	sq_mile->mapped = true;
}


