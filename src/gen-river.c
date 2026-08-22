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
 * Check that a grid lies in a square of grids of a given side length
 * (noting that this is not the same usage of "square" as in struct square...)
 */
static bool grid_in_square(int side, struct loc grid)
{
	return ((grid.x >= 0) && (grid.x < side) && (grid.y >= 0) &&
			(grid.y < side));
}

/**
 * Map a slightly wandering course from one grid to another.
 *
 * \param start the starting grid
 * \param finish the finishing grid
 * \param course a square array with all entries zero
 * \param side the dimensions of the array
 */
static int map_point_to_point(struct loc start, struct loc finish,
							  uint16_t **course, int side, struct loc tl,
							  int count)
{
	struct loc grid = start;
	enum direction dir = DIR_NONE;

	/* Boundary check */
	assert(grid_in_square(side, loc_diff(start, tl)) &&
		   grid_in_square(side, loc_diff(finish, tl)));

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
		//TODO currently really no chance of deviating, because allowing it
		//was leading to the path getting trapped against the edge
		if ((one_in_(60000000) || must_adjust) &&
			(grid.x > tl.x) && (grid.x < tl.x + side - 1) &&
			(grid.y > tl.y) && (grid.y < tl.y + side - 1)) {
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
static void find_river_chunk(struct square_mile *sq_mile,
							 struct loc *int_chunk, enum direction dir)
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
					/* Adjacent chunk in the current square mile */
					use_y = (dir == DIR_N) ? tl_y : tl_y + CPM - 1;
					*int_chunk = loc(tl_x + i, use_y);

					/* Set internal chunk if it exists */
					if (gen_loc_find(tl_x + i, use_y, 0, &lower, &upper)) {
						/* Don't need to check for a river piece, as the square
						 * mile hasn't been mapped, so the only way this
						 * location could exist is from writing river */
						return;
					} else {
						*int_chunk = loc(-1, -1);
					}
				}
			}
		} else {
			/* Right edge of mile left, or left edge of mile right */
			int use_x = (dir == DIR_W) ? tl_x - 1 : tl_x + CPM;
			bool found = gen_loc_find(use_x, tl_y + i, 0, &lower, &upper);
			if (found) {
				struct gen_loc location = gen_loc_list[upper];
				if (location.river_piece) {
					/* Adjacent chunk in the current square mile */
					use_x = (dir == DIR_W) ? tl_x : tl_x + CPM - 1;
					*int_chunk = loc(use_x, tl_y + i);

					/* Verify internal chunk exists */
					if (gen_loc_find(use_x, tl_y + i, 0, &lower, &upper)) {
						/* Don't need to check for a river piece, as the square
						 * mile hasn't been mapped, so the only way this
						 * location could exist is from writing river */
						return;
					} else {
						*int_chunk = loc(-1, -1);
					}
				}
			}
		}
		if ((int_chunk->x >= 0) && (int_chunk->y >= 0)) break;
	}
}

/**
 * Find any adjacent chunks to this square mile with river edges already set
 */
