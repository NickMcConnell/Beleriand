/**
 * \file gen-room.c
 * \brief Dungeon room generation.
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
 * This file covers everything to do with generation of individual rooms in
 * the dungeon.  It consists of room generating helper functions plus the 
 * actual room builders (which are referred to in the room profiles in
 * generate.c).
 *
 * The room builders all take as arguments the chunk they are being generated
 * in, and the co-ordinates of the room centre in that chunk.  Each room
 * builder is also able to find space for itself in the chunk using the 
 * find_space() function; the chunk generating functions can ask it to do that
 * by passing too large centre co-ordinates.
 */

#include "angband.h"
#include "cave.h"
#include "datafile.h"
#include "math.h"
#include "game-event.h"
#include "generate.h"
#include "init.h"
#include "mon-group.h"
#include "mon-make.h"
#include "mon-spell.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-make.h"
#include "obj-pile.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player-util.h"
#include "trap.h"
#include "z-queue.h"
#include "z-type.h"

/**
 * ------------------------------------------------------------------------
 * Selection of random templates
 * ------------------------------------------------------------------------ */
/**
 * Chooses a vault of a particular kind at random.
 * \param depth the current depth, for vault bound checking
 * \param typ vault type
 * \param forge whether we are forcing a forge
 * \return a pointer to the vault template
 */
struct vault *random_vault(int depth, const char *typ, bool forge)
{
	struct vault *v = vaults;
	struct vault *r = NULL;
	uint32_t rarity_sum = 0;
	do {
		if (streq(v->typ, typ) && (v->depth <= depth)) {
			bool valid = v->rarity > 0;
			/* Check if we need a forge and don't have one */
			if (forge && !v->forge) {
				valid = false;
			}

			/* Check if it's a greater vault we've already seen */
			if (streq(v->typ, "Greater vault") && player->vaults[v->index]) {
				valid = false;
			}

			if (valid) {
				rarity_sum += v->rarity;
				if (Rand_div(rarity_sum) < v->rarity) r = v;
			}
		}
		v = v->next;
	} while(v);
	return r;
}



/**
 * ------------------------------------------------------------------------
 * Room build helper functions
 * ------------------------------------------------------------------------ */
/**
 * Mark squares as being in a room, and optionally light them.
 * \param c the current chunk
 * \param y1 inclusive room boundaries
 * \param x1 inclusive room boundaries
 * \param y2 inclusive room boundaries
 * \param x2 inclusive room boundaries
 * \param light whether or not to light the room
 */
static void generate_room(struct chunk *c, int y1, int x1, int y2, int x2,
						  int light)
{
	struct loc grid;
	for (grid.y = y1; grid.y <= y2; grid.y++)
		for (grid.x = x1; grid.x <= x2; grid.x++) {
			sqinfo_on(square(c, grid)->info, SQUARE_ROOM);
			if (light)
				sqinfo_on(square(c, grid)->info, SQUARE_GLOW);
		}
}

/**
 * Mark a rectangle with a sqinfo flag
 * \param c the current chunk
 * \param y1 inclusive room boundaries
 * \param x1 inclusive room boundaries
 * \param y2 inclusive room boundaries
 * \param x2 inclusive room boundaries
 * \param flag the SQUARE_* flag we are marking with
 */
void generate_mark(struct chunk *c, int y1, int x1, int y2, int x2, int flag)
{
	struct loc grid;
	for (grid.y = y1; grid.y <= y2; grid.y++) {
		for (grid.x = x1; grid.x <= x2; grid.x++) {
			sqinfo_on(square(c, grid)->info, flag);
		}
	}
}

/**
 * Fill a rectangle with a feature.
 * \param c the current chunk
 * \param y1 inclusive room boundaries
 * \param x1 inclusive room boundaries
 * \param y2 inclusive room boundaries
 * \param x2 inclusive room boundaries
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 */
void fill_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat,
					int flag)
{
	int y, x;
	for (y = y1; y <= y2; y++)
		for (x = x1; x <= x2; x++)
			square_set_feat(c, loc(x, y), feat);
	if (flag) generate_mark(c, y1, x1, y2, x2, flag);
}

/**
 * Fill the edges of a rectangle with a feature.
 * \param c the current chunk
 * \param y1 inclusive room boundaries
 * \param x1 inclusive room boundaries
 * \param y2 inclusive room boundaries
 * \param x2 inclusive room boundaries
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 * \param overwrite_perm whether to overwrite features already marked as
 * permanent
 */
void draw_rectangle(struct chunk *c, int y1, int x1, int y2, int x2, int feat,
					int flag, bool overwrite_perm)
{
	int y, x;

	for (y = y1; y <= y2; y++) {
		if (overwrite_perm || !square_isperm(c, loc(x1, y))) {
			square_set_feat(c, loc(x1, y), feat);
		}
		if (overwrite_perm || !square_isperm(c, loc(x2, y))) {
			square_set_feat(c, loc(x2, y), feat);
		}
	}
	if (flag) {
		generate_mark(c, y1, x1, y2, x1, flag);
		generate_mark(c, y1, x2, y2, x2, flag);
	}
	for (x = x1; x <= x2; x++) {
		if (overwrite_perm || !square_isperm(c, loc(x, y1))) {
			square_set_feat(c, loc(x, y1), feat);
		}
		if (overwrite_perm || !square_isperm(c, loc(x, y2))) {
			square_set_feat(c, loc(x, y2), feat);
		}
	}
	if (flag) {
		generate_mark(c, y1, x1, y1, x2, flag);
		generate_mark(c, y2, x1, y2, x2, flag);
	}
}

/**
 * Fill a horizontal range with the given feature/info.
 * \param c the current chunk
 * \param y inclusive room boundaries
 * \param x1 inclusive room boundaries
 * \param x2 inclusive range boundaries
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 * \param light lit or not
 */
static void fill_xrange(struct chunk *c, int y, int x1, int x2, int feat, 
						int flag, bool light)
{
	int x;
	for (x = x1; x <= x2; x++) {
		struct loc grid = loc(x, y);
		square_set_feat(c, grid, feat);
		sqinfo_on(square(c, grid)->info, SQUARE_ROOM);
		if (flag) sqinfo_on(square(c, grid)->info, flag);
		if (light)
			sqinfo_on(square(c, grid)->info, SQUARE_GLOW);
	}
}

/**
 * Fill a vertical range with the given feature/info.
 * \param c the current chunk
 * \param x inclusive room boundaries
 * \param y1 inclusive room boundaries
 * \param y2 inclusive range boundaries
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 * \param light lit or not
 */
static void fill_yrange(struct chunk *c, int x, int y1, int y2, int feat, 
						int flag, bool light)
{
	int y;
	for (y = y1; y <= y2; y++) {
		struct loc grid = loc(x, y);
		square_set_feat(c, grid, feat);
		sqinfo_on(square(c, grid)->info, SQUARE_ROOM);
		if (flag) sqinfo_on(square(c, grid)->info, flag);
		if (light)
			sqinfo_on(square(c, grid)->info, SQUARE_GLOW);
	}
}

/**
 * Fill a circle with the given feature/info.
 * \param c the current chunk
 * \param y0 the circle centre
 * \param x0 the circle centre
 * \param radius the circle radius
 * \param border the width of the circle border
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 * \param light lit or not
 */
static void fill_circle(struct chunk *c, int y0, int x0, int radius, int border,
						int feat, int flag, bool light)
{
	int i, last = 0;
	int r2 = radius * radius;
	for(i = 0; i <= radius; i++) {
		double j = sqrt(r2 - (i * i));
		int k = (int)(j + 0.5);

		int b = border;
		if (border && last > k) b++;
		
		fill_xrange(c, y0 - i, x0 - k - b, x0 + k + b, feat, flag, light);
		fill_xrange(c, y0 + i, x0 - k - b, x0 + k + b, feat, flag, light);
		fill_yrange(c, x0 - i, y0 - k - b, y0 + k + b, feat, flag, light);
		fill_yrange(c, x0 + i, y0 - k - b, y0 + k + b, feat, flag, light);
		last = k;
	}
}