static void square_mile_river_borders(struct square_mile *sq_mile,
									  enum direction start_dir,
									  struct loc *start,
									  enum direction finish_dir,
									  struct loc *finish,
									  bool begin, bool end)
{
	enum direction dir;

	/* Start */
	if (begin) {
		/* This river piece starts in this square mile */
	} else if (start_dir % 2 == 0) {
		/* Cardinal direction, simple check */
		find_river_chunk(sq_mile, start, start_dir);
	} else {
		/* Diagonal, check cardinal direction anti-clockwise */
		dir = cycle[chome[start_dir] + 1];
		find_river_chunk(sq_mile, start, dir);

		/* Check clockwise if necessary, note that only one should occur */
		if (start->x < 0) {
			dir = cycle[chome[start_dir] - 1];
			find_river_chunk(sq_mile, start, dir);
		}
	}

	/* Finish */
	if (end) {
		/* This river piece terminates in this square mile */
	} else if (finish_dir % 2 == 0) {
		/* Cardinal direction, simple check */
		find_river_chunk(sq_mile, finish, finish_dir);
	} else {
		/* Diagonal, check cardinal direction anti-clockwise */
		dir = cycle[chome[finish_dir] + 1];
		find_river_chunk(sq_mile, finish, dir);

		/* Check clockwise if necessary, note that only one should occur */
		if (finish->x < 0) {
			dir = cycle[chome[finish_dir] - 1];
			find_river_chunk(sq_mile, finish, dir);
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
					  uint16_t **course, bool inner)
{
	int num = 0;
	int side_use = inner ? side - 2 : side;
	struct loc tl = inner ? loc(1, 1) : loc(0, 0);
	bool add_finish = false;
	int finish_point = inner ? randint1(side_use) : randint0(side_use);

	/* Choose a start point where necessary */
	if (start->x < 0) {
		/* Pick a random point along the border (not needed for diagonals) */
		int start_point = randint1(side_use);
		bool clockwise = one_in_(2);

		/* Start is only not given if we're called from map_river_miles() */
		assert(inner);

		/* Record start */
		switch (start_dir) {
			case DIR_N: {
				course[0][start_point] = ++num;
				*start = loc(start_point, 1);
				break;
			}
			case DIR_NE: {
				course[0][side - 1] = ++num;
				if (clockwise) {
					course[1][side - 1] = ++num;
				} else {
					course[0][side - 2] = ++num;
				}
				*start = loc(side_use, 1);
				break;
			}
			case DIR_E: {
				course[start_point][side - 1] = ++num;
				*start = loc(side_use, start_point);
				break;
			}
			case DIR_SE: {
				course[side - 1][side - 1] = ++num;
				if (clockwise) {
					course[side - 1][side - 2] = ++num;
				} else {
					course[side - 2][side - 1] = ++num;
				}
				*start = loc(side_use, side_use);
				break;
			}
			case DIR_S: {
				course[side - 1][start_point] = ++num;
				*start = loc(start_point, side_use);
				break;
			}
			case DIR_SW: {
				course[side - 1][0] = ++num;
				if (clockwise) {
					course[side - 2][0] = ++num;
				} else {
					course[side - 1][1] = ++num;
				}
				*start = loc(1, side_use);
				break;
			}
			case DIR_W: {
				course[start_point][0] = ++num;
				*start = loc(1, start_point);
				break;
			}
			case DIR_NW: {
				course[0][0] = ++num;
				if (clockwise) {
					course[0][1] = ++num;
				} else {
					course[1][0] = ++num;
				}
				*start = loc(1, 1);
				break;
			}
			default:quit_fmt("No start in map_course().");
		}
	}

	/* Choose a finish point where necessary */
	if (finish->x < 0) {
		/* Pick a random point along the border (not needed for diagonals) */
		int smallest = inner ? 1 : 0;
		int largest = inner ? side - 2 : side - 1;

		/* Record finish */
		switch (finish_dir) {
			case DIR_N: *finish = loc(finish_point, smallest); break;
			case DIR_NE: *finish = loc(largest, smallest); break;
			case DIR_E: *finish = loc(largest, finish_point); break;
			case DIR_SE: *finish = loc(largest, largest); break;
			case DIR_S: *finish = loc(finish_point, largest); break;
			case DIR_SW: *finish = loc(smallest, largest); break;
			case DIR_W: *finish = loc(smallest, finish_point); break;
			case DIR_NW: *finish = loc(smallest, smallest); break;
			default:quit_fmt("No finish in map_course().");
		}

		/* Note that we need to add points outside the inner square */
		add_finish = inner;
	}

	/* Do the actual course */
	num = map_point_to_point(*start, *finish, course, side_use, tl, num);

	/* Adjust the end */
	if (add_finish) {
		bool clockwise = one_in_(2);

		/* Finish only gets added if we're called from map_river_miles() */
		assert(inner);

		/* Record start */
		switch (finish_dir) {
			case DIR_N: {
				course[0][finish_point] = ++num;
				*finish = loc(finish_point, 0);
				break;
			}
			case DIR_NE: {
				if (clockwise) {
					course[0][side - 2] = ++num;
				} else {
					course[1][side - 1] = ++num;
				}
				course[0][side - 1] = ++num;
				*finish = loc(side - 1, 0);
				break;
			}
			case DIR_E: {
				course[finish_point][side - 1] = ++num;
				*finish = loc(side - 1, finish_point);
				break;
			}
			case DIR_SE: {
				if (clockwise) {
					course[side - 2][side - 1] = ++num;
				} else {
					course[side - 1][side - 2] = ++num;
				}
				course[side - 1][side - 1] = ++num;
				*finish = loc(side - 1, side - 1);
				break;
			}
			case DIR_S: {
				course[side - 1][finish_point] = ++num;
				*finish = loc(finish_point, side - 1);
				break;
			}
			case DIR_SW: {
				if (clockwise) {
					course[side - 1][1] = ++num;
				} else {
					course[side - 2][0] = ++num;
				}
				course[side - 1][0] = ++num;
				*finish = loc(0, side - 1);
				break;
			}
			case DIR_W: {
				course[finish_point][0] = ++num;
				*finish = loc(0, finish_point);
				break;
			}
			case DIR_NW: {
				if (clockwise) {
					course[1][0] = ++num;
				} else {
					course[0][1] = ++num;
				}
				course[0][0] = ++num;
				*finish = loc(0, 0);
				break;
			}
			default: ;
		}
	}

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
static struct gen_loc *find_chunk_with_river(struct loc grid)
{
	int lower, upper;
	bool found;
	if ((grid.y < 0) || (grid.y >= CPM * MAX_Y_REGION - 1) ||
		(grid.x < 0) || (grid.x >= CPM * MAX_X_REGION - 1)) return NULL;
	found = gen_loc_find(grid.x, grid.y, 0, &lower, &upper);
	if (found && gen_loc_list[upper].river_piece) return &gen_loc_list[upper];
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

/**
 *
 */
static void find_half_piece_start(struct gen_loc *chunk, enum direction dir,
								  struct loc *start)
{
	enum direction in_dir = dir;
	int loc_x = chunk->x_pos, loc_y = chunk->y_pos;
	int lower, upper;
	bool found;
	struct river_piece *river_piece = chunk->river_piece;
	struct river_grid *rgrid;

	/* Find the direction of the previous river chunk */
	if (in_dir % 2) {
		/* Find the cardinal direction if diagonal */
		switch (in_dir) {
			case DIR_NE: {
				found = gen_loc_find(loc_x, loc_y - 1, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_N;
					break;
				}
				found = gen_loc_find(loc_x + 1, loc_y, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_E;
					break;
				}
				quit_fmt("Incoming river direction failure");
				break;
			}
			case DIR_SE: {
				found = gen_loc_find(loc_x, loc_y + 1, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_S;
					break;
				}
				found = gen_loc_find(loc_x + 1, loc_y, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_E;
					break;
				}
				quit_fmt("Incoming river direction failure");
				break;
			}
			case DIR_SW: {
				found = gen_loc_find(loc_x, loc_y + 1, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_S;
					break;
				}
				found = gen_loc_find(loc_x - 1, loc_y, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_W;
					break;
				}
				quit_fmt("Incoming river direction failure");
				break;
			}
			case DIR_NW: {
				found = gen_loc_find(loc_x, loc_y - 1, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_N;
					break;
				}
				found = gen_loc_find(loc_x - 1, loc_y, 0, &lower, &upper);
				if (found && gen_loc_list[upper].river_piece) {
					in_dir = DIR_W;
					break;
				}
				quit_fmt("Incoming river direction failure");
				break;
			}
			default: quit_fmt("Incoming river direction failure");
		}
	}

	/* Find the greatest extent of the existing river piece */
	switch (in_dir) {
		case DIR_N: {
			int max = 0, ave = 0, num = 0;
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.y > max) {
					max = rgrid->grid.y;
				}
			}
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.y == max) {
					ave += rgrid->grid.x;
					num++;
				}
			}
			ave /= num;
			*start = loc(ave, max);
			break;
		}
		case DIR_E: {
			int min = CHUNK_SIDE, ave = 0, num = 0;
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.x < min) {
					min = rgrid->grid.x;
				}
			}
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.x == min) {
					ave += rgrid->grid.y;
					num++;
				}
			}
			ave /= num;
			*start = loc(min, ave);
			break;
		}
		case DIR_S: {
			int min = CHUNK_SIDE, ave = 0, num = 0;
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.y < min) {
					min = rgrid->grid.y;
				}
			}
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.y == min) {
					ave += rgrid->grid.x;
					num++;
				}
			}
			ave /= num;
			*start = loc(ave, min);
			break;
		}
		case DIR_W: {
			int max = 0, ave = 0, num = 0;
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.x > max) {
					max = rgrid->grid.x;
				}
			}
			for (rgrid = river_piece->grids; rgrid; rgrid = rgrid->next) {
				if (rgrid->grid.x == max) {
					ave += rgrid->grid.y;
					num++;
				}
			}
			ave /= num;
			*start = loc(max, ave);
			break;
		}
		default: quit_fmt("Incoming river direction failure");
	}
}