/**
 * Fill an ellipse with the given feature/info.
 * \param c the current chunk
 * \param y0 the ellipse centre
 * \param x0 the ellipse centre
 * \param y_radius the half-axis in the y direction
 * \param x_radius the half-axis in the x direction
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 * \param light lit or not
 */
void fill_ellipse(struct chunk *c, int y0, int x0, int y_radius, int x_radius,
				  int feat, int flag, bool light)
{
	int y;
	for (y = -y_radius; y <= y_radius; y++) {
		int x = 0;
		while (x * x * y_radius * y_radius + y * y * x_radius * x_radius <=
			   y_radius * y_radius * x_radius * x_radius) {
			x++;
		}

		fill_xrange(c, y0 + y, x0 - x, x0 + x, feat, flag, light);
	}
}

/**
 * Fill the lines of a cross/plus with a feature.
 *
 * \param c the current chunk
 * \param y1 inclusive room boundaries
 * \param x1 inclusive room boundaries
 * \param y2 inclusive room boundaries
 * \param x2 inclusive room boundaries
 * \param feat the terrain feature
 * \param flag the SQUARE_* flag we are marking with
 * When combined with draw_rectangle() this will generate a large rectangular 
 * room which is split into four sub-rooms.
 */
static void generate_plus(struct chunk *c, int y1, int x1, int y2, int x2, 
						  int feat, int flag)
{
	int y, x;

	/* Find the center */
	int y0 = (y1 + y2) / 2;
	int x0 = (x1 + x2) / 2;

	assert(c);

	for (y = y1; y <= y2; y++) square_set_feat(c, loc(x0, y), feat);
	if (flag) generate_mark(c, y1, x0, y2, x0, flag);
	for (x = x1; x <= x2; x++) square_set_feat(c, loc(x, y0), feat);
	if (flag) generate_mark(c, y0, x1, y0, x2, flag);
}

/**
 * Place a square of granite with a flag
 * \param c the current chunk
 * \param grid the square co-ordinates
 * \param flag the SQUARE_* flag we are marking with
 */
void set_marked_granite(struct chunk *c, struct loc grid, int flag)
{
	square_set_feat(c, grid, FEAT_GRANITE);
	if (flag) generate_mark(c, grid.y, grid.x, grid.y, grid.x, flag);
}

/**
 * Given a room (with all grids converted to floors), convert floors on the
 * edges to outer walls so no floor will be adjacent to a grid that is not a
 * floor or outer wall.
 * \param c the current chunk
 * \param y1 lower y bound for room's bounding box
 * \param x1 lower x bound for room's bounding box
 * \param y2 upper y bound for rooms' bounding box
 * \param x2 upper x bound for rooms' bounding box
 * Will not properly handle cases where rooms are close enough that their
 * minimal bounding boxes overlap.
 */
static void set_bordering_walls(struct chunk *c, int y1, int x1, int y2, int x2)
{
	int nx;
	struct loc grid;
	bool *walls;

	assert(x2 >= x1 && y2 >= y1);

	/* Set up storage to track which grids to convert. */
	nx = x2 - x1 + 1;
	walls = mem_zalloc((x2 - x1 + 1) * (y2 - y1 + 1) * sizeof(*walls));

	/* Find the grids to convert. */
	y1 = MAX(0, y1);
	y2 = MIN(c->height - 1, y2);
	x1 = MAX(0, x1);
	x2 = MIN(c->width - 1, x2);
	for (grid.y = y1; grid.y <= y2; grid.y++) {
		int adjy1 = MAX(0, grid.y - 1);
		int adjy2 = MIN(c->height - 1, grid.y + 1);

		for (grid.x = x1; grid.x <= x2; grid.x++) {
			if (square_isfloor(c, grid)) {
				int adjx1 = MAX(0, grid.x - 1);
				int adjx2 = MIN(c->width - 1, grid.x + 1);
				assert(square_isroom(c, grid));

				if (adjy2 - adjy1 != 2 || adjx2 - adjx1 != 2) {
					/*
					 * Adjacent grids are out of bounds.
					 * Make it an outer wall.
					 */
					walls[grid.x - x1 + nx *
						(grid.y - y1)] = true;
				} else {
					int nfloor = 0;
					struct loc adj;

					for (adj.y = adjy1;
							adj.y <= adjy2;
							adj.y++) {
						for (adj.x = adjx1;
								adj.x <= adjx2;
								adj.x++) {
							bool floor =
								square_isfloor(
								c, adj);

							assert(floor ==
								square_isroom(
								c, adj));
							if (floor) {
								++nfloor;
							}
						}
					}
					if (nfloor != 9) {
						/*
						 * At least one neighbor is not
						 * in the room.  Make it an
						 * outer wall.
						 */
						walls[grid.x - x1 + nx *
							(grid.y - y1)] = true;
					}
				}
			} else {
				assert(!square_isroom(c, grid));
			}
		}
	}

	/* Perform the floor to wall conversions. */
	for (grid.y = y1; grid.y <= y2; grid.y++) {
		for (grid.x = x1; grid.x <= x2; grid.x++) {
			if (walls[grid.x - x1 + nx * (grid.y - y1)]) {
				assert(square_isfloor(c, grid) &&
					square_isroom(c, grid));
				set_marked_granite(c, grid, SQUARE_WALL_OUTER);
			}
		}
	}

	mem_free(walls);
}

/**
 * Make a starburst room. -LM-
 *
 * \param c the current chunk
 * \param y1 boundaries which will contain the starburst
 * \param x1 boundaries which will contain the starburst
 * \param y2 boundaries which will contain the starburst
 * \param x2 boundaries which will contain the starburst
 * \param light lit or not
 * \param feat the terrain feature to make the starburst of
 * \param special_ok allow wacky cloverleaf rooms
 * \return success
 *
 * Starburst rooms are made in three steps:
 * 1: Choose a room size-dependent number of arcs.  Large rooms need to 
 *    look less granular and alter their shape more often, so they need 
 *    more arcs.
 * 2: For each of the arcs, calculate the portion of the full circle it 
 *    includes, and its maximum effect range (how far in that direction 
 *    we can change features in).  This depends on room size, shape, and 
 *    the maximum effect range of the previous arc.
 * 3: Use the table "get_angle_to_grid" to supply angles to each grid in 
 *    the room.  If the distance to that grid is not greater than the 
 *    maximum effect range that applies at that angle, change the feature 
 *    if appropriate (this depends on feature type).
 *
 * Usage notes:
 * - This function uses a table that cannot handle distances larger than 
 *   20, so it calculates a distance conversion factor for larger rooms.
 * - This function is not good at handling rooms much longer along one axis 
 *   than the other, so it divides such rooms up, and calls itself to handle
 *   each section.  
 * - It is safe to call this function on areas that might contain vaults or 
 *   pits, because "icky" and occupied grids are left untouched.
 *
 * - Mixing these rooms (using normal floor) with rectangular ones on a 
 *   regular basis produces a somewhat chaotic looking dungeon.  However, 
 *   this code does works well for lakes, etc.
 *
 */