/**
 * Map out the course of a river through a square mile.
 *
 * This function takes the course of a river mile spelled out in chunks across
 * the square mile, fills in courses across each of the chunks, widens, and
 * then writes the river pieces in each chunk.
 */
static void map_one_river_mile(struct square_mile *sq_mile,
							   struct river_mile *r_mile,
							   enum direction start_dir, struct loc start,
							   enum direction finish_dir, struct loc finish,
							   uint16_t **coarse_course, int num)
{
	/* Coordinates of the chunk in the top left corner */
	struct loc tl = loc(sq_mile->map_grid.x * CPM - 1,
						sq_mile->map_grid.y * CPM - 1);

	int side = CPM + 2;
	struct loc prev_chunk = loc(-1, -1);
	struct loc current_chunk = find_course_index(side, 1, coarse_course);
	int y, x;
	int coarse_index = 1, count = 0;
	enum direction in_dir = DIR_NONE, out_dir = DIR_NONE, widen_dir = DIR_NONE;
	struct loc grid = loc(-1, -1);

	/* Get river width */
	int width = get_river_width(r_mile);

	/* Check the chunks at the start and finish of river */
	struct gen_loc *chunk_s = find_chunk_with_river(start);
	struct gen_loc *chunk_f = find_chunk_with_river(finish);

	/* Allocate course array */
	uint16_t **course = mem_zalloc(side * CHUNK_SIDE * sizeof(uint16_t*));
	for (y = 0; y < side * CHUNK_SIDE; y++) {
		course[y] = mem_zalloc(side * CHUNK_SIDE * sizeof(uint16_t));
	}

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

	/* Do the first chunk, connecting start if there is already river there */
	if (chunk_s) {
		/* Find where we continue from the previous river chunk */
		find_half_piece_start(chunk_s, start_dir, &grid);
	} else {
		grid = loc(randint0(CHUNK_SIDE / 2) + randint0(CHUNK_SIDE / 2 + 1),
				   randint0(CHUNK_SIDE / 2) + randint0(CHUNK_SIDE / 2 + 1));
	}

	/* Progress along the square mile course, writing river in every chunk */
	for (coarse_index = 1; coarse_index <= num; coarse_index++) {
        struct loc next_chunk = (coarse_index < num) ?
			find_course_index(side, coarse_index + 1, coarse_course)	:
			loc(-1, -1);
		struct loc in_grid, out_grid;
		int i, local_count = 0;

        /* Allocate in-chunk course array */
        uint16_t **course1 = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t*));
        for (y = 0; y < CHUNK_SIDE; y++) {
			course1[y] = mem_zalloc(CHUNK_SIDE * sizeof(uint16_t));
        }

		/* Get entry direction */
		if (coarse_index > 1) {
			in_dir = grid_direction(prev_chunk, current_chunk, CPM);
		} else {
			in_grid = grid;
		}

		/* Get exit direction */
		if (coarse_index < num) {
			out_dir = grid_direction(next_chunk, current_chunk, CPM);
			out_grid = loc(-1, -1);
		} else if (chunk_f) {
			/* Find where we join the already set river chunk */
			find_half_piece_start(chunk_f, finish_dir, &out_grid);
		} else {
			out_grid = loc(randint0(CHUNK_SIDE / 2) +
						   randint0(CHUNK_SIDE / 2 + 1),
						   randint0(CHUNK_SIDE / 2) +
						   randint0(CHUNK_SIDE / 2 + 1));
		}

		/* Map a course across the chunk */
		local_count =  map_course(CHUNK_SIDE, in_dir, &in_grid, out_dir,
								  &out_grid, course1, false);

		/* Write new in_grid adjacent to out_grid in out_dir */
		in_grid = loc_sum(out_grid, ddgrid[out_dir]);
		in_grid.x = (in_grid.x + CHUNK_SIDE) % CHUNK_SIDE;
		in_grid.y = (in_grid.y + CHUNK_SIDE) % CHUNK_SIDE;

		/* Get the top left corner of this chunk in the course array */
		grid = loc(current_chunk.x * CHUNK_SIDE, current_chunk.y * CHUNK_SIDE);

		/* Write course1 into course */
		for (i = 1; i <= local_count; i++) {
			/* Find the next grid in course1 local coordinates */
			struct loc local_grid = find_course_index(CHUNK_SIDE, i, course1);

			/* Translate into course local coordinates */
			course[local_grid.y + grid.y][local_grid.x + grid.x] = ++count;
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

	/* Widen the river mile */
	(void) widen_river_course(side * CHUNK_SIDE, course, widen_dir, width);

	/* Write the river pieces */
	for (y = 0; y < side; y++) {
		for (x = 0; x < side; x++) {
			int y1, x1;
			int lower, upper;
			bool found = false;;
			struct gen_loc *location;
			int count = 0;

			/* Check for river grids */
			for (y1 = 0; y1 < CHUNK_SIDE; y1++) {
				for (x1 = 0; x1 < CHUNK_SIDE; x1++) {
					if (course[y * CHUNK_SIDE + y1][x * CHUNK_SIDE + x1]) {
						found = true;
						break;
					}
				}
				if (found) break;
			}
			if (!found) continue;

			/* Get the location, make if (as is probable) it doesn't exist */
			found = gen_loc_find(tl.x + x, tl.y + y, 0, &lower, &upper);
			if (!found) {
				gen_loc_make(tl.x + x, tl.y + y, 0, upper);
			}
			location = &gen_loc_list[upper];

			/* Make the river piece if needed */
			if (!location->river_piece) {
				location->river_piece =	mem_zalloc(sizeof(struct river_piece));
			}

			/* Check for river grids */
			for (y1 = 0; y1 < CHUNK_SIDE; y1++) {
				for (x1 = 0; x1 < CHUNK_SIDE; x1++) {
					if (course[y * CHUNK_SIDE + y1][x * CHUNK_SIDE + x1]) {
						struct river_grid *rgrid = mem_zalloc(sizeof(*rgrid));
						rgrid->next = location->river_piece->grids;
						rgrid->grid = loc(x1, y1);
						location->river_piece->grids = rgrid;
						count++;
					}
				}
			}
			location->river_piece->num_grids = count;
		}
	}

	/* Free the course array */
	for (y = 0; y < CHUNK_SIDE; y++) {
		mem_free(course[y]);
	}
	mem_free(course);
}