extern bool generate_starburst_room(struct chunk *c, struct point_set *set,
									int y1, int x1, int y2, int x2,
									bool light, int feat, bool special_ok)
{
	int y0, x0, y, x, ny, nx;
	int i, d;
	int dist, max_dist, dist_conv, dist_check;
	int height, width;
	int degree_first, center_of_arc, degree;

	/* Special variant room.  Discovered by accident. */
	bool make_cloverleaf = false;

	/* Holds first degree of arc, maximum effect distance in arc. */
	int arc[45][2];

	/* Number (max 45) of arcs. */
	int arc_num;

	/* Make certain the room does not cross the dungeon edge. */
	if ((!square_in_bounds(c, loc(x1, y1))) ||
		(!square_in_bounds(c, loc(x2, y2))))
		return (false);

	/* Robustness -- test sanity of input coordinates. */
	if ((y1 + 2 >= y2) || (x1 + 2 >= x2))
		return (false);


	/* Get room height and width. */
	height = 1 + y2 - y1;
	width = 1 + x2 - x1;


	/* Handle long, narrow rooms by dividing them up. */
	if ((height > 5 * width / 2) || (width > 5 * height / 2)) {
		int tmp_ay, tmp_ax, tmp_by, tmp_bx;

		/* Get bottom-left borders of the first room. */
		tmp_ay = y2;
		tmp_ax = x2;
		if (height > width)
			tmp_ay = y1 + 2 * height / 3;
		else
			tmp_ax = x1 + 2 * width / 3;

		/* Make the first room. */
		(void) generate_starburst_room(c, set, y1, x1, tmp_ay, tmp_ax, light,
									   feat, false);


		/* Get top_right borders of the second room. */
		tmp_by = y1;
		tmp_bx = x1;
		if (height > width)
			tmp_by = y1 + 1 * height / 3;
		else
			tmp_bx = x1 + 1 * width / 3;

		/* Make the second room. */
		(void) generate_starburst_room(c, set, tmp_by, tmp_bx, y2, x2, light,
									   feat, false);


		/* If floor, extend a "corridor" between room centers, to ensure 
		 * that the rooms are connected together. */
		if (feat_is_floor(feat)) {
			for (y = (y1 + tmp_ay) / 2; y <= (tmp_by + y2) / 2; y++) {
				for (x = (x1 + tmp_ax) / 2; x <= (tmp_bx + x2) / 2; x++) {
					if (set && !point_set_contains(set, loc(x, y))) continue;
					square_set_feat(c, loc(x, y), feat);
				}
			}
		} else {
			/* Otherwise fill any gap between two starbursts. */
			int tmp_cy1, tmp_cx1, tmp_cy2, tmp_cx2;

			if (height > width) {
				tmp_cy1 = y1 + (height - width) / 2;
				tmp_cx1 = x1;
				tmp_cy2 = tmp_cy1 - (height - width) / 2;
				tmp_cx2 = x2;
			} else {
				tmp_cy1 = y1;
				tmp_cx1 = x1 + (width - height) / 2;
				tmp_cy2 = y2;
				tmp_cx2 = tmp_cx1 + (width - height) / 2;
			}

			/* Make the third room. */
			(void) generate_starburst_room(c, set, tmp_cy1, tmp_cx1, tmp_cy2,
										   tmp_cx2, light, feat, false);
		}

		/* Return. */
		return (true);
	}


	/* Get a shrinkage ratio for large rooms, as table is limited. */
	if ((width > 44) || (height > 44)) {
		if (width > height)
			dist_conv = 10 * width / 44;
		else
			dist_conv = 10 * height / 44;
	} else
		dist_conv = 10;


	/* Make a cloverleaf room sometimes. */
	if ((special_ok) && (height > 10) && (randint0(20) == 0)) {
		arc_num = 12;
		make_cloverleaf = true;
	}

	/* Usually, we make a normal starburst. */
	else {
		/* Ask for a reasonable number of arcs. */
		arc_num = 8 + (height * width / 80);
		arc_num = arc_num + 3 - randint0(7);
		if (arc_num < 8)
			arc_num = 8;
		if (arc_num > 45)
			arc_num = 45;
	}


	/* Get the center of the starburst. */
	y0 = y1 + height / 2;
	x0 = x1 + width / 2;

	/* Start out at zero degrees. */
	degree_first = 0;


	/* Determine the start degrees and expansion distance for each arc. */
	for (i = 0; i < arc_num; i++) {
		/* Get the first degree for this arc. */
		arc[i][0] = degree_first;

		/* Get a slightly randomized start degree for the next arc. */
		degree_first += (180 + randint0(arc_num)) / arc_num;
		if (degree_first < 180 * (i + 1) / arc_num)
			degree_first = 180 * (i + 1) / arc_num;
		if (degree_first > (180 + arc_num) * (i + 1) / arc_num)
			degree_first = (180 + arc_num) * (i + 1) / arc_num;


		/* Get the center of the arc. */
		center_of_arc = degree_first + arc[i][0];

		/* Calculate a reasonable distance to expand vertically. */
		if (((center_of_arc > 45) && (center_of_arc < 135))
			|| ((center_of_arc > 225) && (center_of_arc < 315))) {
			arc[i][1] = height / 4 + randint0((height + 3) / 4);
		}

		/* Calculate a reasonable distance to expand horizontally. */
		else if (((center_of_arc < 45) || (center_of_arc > 315))
				 || ((center_of_arc < 225) && (center_of_arc > 135))) {
			arc[i][1] = width / 4 + randint0((width + 3) / 4);
		}

		/* Handle arcs that count as neither vertical nor horizontal */
		else if (i != 0) {
			if (make_cloverleaf)
				arc[i][1] = 0;
			else
				arc[i][1] = arc[i - 1][1] + 3 - randint0(7);
		}


		/* Keep variability under control. */
		if ((!make_cloverleaf) && (i != 0) && (i != arc_num - 1)) {
			/* Water edges must be quite smooth. */
			if (feat_is_smooth(feat)) {
				if (arc[i][1] > arc[i - 1][1] + 2)
					arc[i][1] = arc[i - 1][1] + 2;

				if (arc[i][1] > arc[i - 1][1] - 2)
					arc[i][1] = arc[i - 1][1] - 2;
			} else {
				if (arc[i][1] > 3 * (arc[i - 1][1] + 1) / 2)
					arc[i][1] = 3 * (arc[i - 1][1] + 1) / 2;

				if (arc[i][1] < 2 * (arc[i - 1][1] - 1) / 3)
					arc[i][1] = 2 * (arc[i - 1][1] - 1) / 3;
			}
		}

		/* Neaten up final arc of circle by comparing it to the first. */
		if ((i == arc_num - 1) && (ABS(arc[i][1] - arc[0][1]) > 3)) {
			if (arc[i][1] > arc[0][1])
				arc[i][1] -= randint0(arc[i][1] - arc[0][1]);
			else if (arc[i][1] < arc[0][1])
				arc[i][1] += randint0(arc[0][1] - arc[i][1]);
		}
	}


	/* Precalculate check distance. */
	dist_check = 21 * dist_conv / 10;

	/* Change grids between (and not including) the edges. */
	for (y = y1 + 1; y < y2; y++) {
		for (x = x1 + 1; x < x2; x++) {
			struct loc grid = loc(x, y);

			/* Do not touch vault grids. */
			if (square_isvault(c, grid)) continue;

			/* Do not touch occupied grids. */
			if (square_monster(c, grid)) continue;
			if (square_object(c, grid)) continue;

			/* Be in any point set */
			if (set && !point_set_contains(set, grid)) continue;

			/* Get distance to grid. */
			dist = distance(loc(x0, y0), grid);

			/* Reject grid if outside check distance. */
			if (dist >= dist_check) continue;

			/* Convert and reorient grid for table access. */
			ny = 20 + 10 * (y - y0) / dist_conv;
			nx = 20 + 10 * (x - x0) / dist_conv;

			/* Illegal table access is bad. */
			if ((ny < 0) || (ny > 40) || (nx < 0) || (nx > 40)) continue;

			/* Get angle to current grid. */
			degree = get_angle_to_grid[ny][nx];

			/* Scan arcs to find the one that applies here. */
			for (i = arc_num - 1; i >= 0; i--) {
				if (arc[i][0] <= degree) {
					max_dist = arc[i][1];

					/* Must be within effect range. */
					if (max_dist >= dist) {
						/* If new feature is not passable, or floor, always 
						 * place it. */
						if (feat_is_floor(feat) || !feat_is_passable(feat)) {
							square_set_feat(c, grid, feat);

							if (feat_is_floor(feat)) {
								sqinfo_on(square(c, grid)->info, SQUARE_ROOM);
							} else {
								sqinfo_off(square(c, grid)->info, SQUARE_ROOM);
							}

							if (light) {
								sqinfo_on(square(c, grid)->info, SQUARE_GLOW);
							} else if (!square_isbright(c, grid)) {
								sqinfo_off(square(c, grid)->info, SQUARE_GLOW);
							}
						}

						/* If new feature is non-floor passable terrain,
						 * place it only over floor. */
						else {
							/* Replace old feature entirely in some cases. */
							if (feat_is_smooth(feat)) {
								if (square_isfloor(c, grid))
									square_set_feat(c, grid, feat);
							} else {
								/* Make denser in the middle. */
								if (square_isfloor(c, grid) &&
									(randint1(max_dist + 5) >= dist + 5))
									square_set_feat(c, grid, feat);
							}

							/* Light grid. */
							if (light)
								sqinfo_on(square(c, grid)->info, SQUARE_GLOW);
						}
					}

					/* Arc found.  End search */
					break;
				}
			}
		}
	}

	/*
	 * If we placed floors or dungeon granite, all dungeon granite next
	 * to floors needs to become outer wall.
	 */
	if (feat_is_floor(feat) || feat == FEAT_GRANITE) {
		for (y = y1 + 1; y < y2; y++) {
			for (x = x1 + 1; x < x2; x++) {

				struct loc grid = loc(x, y);
				/* Floor grids only */
				if (square_isfloor(c, grid)) {
					/* Look in all directions. */
					for (d = 0; d < 8; d++) {
						/* Extract adjacent location */
						struct loc grid1 = loc_sum(grid, ddgrid_ddd[d]);

						/* Join to room, forbid stairs */
						sqinfo_on(square(c, grid1)->info, SQUARE_ROOM);
						//sqinfo_on(square(c, grid1)->info, SQUARE_NO_STAIRS);

						/* Illuminate if requested. */
						if (light)
							sqinfo_on(square(c, grid1)->info, SQUARE_GLOW);

						/* Look for dungeon granite. */
						if (square(c, grid1)->feat == FEAT_GRANITE) {
							/* Mark as outer wall. */
							set_marked_granite(c, grid1, SQUARE_WALL_OUTER);
						}
					}
				}
			}
		}
	}

	/* Success */
	return (true);
}

/**
 * Check that a rectangular range has not been reserved in the block map.
 * \param by1 Is the y block coordinate for the top left corner of the range.
 * \param bx1 Is the x block coordinate for the top left corner of the range.
 * \param by2 Is the y block coordinate for the bottom right corner.
 * \param bx2 Is the x block coordinate for the bottom right corner.
 * \return Return true if the complete range has not been reserved and falls
 * within the bounds of the map.  Otherwise, return false.
 */
static bool check_for_unreserved_blocks(int by1, int bx1, int by2, int bx2)
{
	int by, bx;

	/* Never run off the screen */
	if (by1 < 0 || by2 >= dun->row_blocks) return false;
	if (bx1 < 0 || bx2 >= dun->col_blocks) return false;

	/* Verify open space */
	for (by = by1; by <= by2; by++) {
		for (bx = bx1; bx <= bx2; bx++) {
			if (dun->room_map[by][bx]) return false;
		}
	}
	return true;
}

/**
 * Reserve a rectangular range in the block map.
 * \param by1 Is the y block coordinate for the top left corner of the range.
 * \param bx1 Is the x block coordinate for the top left corner of the range.
 * \param by2 Is the y block coordinate for the bottom right corner.
 * \param bx2 Is the x block coordinate for the bottom right corner.
 */
static void reserve_blocks(int by1, int bx1, int by2, int bx2)
{
	int by, bx;

	for (by = by1; by <= by2; by++) {
		for (bx = bx1; bx <= bx2; bx++) {
			dun->room_map[by][bx] = true;
		}
	}
}

/**
 * Find a good spot for the next room.
 *
 * \param y centre of the room
 * \param x centre of the room
 * \param height dimensions of the room
 * \param width dimensions of the room
 * \return success
 *
 * Find and allocate a free space in the dungeon large enough to hold
 * the room calling this function.
 *
 * We allocate space in blocks.
 *
 * Be careful to include the edges of the room in height and width!
 *
 * Return true and values for the center of the room if all went well.
 * Otherwise, return false.
 */
static bool find_space(struct loc *centre, int height, int width)
{
	int i;
	int by1, bx1, by2, bx2;

	/* Find out how many blocks we need. */
	int blocks_high = 1 + ((height - 1) / dun->block_hgt);
	int blocks_wide = 1 + ((width - 1) / dun->block_wid);

	/* We'll allow twenty-five guesses. */
	for (i = 0; i < 25; i++) {
		/* Pick a top left block at random */
		by1 = randint0(dun->row_blocks);
		bx1 = randint0(dun->col_blocks);

		/* Extract bottom right corner block */
		by2 = by1 + blocks_high - 1;
		bx2 = bx1 + blocks_wide - 1;

		if (!check_for_unreserved_blocks(by1, bx1, by2, bx2)) continue;

		/* Get the location of the room */
		centre->y = ((by1 + by2 + 1) * dun->block_hgt) / 2;
		centre->x = ((bx1 + bx2 + 1) * dun->block_wid) / 2;

		/* Save the room location */
		if (dun->cent_n < z_info->level_room_max) {
			dun->cent[dun->cent_n] = *centre;
			dun->cent_n++;
		}

		reserve_blocks(by1, bx1, by2, bx2);

		/* Success. */
		return (true);
	}

	/* Failure. */
	return (false);
}

/**
 * Build a vault from its string representation.
 * \param c the chunk the room is being built in
 * \param centre the room centre; out of chunk centre invokes find_space()
 * \param v pointer to the vault template
 * \param flip whether or not to diagonally flip (interchange x and y) the
 * vault template
 * \return success
 */
bool build_vault(struct chunk *c, struct loc *centre, bool *rotated,
				 struct vault *v)
{
	const char *data = v->text;
	int y1, x1, y2, x2;
	int x, y;
	const char *t;
	int rotate, thgt, twid;
	bool reflect;
	bool transform = (centre->y >= c->height) || (centre->x >= c->width);
	bool floor = chunk_list[player->place].z_pos > 0;

	assert(c);