/**
 * Map out the course of rivers through a square mile.
 *
 * This function is called on the player first entering a square mile, and it
 * writes river pieces into all the locations that it deems any river to pass
 * through, creating these locations first.
 */
void map_river_miles(struct square_mile *sq_mile)
{
	struct river_mile *r_mile;
	bool two_up = false;
	bool two_down = false;
	struct loc join = loc(-1, -1);
	int y;
	int side = CPM + 2;

	/* Already mapped */
	if (sq_mile->mapped) return;

	/* Check each river mile that passes through (two maximum) */
	for (r_mile = sq_mile->river_miles; r_mile; r_mile = r_mile->next) {
		/* Starting and finishing directions for the course */
		enum direction start_dir = DIR_NONE, finish_dir = DIR_NONE;

		/* Start and finish locations (in global chunk coordinates) */
		struct loc start = loc(-1, -1), finish = loc(-1, -1);

		/* Coordinate of start and finish in the square mile,
		 * augmented by a line of extra chunks all around */
		struct loc start_local = loc(-1, -1), finish_local = loc(-1, -1);

		/* Coordinates of top left grid in the square mile surrounded by
		 * extra chunks (side x side) */
		struct loc top_left = loc(sq_mile->map_grid.x * CPM - 1,
								  sq_mile->map_grid.y * CPM - 1);

		/* Rough centre in case it's needed for start and stop purposes */
		struct loc centre = loc(randint0(side / 2) + randint0(side / 2 + 1),
								randint0(side / 2) + randint0(side / 2 + 1));

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
		uint16_t **course = mem_zalloc(side * sizeof(uint16_t*));
		for (y = 0; y < side; y++) {
			course[y] = mem_zalloc(side * sizeof(uint16_t));
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
		square_mile_river_borders(sq_mile, start_dir, &start, finish_dir,
								  &finish, begin, end);

		/* Set local-to-square-mile coordinates for start and finish points
		 * if they are set */
		if ((start.x >= 0) && (start.y >= 0)) {
			start_local = loc_diff(start, top_left);
			assert(grid_in_square(side, start_local));
		}
		if ((finish.x >= 0) && (finish.y >= 0)) {
			finish_local = loc_diff(finish, top_left);
			assert(grid_in_square(side, finish_local));
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
		num = map_course(side, start_dir, &start_local, finish_dir,
						 &finish_local, course, true);

		/* Update start and finish chunks */
		assert(grid_in_square(side, start_local) &&
			   grid_in_square(side, finish_local));
		if ((start.x < 0) && (start.y < 0)) {
			start = loc_sum(top_left, start_local);
		}
		if ((finish.x < 0) && (finish.y < 0)) {
			finish = loc_sum(top_left, finish_local);
		}

		/* Map the river through the mile, writing the river pieces */
		map_one_river_mile(sq_mile, r_mile,	start_dir, start, finish_dir,
						   finish, course, num);

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