	/* Find and reserve some space in the dungeon.  Get center of room. */
	event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, v->name);
	if (transform) {
		get_random_symmetry_transform(v->hgt, v->wid, SYMTR_FLAG_NONE,
									  calc_default_transpose_weight(v->hgt, v->wid),
									  &rotate, &reflect, &thgt, &twid);
		if (rotate % 2) *rotated = true;
		event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE, thgt + 2, twid + 2);
		if (!find_space(centre, thgt + 2, twid + 2))
			return false;
	}

	/* Check that the vault doesn't contain invalid things for its depth */
	for (t = data, y = 0; y < v->hgt; y++) {	
		for (x = 0; x < v->wid; x++, t++) {
			/* Barrow wights can't be deeper than level 2 */
			if ((*t == 'W') && (c->depth > 2)) {
				return false;
			}

            /* Chasms can't occur at 450 ft */
			if ((*t == '7') && (c->depth >= z_info->dun_depth - 1)) {
				return false;
			}
		}
	}

	/* Convert centre to translation for the symmetry transformation. */
	centre->x -= twid / 2;
	centre->y -= thgt / 2;

	/* Get the room corners */
	y1 = centre->y;
	x1 = centre->x;
	y2 = y1 + thgt - 1;
	x2 = x1 + twid - 1;

	/* Place dungeon features and objects */
	get_terrain(c, loc(0, 0), loc(v->wid, v->hgt), *centre, v->hgt, v->wid,
				rotate, reflect, v->flags, floor, data, false);

	/* Finished if it's been generated before */
	if (!dun->first_time) return true;

	/* Switch random number generator so terrain always generates the same */
	Rand_quick = false;

	/* Save the current seed value */
	dun->seed = Rand_value;

	/* Place regular dungeon monsters and objects */
	for (t = data, y = 0; y < v->hgt && *t; y++) {
		for (x = 0; x < v->wid && *t; x++, t++) {
			struct loc grid = loc(x, y);
			struct monster_group_info info = { 0, 0 };

			if (transform) {
				symmetry_transform(&grid, centre->y, centre->x, v->hgt,
								   v->wid, rotate, reflect);
				assert(grid.x >= x1 && grid.x <= x2 &&
					   grid.y >= y1 && grid.y <= y2);
			}

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Analyze the symbol */
			switch (*t)
			{
				/* A monster from 1 level deeper */
				case '1': {
					pick_and_place_monster(c, '$', REALM_MORGOTH, grid,
										   player_danger_level(player) + 1,
										   true, true, ORIGIN_DROP_VAULT);
					break;
				}

				/* A monster from 2 levels deeper */
				case '2': {
					pick_and_place_monster(c, '$', REALM_MORGOTH, grid,
										   player_danger_level(player) + 2,
										   true, true, ORIGIN_DROP_VAULT);
					break;
				}

				/* A monster from 3 levels deeper */
				case '3': {
					pick_and_place_monster(c, '$', REALM_MORGOTH, grid,
										   player_danger_level(player) + 3,
										   true, true, ORIGIN_DROP_VAULT);
					break;
				}

				/* A monster from 4 levels deeper */
				case '4': {
					pick_and_place_monster(c, '$', REALM_MORGOTH, grid,
										   player_danger_level(player) + 4,
										   true, true, ORIGIN_DROP_VAULT);
					break;
				}

				/* An object from 1-4 levels deeper */
				case '*': {
					place_object(c, grid,
								 player_danger_level(player) + randint1(4),
								 false, false, ORIGIN_VAULT,
								 lookup_drop("not useless"));
					break;
				}

				/* A good object from 1-4 levels deeper */
				case '&': {
					place_object(c, grid,
								 player_danger_level(player) + randint1(4),
								 true, false, ORIGIN_VAULT,
								 lookup_drop("not useless"));
					break;
				}

				/* A chest from 4 levels deeper */
				case '~': {
					int depth = player_danger_level(player) + 4;
					place_object(c, grid, depth, false, false,
								 ORIGIN_VAULT, lookup_drop("chest"));
					break;
				}

				/* A skeleton */
				case 'S': {
					/* Make a skeleton 1/2 of the time */
					if (one_in_(2)) {
						struct object *obj = object_new();
						int sval;
						struct object_kind *kind;

						if (one_in_(3)) {
							sval = lookup_sval(TV_USELESS, "Human Skeleton");
						} else {
							sval = lookup_sval(TV_USELESS, "Elf Skeleton");
						}
						kind = lookup_kind(TV_USELESS, sval);

						/* Prepare the item */
						object_prep(obj, kind, player_danger_level(player),
									RANDOMISE);

						/* Drop it in the dungeon */
						drop_near(c, &obj, 0, grid, false, false);
					}
					break;
				}

				/* Monster and/or object from 1 level deeper */
				case '?': {
					int r = randint1(3);
					
					if (r <= 2) {
						pick_and_place_monster(c, '$', REALM_MORGOTH, grid,
											   player_danger_level(player) + 1,
											   true, true, ORIGIN_DROP_VAULT);
					}
					if (r >= 2) {
						place_object(c, grid, player_danger_level(player) + 1,
									 false, false, ORIGIN_VAULT, NULL);
					}
					break;
				}


				/* Carcharoth */
				case 'C': {
					place_new_monster_one(c, grid, lookup_monster("Carcharoth"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
				
				/* silent watcher */
				case 'H': {
					place_new_monster_one(c, grid,
										  lookup_monster("Silent watcher"),
										  true, false, info,
										  ORIGIN_DROP_VAULT);
					break;
				}

				/* easterling spy */
				case '@': {
					place_new_monster_one(c, grid,
										  lookup_monster("Easterling spy"),
										  true, false, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
					
				/* orc champion */
				case 'o': {
					place_new_monster_one(c, grid,
										  lookup_monster("Orc champion"), true,
										  false, info, ORIGIN_DROP_VAULT);
					break;
				}

				/* orc captain */
				case 'O': {
					place_new_monster_one(c, grid,
										  lookup_monster("Orc captain"), true,
										  false, info, ORIGIN_DROP_VAULT);
					break;
				}

				/* cat warrior */
				case 'f': {
					place_new_monster_one(c, grid,
										  lookup_monster("Cat warrior"), true,
										  false, info, ORIGIN_DROP_VAULT);
					break;
				}

				/* cat assassin */
				case 'F': {
					place_new_monster_one(c, grid,
										  lookup_monster("Cat assassin"), true,
										  false, info, ORIGIN_DROP_VAULT);
					break;
				}
					
				/* troll guard */
				case 'T': {
					place_new_monster_one(c, grid,
										  lookup_monster("Troll guard"), true,
										  false, info, ORIGIN_DROP_VAULT);
					break;
				}

				/* barrow wight */
				case 'W': {
					place_new_monster_one(c, grid,
										  lookup_monster("Barrow wight"), true,
										  false, info, ORIGIN_DROP_VAULT);
					break;
				}
				
				/* dragon */
				case 'd': {
					place_monster_by_flag(c, '$', REALM_MORGOTH, grid,
										  RF_DRAGON, -1, true,
										  player_danger_level(player) + 4,
										  false);
					break;
				}

				/* young cold drake */
				case 'y': {
					place_new_monster_one(c, grid,
										  lookup_monster("Young cold-drake"),
										  true, false, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
					
				/* young fire drake */
				case 'Y': {
					place_new_monster_one(c, grid,
									  lookup_monster("Young fire-drake"),
									  true, false, info, ORIGIN_DROP_VAULT);
					break;
				}
					
				/* Spider */
				case 'M': {
					place_monster_by_flag(c, '$', REALM_MORGOTH, grid,
										  RF_SPIDER, -1, true,
										  player_danger_level(player)
										  + rand_range(1, 4), false);
					break;
				}
				
				/* Vampire */
				case 'v': {
					place_monster_by_letter(c, '$', REALM_MORGOTH, grid, 'v',
											true, player_danger_level(player)
											+ rand_range(1, 4));
					break;
				}

                /* Archer */
				case 'a': {
					place_monster_by_flag(c, '$', REALM_MORGOTH, grid,
										  RSF_ARROW1, RSF_ARROW2, true,
										  player_danger_level(player) + 1,
										  true);
					break;
				}

                /* Flier */
				case 'b': {
					place_monster_by_flag(c, '$', REALM_MORGOTH, grid,
										  RF_FLYING, -1, true,
										  player_danger_level(player) + 1,
										  false);
					break;
				}

				/* Wolf */
				case 'c': {
					place_monster_by_flag(c, '$', REALM_MORGOTH, grid, RF_WOLF,
										  -1, true,
										  player_danger_level(player)
										  + rand_range(1, 4), false);
					break;
				}
					
				/* Rauko */
				case 'r': {
					place_monster_by_flag(c, '$', REALM_MORGOTH, grid, RF_RAUKO,
										  -1, true,
										  player_danger_level(player)
										  + rand_range(1, 4), false);
					break;
				}
					
                /* Aldor */
				case 'A': {
					place_new_monster_one(c, grid, lookup_monster("Aldor"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
                    
				/* Glaurung */
				case 'D': {
					place_new_monster_one(c, grid, lookup_monster("Glaurung"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}

				/* Gothmog */
				case 'R': {
					place_new_monster_one(c, grid, lookup_monster("Gothmog"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
					
				/* Ungoliant */
				case 'U': {
					place_new_monster_one(c, grid, lookup_monster("Ungoliant"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}

				/* Gorthaur */
				case 'G': {
					place_new_monster_one(c, grid, lookup_monster("Gorthaur"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
					
				/* Morgoth */
				case 'V': {
					place_new_monster_one(c, grid, lookup_monster("Morgoth, Lord of Darkness"),
										  true, true, info,
										  ORIGIN_DROP_VAULT);
					break;
				}
			}

		}
	}

	for (t = data, y = 0; y < v->hgt && *t; y++) {
		for (x = 0; x < v->wid && *t; x++, t++) {
			struct loc grid = loc(x, y);
			int mult;

			if (transform) {
				symmetry_transform(&grid, centre->y, centre->x, v->hgt,
								   v->wid, rotate, reflect);
				assert(grid.x >= x1 && grid.x <= x2 &&
					   grid.y >= y1 && grid.y <= y2);
			}

			/* Hack -- skip "non-grids" */
			if (*t == ' ') continue;

			/* Some vaults are always lit */
			if (roomf_has(v->flags, ROOMF_LIGHT)) {
				sqinfo_on(square(c, grid)->info, SQUARE_GLOW);
			}

			/* Traps are usually 5 times as likely in vaults,
			 * but are 10 times as likely if the TRAPS flag is set */
			mult = roomf_has(v->flags, ROOMF_TRAPS) ? 10 : 5;

			/* Another chance to place traps, with 4 times the normal chance
			 * so traps in interesting rooms and vaults are a total of 5 times
			 * more likely */
			if (randint1(1000) <= trap_placement_chance(c, grid) * (mult - 1)) {
				square_add_trap(c, grid);
			} else if (roomf_has(v->flags, ROOMF_WEBS) && one_in_(20)) {
				/* Webbed vaults also have a large chance of receiving webs */
				square_add_web(c, grid);

				/* Hide it half the time */
				if (one_in_(2)) {
					struct trap *trap = square_trap(c, grid);
					trf_on(trap->flags, TRF_INVISIBLE);
				}
			}
		}
	}

	/* Revert to the quick RNG, restore the seed */
	Rand_value = dun->seed;
	Rand_quick = true;

	return true;
}

/**
 * Helper function for building vaults.
 * \param c the chunk the room is being built in
 * \param centre the room centre; out of chunk centre invokes find_space()
 * \param typ the vault type
 * \param forge whether we are forcing a forge
 * \return success
 */
static bool build_vault_type(struct chunk *c, const char *typ, bool forge)
{
	struct loc centre = loc(c->width, c->height);
	bool rotated = false;
	struct vault *v = random_vault(c->depth, typ, forge);
	if (v == NULL) {
		return false;
	}

	/* Build the vault */
	if (!build_vault(c, &centre, &rotated, v)) {
		return false;
	}

	ROOM_LOG("%s (%s)", typ, v->name);

	/* Memorise and mark greater vaults */
	if (streq(typ, "Greater vault")) {
		int y1, x1, y2, x2;

		if (rotated) {
			/* Determine the coordinates with height/width flipped */
			y1 = centre.y - (v->wid / 2);
			x1 = centre.x - (v->hgt / 2);
			y2 = y1 + v->wid - 1;
			x2 = x1 + v->hgt - 1;
		} else {
			/* Determine the coordinates */
			y1 = centre.y - (v->hgt / 2);
			x1 = centre.x - (v->wid / 2);
			y2 = y1 + v->hgt - 1;
			x2 = x1 + v->wid - 1;
		}

		player->vaults[v->index] = true;
		generate_mark(c, y1, x1, y2, x2, SQUARE_G_VAULT);
		assert(!c->vault_name);
		c->vault_name = string_make(v->name);
	}

	return true;
}


/**
 * ------------------------------------------------------------------------
 * Room builders
 * ------------------------------------------------------------------------ */
/**
 * Build a staircase to connect with a previous staircase on the level one up
 * or (occasionally) one down
 */
bool build_staircase(struct chunk *c, struct loc centre)
{
	struct connector *join = dun->curr_join;
	struct loc tl, br;
	int by1, bx1, by2, bx2;

	if (!join) {
		quit_fmt("build_staircase() called without dun->curr_join set");
	}

	/*
	 * Verify that there's space for the 1 x 1 room at the
	 * staircase location (3 x 3 including the walls; if not at
	 * an edge also want a one grid buffer around the walls so
	 * the wall piercings for tunneling will work).
	 */

	centre = join->grid;
	if (centre.y < 1 || centre.y > c->height - 2 || centre.x < 1 ||
		centre.x > c->width - 2) return false;
	tl = loc(centre.x - ((centre.x > 1) ? 2 : 1),
			 centre.y - ((centre.y > 1) ? 2 : 1));
	br = loc(centre.x + ((centre.x < c->width - 2) ? 2 : 1),
			 centre.y + ((centre.y < c->height - 2) ? 2 : 1));
	event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE,
					  br.y - tl.y + 1, br.x - tl.x + 1);
	by1 = tl.y / dun->block_hgt;
	bx1 = tl.x / dun->block_wid;
	by2 = br.y / dun->block_hgt;
	bx2 = br.x / dun->block_wid;
	/*
	 * If the block size is greater than one, look for room flags
	 * rather than check the block map.  It's less efficient, but
	 * gives a better chance of success since multiple staircase
	 * rooms could be placed in a block if they're far enough apart.
	 */
	if (dun->block_hgt > 1 || dun->block_wid > 1) {
		struct loc rg;

		if (cave_find_in_range(c, &rg, tl, br, square_isroom))
			return false;
	} else if (!check_for_unreserved_blocks(by1, bx1, by2, bx2)) {
		return false;
	}

	reserve_blocks(by1, bx1, by2, bx2);

	/* Save the room location */
	if (dun->cent_n < z_info->level_room_max) {
		dun->cent[dun->cent_n] = centre;
		dun->cent_n++;
	}

	/* Generate new room and outer walls */
	generate_room(c, centre.y - 1, centre.x - 1, centre.y + 1, centre.x + 1,
				  false);
	draw_rectangle(c, centre.y - 1, centre.x - 1, centre.y + 1, centre.x + 1,
		FEAT_GRANITE, SQUARE_WALL_OUTER, false);

	/* Place the correct stair or shaft */
	square_set_feat(c, centre, join->feat);

	/* Success */
	return true;
}

/**
 * Build a circular room (interior radius 4-7).
 * \param c the chunk the room is being built in
 * \param centre the room centre; out of chunk centre invokes find_space()
 * \return success
 */
bool build_circular(struct chunk *c, struct loc centre)
{
	/* Pick a room size */
	int radius = 2 + randint1(2) + randint1(3);

	/* Occasional light */
	bool light = player->depth <= randint1(8) ? true : false;

	/* Find and reserve lots of space in the dungeon.  Get center of room. */
	event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE,
		2 * radius + 10, 2 * radius + 10);
	if (!find_space(&centre, 2 * radius + 10, 2 * radius + 10))
		return (false);

	/* Mark as a room. */
	fill_circle(c, centre.y, centre.x, radius + 1, 0, FEAT_FLOOR,
		SQUARE_NONE, light);

	/* Convert some floors to be the outer walls. */
	set_bordering_walls(c, centre.y - radius - 2, centre.x - radius - 2,
		centre.y + radius + 2, centre.x + radius + 2);

	/* Especially large circular rooms will have a middle chamber */
	if (radius - 4 > 0 && randint0(4) < radius - 4) {
		struct loc offset;

		event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "middle chamber");

		/* choose a random direction */
		rand_dir(&offset);

		/* draw a room with a closed door on a random side */
		draw_rectangle(c, centre.y - 2, centre.x - 2, centre.y + 2,
			centre.x + 2, FEAT_GRANITE, SQUARE_WALL_INNER, false);
		place_closed_door(c, loc(centre.x + offset.x * 2,
								 centre.y + offset.y * 2));
	}

	return true;
}

/**
 * Build an elliptical room (interior radius 4-7).
 * \param c the chunk the room is being built in
 * \param centre the room centre; out of chunk centre invokes find_space()
 * \return success
 */
bool build_elliptical(struct chunk *c, struct loc centre)
{
	/* Pick a room size */
	int y_radius = 2 + randint1(2) + randint1(5);
	int x_radius = 2 + randint1(2) + randint1(5);

	/* Occasional light */
	bool light = player->depth <= randint1(8) ? true : false;

	/* Find and reserve lots of space in the dungeon.  Get center of room. */
	event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE,
		2 * y_radius + 10, 2 * x_radius + 10);
	if (!find_space(&centre, 2 * y_radius + 10, 2 * x_radius + 10))
		return (false);

	/* Mark as a room. */
	fill_ellipse(c, centre.y, centre.x, y_radius + 1, x_radius + 1,
				 FEAT_FLOOR, SQUARE_NONE, light);

	/* Convert some floors to be the outer walls. */
	set_bordering_walls(c, centre.y - y_radius - 2, centre.x - x_radius - 2,
		centre.y + y_radius + 2, centre.x + x_radius + 2);

	/* Especially large elliptical rooms will have pillars at the foci */
	//TODO actually do this
	if (y_radius - 5 > 0 && randint0(4) < x_radius - 4) {//WRONG
		struct loc offset;

		event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "middle chamber");

		/* choose a random direction */
		rand_dir(&offset);

		/* draw a room with a closed door on a random side */
		draw_rectangle(c, centre.y - 2, centre.x - 2, centre.y + 2,
			centre.x + 2, FEAT_GRANITE, SQUARE_WALL_INNER, false);
		place_closed_door(c, loc(centre.x + offset.x * 2,
								 centre.y + offset.y * 2));
	}

	return true;
}

/**
 * Builds a normal rectangular room.
 * \param c the chunk the room is being built in
 * \param centre the room centre
 * \return success
 */
bool build_simple(struct chunk *c, struct loc centre)
{
	int y, x, y1, x1, y2, x2;
	bool light = false;

	/* Pick a room size */
	int height = 1 + randint1(4) + randint1(3);
	int width = 1 + randint1(11) + randint1(11);

	/* Find and reserve some space in the dungeon.  Get center of room. */
	event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE, height + 2, width + 2);
	if ((centre.y >= c->height) || (centre.x >= c->width)) {
		if (!find_space(&centre, height + 2, width + 2))
			return (false);
	}

	/* Occasional light - chance of darkness starts very small and
	 * increases quadratically until always dark at 450 ft */
	if ((c->depth < randint1(z_info->dun_depth - 1)) ||
		(c->depth < randint1(z_info->dun_depth - 1))) {
		light = true;
	}

	/* Set bounds */
	y1 = centre.y - height / 2;
	x1 = centre.x - width / 2;
	y2 = y1 + height - 1;
	x2 = x1 + width - 1;

	/* Generate new room */
	generate_room(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, light);

	/* Generate outer walls and inner floors */
	draw_rectangle(c, y1 - 1, x1 - 1, y2 + 1, x2 + 1, FEAT_GRANITE,
				   SQUARE_WALL_OUTER, false);
	fill_rectangle(c, y1, x1, y2, x2, FEAT_FLOOR, SQUARE_NONE);

	/* Sometimes make a pillar room */
	if (one_in_(20) && ((x2 - x1) % 2 == 0) && ((y2 - y1) % 2 == 0)) {
		event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "pillared");

		for (y = y1 + 1; y <= y2; y += 2) {
			for (x = x1 + 1; x <= x2; x += 2) {
				set_marked_granite(c, loc(x, y), SQUARE_WALL_INNER);
			}
		}
	} else if (one_in_(10) && ((x2 - x1) % 2 == 0) && ((y2 - y1) % 2 == 0)) {
		/* Sometimes make a pillar-lined room */
		event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "ragged");

		for (y = y1 + 1; y <= y2; y += 2) {
			for (x = x1 + 1; x <= x2; x += 2) {
				if ((x == x1 + 1) || (x == x2 - 1) || (y == y1 + 1) ||
					(y == y2 - 1)) {
					set_marked_granite(c, loc(x, y), SQUARE_WALL_INNER);
				}
			}
		}
	}

	return true;
}


/**
 * Builds an overlapping rectangular room.
 * \param c the chunk the room is being built in
 * \param centre the room centre; out of chunk centre invokes find_space()
 * \return success
 */
bool build_overlap(struct chunk *c, struct loc centre)
{
	int y1a, x1a, y2a, x2a;
	int y1b, x1b, y2b, x2b;
	int height, width;

	int light = false;

	/* Occasional light - always at level 1 down to never at Morgoth's level */
	if (c->depth <= randint1(z_info->dun_depth - 1)) light = true;

	/* Determine extents of room (a) */
	y1a = randint1(4);
	x1a = randint1(11);
	y2a = randint1(3);
	x2a = randint1(10);

	/* Determine extents of room (b) */
	y1b = randint1(3);
	x1b = randint1(10);
	y2b = randint1(4);
	x2b = randint1(11);

	/* Calculate height and width */
	height = 2 * MAX(MAX(y1a, y2a), MAX(y1b, y2b)) + 1;
	width = 2 * MAX(MAX(x1a, x2a), MAX(x1b, x2b)) + 1;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE, height + 2, width + 2);
	if (!find_space(&centre, height + 2, width + 2))
		return (false);

	/* locate room (a) */
	y1a = centre.y - y1a;
	x1a = centre.x - x1a;
	y2a = centre.y + y2a;
	x2a = centre.x + x2a;

	/* locate room (b) */
	y1b = centre.y - y1b;
	x1b = centre.x - x1b;
	y2b = centre.y + y2b;
	x2b = centre.x + x2b;

	/* Generate new room (a) */
	generate_room(c, y1a - 1, x1a - 1, y2a + 1, x2a + 1, light);

	/* Generate new room (b) */
	generate_room(c, y1b - 1, x1b - 1, y2b + 1, x2b + 1, light);

	/* Generate outer walls (a) */
	draw_rectangle(c, y1a - 1, x1a - 1, y2a + 1, x2a + 1, 
		FEAT_GRANITE, SQUARE_WALL_OUTER, false);

	/* Generate outer walls (b) */
	draw_rectangle(c, y1b - 1, x1b - 1, y2b + 1, x2b + 1, 
		FEAT_GRANITE, SQUARE_WALL_OUTER, false);

	/* Generate inner floors (a) */
	fill_rectangle(c, y1a, x1a, y2a, x2a, FEAT_FLOOR, SQUARE_NONE);

	/* Generate inner floors (b) */
	fill_rectangle(c, y1b, x1b, y2b, x2b, FEAT_FLOOR, SQUARE_NONE);

	return true;
}


/**
 * Builds a cross-shaped room.
 * \param c the chunk the room is being built in
 * \param centre the room centre
 * \return success
 *
 * Room "v" runs north/south, and Room "h" runs east/west 
 */
bool build_crossed(struct chunk *c, struct loc centre)
{
	int y, x;
	int height, width;

	int y1h, x1h, y2h, x2h;
	int y1v, x1v, y2v, x2v;

	int h_hgt, h_wid, v_hgt, v_wid;

	int light = false;

	/* Occasional light - always at level 1 down to never at Morgoth's level */
	if (c->depth <= randint1(z_info->dun_depth - 1)) light = true;

	/* Pick a room size */
	h_hgt = 1;                /* 3 */
	h_wid = rand_range(5, 7); /* 11, 13, 15 */

	v_hgt = rand_range(3, 6); /* 7, 9, 11, 13 */
	v_wid = rand_range(1, 2); /* 3, 5 */

	/* Calculate height and width */
	height = 2 * v_hgt + 1;
	width = 2 * h_wid + 1;

	/* Find and reserve some space in the dungeon.  Get center of room. */
	event_signal_size(EVENT_GEN_ROOM_CHOOSE_SIZE, height + 2, width + 2);
	if (!find_space(&centre, height + 2, width + 2))
		return (false);

	/* Get the room boundaries */
	y1h = centre.y - h_hgt;
	x1h = centre.x - h_wid;
	y2h = centre.y + h_hgt;
	x2h = centre.x + h_wid;

	y1v = centre.y - v_hgt;
	x1v = centre.x - v_wid;
	y2v = centre.y + v_hgt;
	x2v = centre.x + v_wid;

	/* Generate new rooms */
	generate_room(c, y1h - 1, x1h - 1, y2h + 1, x2h + 1, light);
	generate_room(c, y1v - 1, x1v - 1, y2v + 1, x2v + 1, light);

	/* Generate outer walls */
	draw_rectangle(c, y1h - 1, x1h - 1, y2h + 1, x2h + 1, 
				   FEAT_GRANITE, SQUARE_WALL_OUTER, false);
	draw_rectangle(c, y1v - 1, x1v - 1, y2v + 1, x2v + 1,
				   FEAT_GRANITE, SQUARE_WALL_OUTER, false);

	/* Generate inner floors */
	fill_rectangle(c, y1h, x1h, y2h, x2h, FEAT_FLOOR, SQUARE_NONE);
	fill_rectangle(c, y1v, x1v, y2v, x2v, FEAT_FLOOR, SQUARE_NONE);

	/* Special features */
	switch (randint1(7)) {
		case 1: {
			event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "chest");
			if ((v_wid == 2) && (v_hgt == 6)) {
				for (y = y1v + 1; y <= y2v; y += 2) {
					for (x = x1v + 1; x <= x2v; x += 2) {
						set_marked_granite(c, loc(x, y), SQUARE_WALL_INNER);
					}
				}
				place_object(c, centre, player_danger_level(player), false,
							 false, ORIGIN_SPECIAL, lookup_drop("chest"));
			}
			break;
		}

		case 2: {
			if ((v_wid == 1) && (h_hgt == 1)) {
				event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "plus");
				generate_plus(c, centre.y - 1, centre.x - 1, centre.y + 1,
							  centre.x + 1, FEAT_GRANITE, SQUARE_WALL_INNER);
			}
			break;
		}

		case 3: {
			if ((v_wid == 1) && (h_hgt == 1)) {
				event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "pinched");

				set_marked_granite(c, loc(centre.x - 1, centre.y - 1),
								   SQUARE_WALL_INNER);
				set_marked_granite(c, loc(centre.x - 1, centre.y + 1),
								   SQUARE_WALL_INNER);
				set_marked_granite(c, loc(centre.x + 1, centre.y - 1),
								   SQUARE_WALL_INNER);
				set_marked_granite(c, loc(centre.x + 1, centre.y + 1),
								   SQUARE_WALL_INNER);
			}
			break;
		}

		case 4: {
			if ((v_wid == 1) && (h_hgt == 1)) {
				event_signal_string(EVENT_GEN_ROOM_CHOOSE_SUBTYPE, "hollow plus");

				set_marked_granite(c, loc(centre.x - 1, centre.y),
								   SQUARE_WALL_INNER);
				set_marked_granite(c, loc(centre.x + 1, centre.y),
								   SQUARE_WALL_INNER);
				set_marked_granite(c, loc(centre.x, centre.y - 1),
								   SQUARE_WALL_INNER);
				set_marked_granite(c, loc(centre.x, centre.y + 1),
								   SQUARE_WALL_INNER);
			}
			break;
		}
		default: {
			break;
		}
	}

	return true;
}


/**
 * Build an interesting room.
 * \param c the chunk the room is being built in
 * \param centre the room centre
 * \return success
 */
bool build_interesting(struct chunk *c, struct loc centre)
{
	return build_vault_type(c, "Interesting room", player->upkeep->force_forge);
}


/**
 * Build a lesser vault.
 * \param c the chunk the room is being built in
 * \param centre the room centre
 * \return success
 */
bool build_lesser_vault(struct chunk *c, struct loc centre)
{
	return build_vault_type(c, "Lesser vault", false);
}


/**
 * Build a greater vault.
 * \param c the chunk the room is being built in
 * \param centre the room centre
 * \return success
 */

bool build_greater_vault(struct chunk *c, struct loc centre)
{
	/* Can only have one greater vault per level */
	if (c->vault_name) {
		return false;
	}
	return build_vault_type(c, "Greater vault", false);
}


/**
 * Build Morgoth's throne room.
 * \param c the chunk the room is being built in
 * \param centre the room centre
 * \return success
 */
bool build_throne(struct chunk *c, struct loc centre)
{
	int y1, x1, y2, x2;
	bool dummy = false;
	struct vault *v = random_vault(c->depth, "Throne room", false);
	if (v == NULL) {
		return false;
	}

	/* Determine the coordinates */
	centre = loc(c->width / 2, c->height / 2);
	y1 = centre.y - (v->hgt / 2);
	x1 = centre.x - (v->wid / 2);
	y2 = y1 + v->hgt - 1;
	x2 = x1 + v->wid - 1;

	/* Build the vault */
	if (!build_vault(c, &centre, &dummy, v)) {
		return false;
	}

	/* Memorise and mark */
	generate_mark(c, y1, x1, y2, x2, SQUARE_G_VAULT);
	assert(!c->vault_name);
	c->vault_name = string_make(v->name);

	return true;
}

/**
 * Attempt to build a room of the given type at the given block
 *
 * \param c the chunk the room is being built in
 * \param profile the profile of the rooom we're trying to build
 * \return success
 *
 * Note that this code assumes that profile height and width are the maximum
 * possible grid sizes, and then allocates a number of blocks that will always
 * contain them.
 */
bool room_build(struct chunk *c, struct room_profile profile)
{
	event_signal_string(EVENT_GEN_ROOM_START, profile.name);

	/* Enforce the room profile's minimum depth */
	if (player->depth < profile.level) {
		event_signal_flag(EVENT_GEN_ROOM_END, false);
		return false;
	}

	/* Try to build a room */
	if (!profile.builder(c, loc(c->width, c->height))) {
		event_signal_flag(EVENT_GEN_ROOM_END, false);
		return false;
	}

	/* Success */
	event_signal_flag(EVENT_GEN_ROOM_END, true);
	return true;
}
