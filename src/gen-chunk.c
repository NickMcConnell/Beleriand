/**
 * \file gen-chunk.c 
 * \brief Handling of chunks of cave
 *
 * Copyright (c) 2014 Nick McConnell
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
 * This file maintains a list of saved chunks of world which can be reloaded
 * at any time.  The initial example of this is the town, which is saved
 * immediately after generation and restored when the player returns there.
 *
 * The copying routines are also useful for generating a level in pieces and
 * then copying those pieces into the actual level chunk.
 */

#include "angband.h"
#include "cave.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-group.h"
#include "mon-make.h"
#include "mon-move.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "trap.h"

uint16_t chunk_max = 1;				/* Number of allocated chunks */
uint16_t chunk_cnt = 0;				/* Number of live chunks */
uint32_t gen_loc_cnt = 0;			/* Number of actual generated locations */
struct chunk_ref *chunk_list;     /**< list of pointers refs to saved chunks */


/**
 * ------------------------------------------------------------------------
 * Chunk transforming routines
 * ------------------------------------------------------------------------ */
/**
 * Transform y, x coordinates by rotation, reflection and translation
 * Stolen from PosChengband
 * \param grid the grid being transformed
 * \param y0 how much the grid is being translated vertically
 * \param x0 how much the grid is being translated horizontally
 * \param height height of the chunk
 * \param width width of the chunk
 * \param rotate how much to rotate, in multiples of 90 degrees clockwise
 * \param reflect whether to reflect horizontally
 */
void symmetry_transform(struct loc *grid, int y0, int x0, int height, int width,
						int rotate, bool reflect)
{
	/* Track what the dimensions are after rotations. */
	int rheight = height, rwidth = width;
	int i;

	/* Rotate (in multiples of 90 degrees clockwise) */
	for (i = 0; i < rotate % 4; i++) {
		int temp = grid->x;
		grid->x = rheight - 1 - (grid->y);
		grid->y = temp;
		temp = rwidth;
		rwidth = rheight;
		rheight = temp;
	}

	/* Reflect (horizontally in the rotated system) */
	if (reflect)
		grid->x = rwidth - 1 - grid->x;

	/* Translate */
	grid->y += y0;
	grid->x += x0;
}

/**
 * Select a random symmetry transformation subject to certain constraints.
 * \param height Is the height of the piece to transform.
 * \param width Is the width of the piece to transform.
 * \param flags Is a bitwise-or of one or more of SYMTR_FLAG_NONE,
 * SYMTR_FLAG_NO_ROT (disallow 90 and 270 degree rotation and 180 degree
 * rotation if not accompanied by a horizontal reflection - equivalent to a
 * vertical reflection), SYMTR_FLAG_NO_REF (forbid horizontal reflection), and
 * SYMTR_FLAG_FORCE_REF (force horizontal reflection).  If flags
 * includes both SYMTR_FLAG_NO_REF and SYMTR_FLAG_FORCE_REF, the former takes
 * precedence.
 * \param transpose_weight Is the probability weight to use for transformations
 * that include a tranposition (90 degree rotation, 270 degree rotation,
 * 90 degree rotation + horizontal reflection, 270 degree rotation + horizontal
 * reflection).  Coerced to be in the range of [0, SYMTR_MAX_WEIGHT] where 0
 * means forbidding such transformations.
 * \param rotate *rotate is set to the number of 90 degree clockwise rotations
 * to perform for the random transform.
 * \param reflect *reflect is set to whether the random transform includes a
 * horizontal reflection.
 * \param theight If theight is not NULL, *theight is set to the height of the
 * piece after applying the transform.
 * \param twidth If twidth is not NULL, *twidth is set to the width of the
 * piece after applying the transform.
 */
void get_random_symmetry_transform(int height, int width, int flags,
	int transpose_weight, int *rotate, bool *reflect,
	int *theight, int *twidth)
{
	/*
	 * Without any constraints there are 8 possibilities (4 rotations times
	 * 2 options for whether or not there is a horizontal reflection).
	 * Use an array of 9 elements (extra element for a leading zero) to
	 * store the cumulative probability weights.  The first four are for
	 * rotations without reflection.  The remainder are for the rotations
	 * with reflection.
	 */
	int weights[9], draw, ilow, ihigh;

	transpose_weight = MIN(SYMTR_MAX_WEIGHT, MAX(0, transpose_weight));
	weights[0] = 0;
	if ((flags & SYMTR_FLAG_NO_REF) || !(flags & SYMTR_FLAG_FORCE_REF)) {
		weights[1] = weights[0] + SYMTR_MAX_WEIGHT;
	} else {
		weights[1] = weights[0];
	}
	if (flags & SYMTR_FLAG_NO_ROT) {
		weights[2] = weights[1];
		weights[3] = weights[2];
		weights[4] = weights[3];
	} else if ((flags & SYMTR_FLAG_NO_REF) ||
			!(flags & SYMTR_FLAG_FORCE_REF)) {
		weights[2] = weights[1] + transpose_weight;
		weights[3] = weights[2] + SYMTR_MAX_WEIGHT;
		weights[4] = weights[3] + transpose_weight;
	} else {
		/* Reflection is forced so these all have zero weight. */
		weights[2] = weights[1];
		weights[3] = weights[2];
		weights[4] = weights[3];
	}
	if (flags & SYMTR_FLAG_NO_REF) {
		/* Reflection is forbidden so these all have zero weight. */
		weights[5] = weights[4];
		weights[6] = weights[5];
		weights[7] = weights[6];
		weights[8] = weights[7];
	} else {
		weights[5] = weights[4] + SYMTR_MAX_WEIGHT;
		if (flags & SYMTR_FLAG_NO_ROT) {
			weights[6] = weights[5];
			/*
			 * 180 degree rotation with a horizontal reflection is
			 * equivalent to a vertical reflection so don't exclude
			 * in when forbidding rotations.
			 */
			weights[7] = weights[6] + SYMTR_MAX_WEIGHT;
			weights[8] = weights[7];
		} else {
			weights[6] = weights[5] + transpose_weight;
			weights[7] = weights[6] + SYMTR_MAX_WEIGHT;
			weights[8] = weights[7] + transpose_weight;
		}
	}
	assert(weights[8] > 0);

	draw = randint0(weights[8]);

	/* Find by a binary search. */
	ilow = 0;
	ihigh = 8;
	while (1) {
		int imid;

		if (ilow == ihigh - 1) {
			break;
		}
		imid = (ilow + ihigh) / 2;
		if (weights[imid] <= draw) {
			ilow = imid;
		} else {
			ihigh = imid;
		}
	}

	*rotate = ilow % 4;
	*reflect = (ilow >= 4);
	if (theight) {
		*theight = (*rotate == 0 || *rotate == 2) ?  height : width;
	}
	if (twidth) {
		*twidth = (*rotate == 0 || *rotate == 2) ?  width : height;
	}
}

/**
 * Select a weight for transforms that involve transpositions so that
 * such transforms are forbidden if width >= 2 * height and the probability of
 * such a transform increases as height / width up to a maximum of
 * SYMTR_MAX_WEIGHT when the height is greater than or equal to the width.
 * That's so transformed pieces will usually fit well into the aspect ratio
 * of generated levels.
 * \param height Is the height of the piece being transformed.
 * \param width Is the width of the piece being transformed.
 */
int calc_default_transpose_weight(int height, int width)
{
	return (SYMTR_MAX_WEIGHT / 64) *
		MAX(0, MIN(64, (128 * height) / width - 64));
}

/**
 * ------------------------------------------------------------------------
 * Chunk copying routines
 * ------------------------------------------------------------------------ */
/**
 * Write the grid details (terrain, objects, traps) of a chunk to another
 */
static void chunk_copy_grid(struct player *p, struct chunk *source,
							struct chunk *dest, int height, int width,
							struct loc src_top_left, struct loc dest_top_left,
							int idx, int rotate, bool reflect, bool player_here)
{
	struct loc grid, trans = loc_diff(dest_top_left, src_top_left);

	/* Write the location stuff (terrain, objects, traps) */
	for (grid.y = src_top_left.y; grid.y < src_top_left.y + height; grid.y++) {
		for (grid.x = src_top_left.x; grid.x < src_top_left.x + width;
			 grid.x++) {
			struct monster *mon = square_monster(source, grid);
			/* Work out where we're going */
			struct loc dest_grid = grid;
			symmetry_transform(&dest_grid, trans.y, trans.x, height, width,
							   rotate, reflect);

			/* Terrain */
			dest->squares[dest_grid.y][dest_grid.x].feat =
				square(source, grid)->feat;
			sqinfo_copy(square(dest, dest_grid)->info,
						square(source, grid)->info);

			/* Dungeon objects */
			if (square_object(source, grid)) {
				struct object *obj;
				dest->squares[dest_grid.y][dest_grid.x].obj =
					square_object(source, grid);

				for (obj = square_object(source, grid); obj; obj = obj->next) {
					/* Adjust position */
					obj->grid = dest_grid;
				}
				source->squares[grid.y][grid.x].obj = NULL;
			}

			/* Traps */
			if (square(source, grid)->trap) {
				struct trap *trap = square(source, grid)->trap;
				dest->squares[dest_grid.y][dest_grid.x].trap = trap;

				/* Traverse the trap list */
				while (trap) {
					/* Adjust location */
					trap->grid = dest_grid;
					trap = trap->next;
				}
				source->squares[grid.y][grid.x].trap = NULL;
			}

			/* Monsters */
			if (mon) {
				dest->squares[dest_grid.y][dest_grid.x].mon = mon->midx;
				source->squares[grid.y][grid.x].mon = 0;
				mon->grid = dest_grid;
				mon->place = idx;
				flow_free(source, &mon->flow);
				flow_new(dest, &mon->flow);
			}

			/* Player */
			if ((square(source, grid)->mon == -1) && player_here) {
				dest->squares[dest_grid.y][dest_grid.x].mon = -1;
				source->squares[dest_grid.y][dest_grid.x].mon = 0;
				p->grid = dest_grid;
			}
		}
	}
}

/**
 * Add the object list from one chunk to another.
 * This assumes the objects have already been copied by chunk_copy_grid()
 */
static void chunk_copy_objects_add(struct player *p, struct chunk *source,
								   struct chunk *p_source, struct chunk *dest,
								   struct chunk *p_dest)
{
	int i, source_max = 0, dest_max = 0, source_extra = 0, dest_extra = 0;
	int old_max = dest->obj_max;

	/* Count */
	for (i = 0; i <= dest->obj_max; i++) {
		if (dest->objects[i]) {
			dest_max = i;
		} else if (p_dest && p_dest->objects[i]) {
			dest_extra++;
		}
	}
	for (i = 0; i <= source->obj_max; i++) {
		if (source->objects[i]) {
			source_max = i;
		} else if (p_source && p_source->objects[i]) {
			source_extra++;
		}
	}

	/* Extend if needed */
	while (dest_max + source_max + dest_extra + source_extra > dest->obj_max) {
		dest->obj_max += OBJECT_LIST_INCR;
	}
	if (dest->obj_max > old_max) {
		dest->objects = mem_realloc(dest->objects, dest->obj_max
									* sizeof(struct object*));
		if (p_dest) p_dest->objects = mem_realloc(p_dest->objects, dest->obj_max
												  * sizeof(struct object*));
	}
	for (i = old_max + 1; i <= dest->obj_max; i++) {
		dest->objects[i] = NULL;
		if (p_dest) p_dest->objects[i] = NULL;
	}

	/* Copy over */
	dest_max += dest_extra + 1;
	for (i = 0; i <= dest->obj_max; i++) {
		if (source->objects[i]) {
			dest->objects[dest_max] = source->objects[i];
			dest->objects[dest_max]->oidx = dest_max;
			if (p_source && p_source->objects[i]) {
				p_dest->objects[dest_max] = p_source->objects[i];
				p_dest->objects[dest_max]->oidx = dest_max;
			}
			dest_max++;
			source->objects[i] = NULL;
		} else if (p_source && p_source->objects[i]) {
			p_dest->objects[dest_max] = p_source->objects[i];
			p_dest->objects[dest_max]->oidx = dest_max;
			dest_max++;
			p_source->objects[i] = NULL;
		}
	}
	source->obj_max = 1;
	if (p_source) p_source->obj_max = 1;
}

/**
 * Write the object list from one chunk to a fresh chunk
 * This assumes the objects have already been copied by chunk_copy_grid()
 */
static void chunk_copy_objects_split(struct player *p, struct chunk *source,
									 struct chunk *p_source, struct chunk *dest,
									 struct chunk *p_dest, int height,
									 int width, struct loc dest_top_left)
{
	struct loc grid;
	int count = 0, extra = 0;

	/* Count floor objects */
	for (grid.y = dest_top_left.y; grid.y < height; grid.y++) {
		for (grid.x = dest_top_left.x; grid.x < width; grid.x++) {
			struct object *obj = square_object(dest, grid);
			while (obj) {
				count++;
				if (obj) obj = obj->next;
			}
		}
	}

	/* Reduce if possible */
	dest->obj_max = source->obj_max;
	while (dest->obj_max - count > OBJECT_LIST_INCR) {
		dest->obj_max -= OBJECT_LIST_INCR;
	}
	dest->objects = mem_zalloc((dest->obj_max + 1) * sizeof(struct object*));

	/* List/delist objects, keeping actual and known versions aligned */
	count = 0;
	for (grid.y = dest_top_left.y; grid.y < height; grid.y++) {
		for (grid.x = dest_top_left.x; grid.x < width; grid.x++) {
			struct object *obj = square_object(dest, grid);
			struct object *p_obj = square_object(p_dest, grid);
			struct monster *mon = square_monster(dest, grid);

			/* Keep a list of known objects for reference */
			int i, j, num = 0;
			int *known = mem_zalloc(z_info->floor_size * sizeof(int));
			while (p_obj) {
				known[num] = p_obj->oidx;
				num++;
				p_obj = p_obj->next;
			}
			for (i = num; i < z_info->floor_size; i++) {
				known[i] = -1;
			}

			/* Move floor objects, pairing with known objects as needed */
			while (obj) {
				int oidx = obj->oidx;
				count++;
				/* Find the known object if it is here */
				p_obj = square_object(p_dest, grid);
				while (p_obj) {
					if (p_obj->oidx == obj->oidx) break;
					p_obj = p_obj->next;
				}

				/* Relabel */
				dest->objects[count] = obj;
				dest->objects[count]->oidx = count;
				source->objects[oidx] = NULL;

				/* Keep track of known objects */
				if (p_obj) {
					/* Remove it from the list */
					for (i = 0; i < num; i++) {
						if (known[i] == p_obj->oidx) {
							for (j = i; j < num - 1; j++) {
								known[j] = known[j + 1];
							}
							known[num - 1] = -1;
							num--;
							break;
						}
					}

					/* Relabel */
					p_dest->objects[count] = p_obj;
					p_dest->objects[count]->oidx = count;
					p_source->objects[oidx] = NULL;
					assert(p_obj == obj->known);
				}
				obj = obj->next;
			}

			/* Move monster objects */
			if (mon) {
				/* Held objects */
				obj = mon->held_obj;
				while (obj) {
					count++;
					p_obj = p_source->objects[obj->oidx];

					/* Relabel */
					dest->objects[count] = obj;
					dest->objects[count]->oidx = count;
					source->objects[obj->oidx] = NULL;
					if (p_obj) {
						p_dest->objects[count] = p_obj;
						p_dest->objects[count]->oidx = count;
						p_source->objects[p_obj->oidx] = NULL;
						assert(p_obj == obj->known);
					}
					obj = obj->next;
				}
			}


			/* Now see if there are any left over known objects */
			for (i = 0; i < num; i++) {
				p_obj = square_object(p_dest, grid);
				while (p_obj) {
					if (known[i] == p_obj->oidx) break;
					p_obj = p_obj->next;
				}
				assert(p_obj);

				/* Extend the object list if needed */
				count++;
				extra++;
				if (extra >= dest->obj_max - count) {
					dest->obj_max += OBJECT_LIST_INCR;
					dest->objects = mem_realloc(dest->objects,
												(dest->obj_max + 1)
												* sizeof(struct object*));
				}
				p_dest->objects[count] = p_obj;
				p_dest->objects[count]->oidx = count;
				p_source->objects[p_obj->oidx] = NULL;
			}
			mem_free(known);
		}
	}
}

/**
 * Write a chunk, transformed, to a given offset in another chunk.
 *
 * This function assumes that it is being called at level generation, when
 * there has been no interaction between the player and the level, monsters
 * have not been activated, all monsters are in only one group, and objects
 * are in their original positions.
 *
 * \param dest the chunk where the copy is going
 * \param p is the player; if the player is in the chunk being copied, the
 * player's position will be updated to be the player's location in the
 * destination
 * \param source the chunk being copied
 * \param y0 transformation parameters  - see symmetry_transform()
 * \param x0 transformation parameters  - see symmetry_transform()
 * \param rotate transformation parameters  - see symmetry_transform()
 * \param reflect transformation parameters  - see symmetry_transform()
 * \return success - fails if the copy would not fit in the destination chunk
 */
bool chunk_copy(struct chunk *dest, struct player *p, struct chunk *source,
		int y0, int x0, int rotate, bool reflect)
{
	int i, h = source->height, w = source->width;

	/* Check bounds */
	if (rotate % 1) {
		if ((w + y0 > dest->height) || (h + x0 > dest->width))
			return false;
	} else {
		if ((h + y0 > dest->height) || (w + x0 > dest->width))
			return false;
	}

	chunk_copy_grid(p, source, dest, source->height, source->width,
					loc(0, 0), loc(x0, y0), CHUNK_TEMP, rotate, reflect, true);
	chunk_copy_objects_add(p, source, NULL, dest, NULL);
	chunk_validate_objects(dest);
	object_lists_check_integrity(dest, NULL);

	/* Feature counts */
	for (i = 0; i < FEAT_MAX + 1; i++) {
		dest->feat_count[i] += source->feat_count[i];
	}

	return true;
}

/**
 * Read a chunk from the chunk list and put it back into the current playing
 * area
 */
void chunk_read(struct player *p, int idx, int y_coord, int x_coord)
{
	int i, y0 = y_coord * CHUNK_SIDE, x0 = x_coord * CHUNK_SIDE;
	struct chunk *chunk = chunk_list[idx].chunk;
	struct chunk *p_chunk = chunk_list[idx].p_chunk;

	/* Restore the monsters */
	restore_monsters(idx, turn - chunk_list[idx].turn);

	/* Copy everything across */
	chunk_copy_grid(p, chunk, cave, CHUNK_SIDE, CHUNK_SIDE,
					loc(0, 0), loc(x0, y0), CHUNK_CUR, 0, false, false);
	chunk_copy_grid(p, p_chunk, p->cave, CHUNK_SIDE, CHUNK_SIDE,
					loc(0, 0), loc(x0, y0), CHUNK_CUR, 0, false, false);
	chunk_copy_objects_add(p, chunk, p_chunk, cave, p->cave);
	chunk_validate_objects(cave);
	chunk_validate_objects(p->cave);
	object_lists_check_integrity(cave, p->cave);

	/* Feature counts */
	for (i = 0; i < FEAT_MAX + 1; i++) {
		cave->feat_count[i] += chunk->feat_count[i];
	}

	/* Reset the turn */
	chunk_list[idx].turn = turn;

	/* Wipe it */
	chunk_wipe(chunk);
	chunk_wipe(p_chunk);
	chunk_list[idx].chunk = NULL;
	chunk_list[idx].p_chunk = NULL;
}

/**
 * Write a pair of chunks to memory and record pointers to them
 */
static void chunk_write(struct player *p, int idx, int y_coord, int x_coord,
						struct chunk **chunk, struct chunk **p_chunk)
{
	struct loc from = loc(x_coord * CHUNK_SIDE, y_coord * CHUNK_SIDE);
	struct chunk *new = chunk_new(CHUNK_SIDE, CHUNK_SIDE);
	struct chunk *p_new = chunk_new(CHUNK_SIDE, CHUNK_SIDE);

	chunk_copy_grid(p, cave, new, CHUNK_SIDE, CHUNK_SIDE,
					from, loc(0, 0), idx, 0, false, false);
	chunk_copy_grid(p, p->cave, p_new, CHUNK_SIDE, CHUNK_SIDE,
					from, loc(0, 0), idx, 0, false, false);
	chunk_copy_objects_split(p, cave, p->cave, new, p_new, CHUNK_SIDE,
							 CHUNK_SIDE, loc(0, 0));
	chunk_validate_objects(new);
	chunk_validate_objects(p_new);
	object_lists_check_integrity(new, p_new);

	*chunk = new;
	*p_chunk = p_new;
}

/**
 * Validate that the chunk contains no NULL objects.
 * Only checks for nonzero tval.
 * \param c is the chunk to validate.
 */

void chunk_validate_objects(struct chunk *c) {
	int x, y;
	struct object *obj;

	for (y = 0; y < c->height; y++) {
		for (x = 0; x < c->width; x++) {
			struct loc grid = loc(x, y);
			for (obj = square_object(c, grid); obj; obj = obj->next) {
				assert(obj->tval != 0);
				assert(c->objects[obj->oidx] == obj);
			}
			if (square(c, grid)->mon > 0) {
				struct monster *mon = square_monster(c, grid);
				if (mon && mon->held_obj) {
					for (obj = mon->held_obj; obj; obj = obj->next) {
						assert(obj->tval != 0);
						//assert(c->objects[obj->oidx] == obj);
					}
				}
			}
		}
	}
}

/**
 * ------------------------------------------------------------------------
 * Chunk placement utilities
 *
 * Note that offsets are from the current chunk, indexed -1, 0, 1 in the
 * x and y directions from the top left, so for example the chunk down and to
 * the right of the current chunk has x offset 1, y offset 1 (keypad 3).
 * ------------------------------------------------------------------------ */
/**
 * Translate from offsets to adjacent index.  0 is up, 10 is down, 1-9 are 
 * the keypad directions
 */
int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset)
{
	if (z_offset == -1) {
		return DIR_UP;
	} else if (z_offset == 1) {
		return DIR_DOWN;
	} else if ((y_offset >= -1) && (y_offset <= 1) &&
			   (x_offset >= -1) && (x_offset <= 1)) {
		return (5 - 3 * y_offset + x_offset);
	} else {
		return -1;
	}
}

/**
 * Translate from adjacent index to offsets
 */
static void chunk_adjacent_to_offset(int adjacent, int *z_off, int *y_off,
									 int *x_off)
{
	if (adjacent == DIR_UP) {
		*z_off = -1;
		*y_off = 0;
		*x_off = 0;
	} else if (adjacent == DIR_DOWN) {
		*z_off = 1;
		*y_off = 0;
		*x_off = 0;
	} else {
		*z_off = 0;
		*y_off = 1 - ((adjacent - 1) / 3);
		*x_off = ((adjacent - 1) % 3) - 1;
	}
}

/**
 * Translate place in current surface arena into a chunk_list index
 */
static int chunk_get_idx(struct player *p, int y_coord, int x_coord)
{
	int y_off = ARENA_CHUNKS / 2, x_off = ARENA_CHUNKS / 2;
	int idx = p->place;

	/* Move north or south */
	if (y_coord < y_off) {
		while (y_coord < y_off) {
			idx = chunk_list[idx].adjacent[DIR_N];
			y_off--;
			if (idx == MAX_CHUNKS) return idx;
		}
	} else if (y_coord > y_off) {
		while (y_coord > y_off) {
			idx = chunk_list[idx].adjacent[DIR_S];
			y_off++;
			if (idx == MAX_CHUNKS) return idx;
		}
	}

	/* Move west or east */
	if (x_coord < x_off) {
		while (x_coord < x_off) {
			idx = chunk_list[idx].adjacent[DIR_W];
			x_off--;
			if (idx == MAX_CHUNKS) return idx;
		}
	} else if (x_coord > x_off) {
		while (x_coord > x_off) {
			idx = chunk_list[idx].adjacent[DIR_E];
			x_off++;
			if (idx == MAX_CHUNKS) return idx;
		}
	}

	return idx;
}

/**
 * Find the region a set of coordinates is in - dungeons are treated as part
 * of the region they are directly below
 */
int find_region(int y_pos, int x_pos)
{
	int i;

	for (i = 1; i < z_info->region_max; i++) {
		struct world_region *region = &region_info[i];
		size_t entry;

		if ((y_pos / CPM < region->y_offset) ||
			(y_pos / CPM >= region->y_offset + region->height)) {
			continue;
		}

		if ((x_pos / CPM < region->x_offset) ||
			(x_pos / CPM >= region->x_offset + region->width)) {
			continue;
		}

		entry =	region->width * ((y_pos / CPM) - region->y_offset) +
			(x_pos / CPM) - region->x_offset;
		if ((entry >= strlen(region->text)) || (region->text[entry] == ' ')) {
			continue;
		}

		break;
	}

	return i;
}

/**
 * Get the location data for a chunk offset relative to another chunk.
 * \param ref the chunk reference which will be altered subject to the offsets
 * \param z_offset offset in the z direction
 * \param y_offset offset in the y direction
 * \param x_offset offset in the x direction
 */
void chunk_offset_data(struct chunk_ref *ref, int z_offset, int y_offset,
						 int x_offset)
{
	if (((ref->y_pos == 0) && (y_offset < 0)) ||
		((ref->y_pos >= CPM * MAX_Y_REGION - 1) && (y_offset > 0)) ||
		((ref->x_pos == 0) && (x_offset < 0)) ||
		((ref->x_pos >= CPM * MAX_X_REGION - 1) && (x_offset > 0))) {
		ref->region = 0;
	} else {
		int lower, upper;
		bool find;
		ref->z_pos += z_offset;
		ref->y_pos += y_offset;
		ref->x_pos += x_offset;
		if (z_offset == 0) {
			ref->region = find_region(ref->y_pos, ref->x_pos);
		}
		find = gen_loc_find(ref->x_pos, ref->y_pos, ref->z_pos, &lower, &upper);
		if (find) {
			ref->gen_loc_idx = upper;
		}
	}
}

/**
 * Copy a connector
 */
static void connector_copy(struct connector *dest, struct connector *source)
{
	dest->grid.y = source->grid.y;
	dest->grid.x = source->grid.x;
	dest->feat = source->feat;
	sqinfo_copy(dest->info, source->info);
	dest->type = source->type;
	dest->next = NULL;
}

/**
 * Add a connector to the list
 */
static void connector_add(struct connector **first, struct connector **latest,
						  struct connector **current)
{
	if (*first) {
		(*latest)->next = *current;
		*latest = (*latest)->next;
	} else {
		*first = *current;
		*latest = *first;
	}
	(*latest)->next = NULL;
}

/**
 * Free a linked list of connectiors.
 */
void connectors_free(struct connector *join)
{
	while (join) {
		struct connector *current = join;

		join = current->next;
		mem_free(current);
	}
}

/**
 * ------------------------------------------------------------------------
 * Chunk list operations
 * ------------------------------------------------------------------------ */
/**
 * Allocate the chunk list
 */
void chunk_list_init(void)
{
	chunk_list = mem_zalloc(MAX_CHUNKS * sizeof(struct chunk_ref));
}

/**
 * Clean up the chunk list
 */
void chunk_list_cleanup(void)
{
	int i;

	/* Free the chunk list */
	for (i = 0; i < MAX_CHUNKS; i++) {
		struct chunk_ref *ref = &chunk_list[i];
		if (ref->chunk) chunk_wipe(ref->chunk);
		ref->chunk = NULL;
		if (ref->p_chunk) chunk_wipe(ref->p_chunk);
		ref->p_chunk = NULL;
	}
	mem_free(chunk_list);
	chunk_list = NULL;
}

/**
 * Delete a chunk ref from the chunk_list
 */
static void chunk_delete(int idx)
{
	int i, j;
	struct chunk_ref *ref = &chunk_list[idx];

	ref->turn = 0;
	ref->region = 0;
	ref->z_pos = 0;
	ref->y_pos = 0;
	ref->x_pos = 0;
	ref->gen_loc_idx = 0;
	if (ref->chunk) {
		chunk_wipe(ref->chunk);
		ref->chunk = NULL;
	}
	if (ref->p_chunk) {
		chunk_wipe(ref->p_chunk);
		ref->p_chunk = NULL;
	}
	for (i = 0; i < DIR_MAX; i++) {
		ref->adjacent[i] = MAX_CHUNKS;
	}

	/* Repair chunks */
	for (i = 0; i < chunk_max; i++) {
		/* Get the chunk */
		struct chunk_ref *chunk = &chunk_list[i];

		/* Skip dead chunks */
		if (!chunk->region) continue;

		/* Repair adjacencies */
		for (j = 0; j < DIR_MAX; j++) {
			if (chunk->adjacent[j] == idx) {
				chunk->adjacent[j] = MAX_CHUNKS;
			}
		}
	}
}

/**
 * Delete all the same age dungeon chunks (wiping out all of a level
 * if some goes)
 */
static void chunk_delete_level(int age)
{
	int i;

	for (i = 0; i < MAX_CHUNKS; i++) {
		if (chunk_list[i].turn == turn) {
			chunk_delete(i);

			/* Decrement the counter, and the maximum if necessary */
			chunk_cnt--;
			if (i == chunk_max) {
				chunk_max--;
			}
		}
	}
}

/**
 * Find a chunk_ref in chunk_list
 */
int chunk_find(struct chunk_ref ref)
{
	int i;

	/* Search the list */
	for (i = 0; i < MAX_CHUNKS; i++) {
		/* Reject wrong values */
		if (ref.region != chunk_list[i].region) continue;
		if (ref.x_pos != chunk_list[i].x_pos) continue;
		if (ref.y_pos != chunk_list[i].y_pos) continue;
		if (ref.z_pos != chunk_list[i].z_pos) continue;

		break;
	}

	return i;
}

/**
 * Check and repair all the entries in the chunk_list
 */
static void chunk_fix_all(void)
{
	int n, z_off, y_off, x_off, idx;

	for (idx = 0; idx < MAX_CHUNKS; idx++) {
		/* Get the chunk ref */
		struct chunk_ref *ref = &chunk_list[idx];

		/* Remove dead chunks */
		if (!ref->region) {//B need something better, as this is Belegaer
			chunk_delete(idx);
			continue;
		}

		/* Set the index */
		ref->place = idx;

		/* Set adjacencies */
		for (n = 0; n < DIR_MAX; n++) {
			struct chunk_ref ref1 = { 0 };
			int chunk_idx;

			/* Self-reference (not strictly necessary) */
			if (n == DIR_NONE) {
				ref->adjacent[n] = idx;
				continue;
			}

			/* Set to the default */
			ref->adjacent[n] = MAX_CHUNKS;

			/* Get the reference data for the adjacent chunk */
			chunk_adjacent_to_offset(n, &z_off, &y_off, &x_off);
			ref1.z_pos = ref->z_pos;
			ref1.y_pos = ref->y_pos;
			ref1.x_pos = ref->x_pos;
			ref1.region = ref->region;
			chunk_offset_data(&ref1, z_off, y_off, x_off);

			/* Deal with existing chunks */
			chunk_idx = chunk_find(ref1);
			if (chunk_idx < MAX_CHUNKS) {
				ref->adjacent[n] = chunk_idx;
			}
		}
	}
}

/**
 * Store a chunk pair from the current playing area into the chunk list
 */
int chunk_store(int y_coord, int x_coord, uint16_t region, uint16_t z_pos,
				uint16_t y_pos, uint16_t x_pos, uint32_t gen_loc_idx,
				bool write)
{
	int i;
	int max = turn, idx = 0;

	struct chunk_ref ref = { 0 };

	/* Check for an existing one */
	ref.region = region;
	ref.x_pos = x_pos;
	ref.y_pos = y_pos;
	ref.z_pos = z_pos;

	idx = chunk_find(ref);

	/* We need a new slot */
	if (idx == MAX_CHUNKS) {
		/* Too many chunks */
		if (chunk_cnt >= MAX_CHUNKS - 1) {
			/* Find and delete the oldest chunk */
			idx = 0;
			for (i = 0; i < MAX_CHUNKS; i++) {
				if (chunk_list[i].turn < max) {
					max = chunk_list[i].turn;
					idx = i;
				}
			}
			chunk_delete(idx);

			/* Delete whole levels at once */
			if (chunk_list[idx].z_pos > 0) {
				chunk_delete_level(max);
			} else {
				/* Decrement the counter, and the maximum if necessary */
				chunk_cnt--;
				if (idx == chunk_max)
					chunk_max--;
			}
		} else {
			/* Find the next free slot */
			for (idx = 0; idx < chunk_max; idx++) {
				if (!chunk_list[idx].region) {
					break;
				}
			}
		}

		/* Increment the counter, and the maximum if necessary */
		chunk_cnt++;
		assert(chunk_max <= MAX_CHUNKS);
		if (idx == chunk_max)
			chunk_max++;
	}

	/* Set all the values */
	chunk_list[idx].place = idx;

	chunk_list[idx].turn = turn;

	chunk_list[idx].region = region;
	chunk_list[idx].y_pos = y_pos;
	chunk_list[idx].x_pos = x_pos;
	chunk_list[idx].z_pos = z_pos;
	chunk_list[idx].gen_loc_idx = gen_loc_idx;
	chunk_list[idx].adjacent[5] = idx;

	/* Write the chunks */
	if (write) {
		chunk_write(player, idx, y_coord, x_coord, &chunk_list[idx].chunk,
					&chunk_list[idx].p_chunk);
	}

	/* Repair the list */
	chunk_fix_all();

	return idx;
}

/**
 * ------------------------------------------------------------------------
 * Handling of player going from one chunk to the next
 * ------------------------------------------------------------------------ */
/**
 * Generate a chunk
 */
static void chunk_generate(struct chunk *c, struct gen_loc *loc,
						   struct chunk_ref *ref, int y_coord, int x_coord,
						   struct connector *first)
{
	int n, z_pos = ref->z_pos, y_pos = ref->y_pos, x_pos = ref->x_pos;

	/* Check for landmarks */
	for (n = 0; n < z_info->landmark_max; n++) {
		struct landmark *landmark = &landmark_info[n];

		/* Must satisfy all the conditions */
		if (landmark->map_z != z_pos)
			continue;
		if (landmark->map_y > y_pos)
			continue;
		if (landmark->map_y + landmark->height <= y_pos)
			continue;
		if (landmark->map_x > x_pos)
			continue;
		if (landmark->map_x + landmark->width <= x_pos)
			continue;

		break;
	}

	/* Build the landmark... */
	if (n < z_info->landmark_max) {
		build_landmark(c, n, y_pos, x_pos, y_coord, x_coord);
	} else {
		/* or set the RNG to give reproducible results... */
		Rand_quick = true;
		loc->seed = randint0(0x10000000);
		Rand_value = loc->seed;

		/* ...and generate the chunk */
		surface_gen(c, ref, y_coord, x_coord, first);
	}
}

/**
 * Generate a chunk on the surface
 */
int chunk_fill(struct chunk *c, struct chunk_ref *ref, int y_coord, int x_coord)
{
	int n, z_off, y_off, x_off, idx;
	int z_pos = ref->z_pos, y_pos = ref->y_pos, x_pos = ref->x_pos;
	int lower, upper, region;
	bool reload;
	struct gen_loc *location;
	struct connector east[CHUNK_SIDE] = {{{0}, 0, {0}, 0, 0}};
	struct connector west[CHUNK_SIDE] = {{{0}, 0, {0}, 0, 0}};
	struct connector north[CHUNK_SIDE] = {{{0}, 0, {0}, 0, 0}};
	struct connector south[CHUNK_SIDE] = {{{0}, 0, {0}, 0, 0}};
	struct connector vertical[CHUNK_SIDE][CHUNK_SIDE] = {{{{0}, 0, {0}, 0, 0}}};
	struct connector *first = NULL;
	struct connector *latest = NULL;

	/* If underground, return */
	if (z_pos) return MAX_CHUNKS;

	/* See if we've been generated before */
	reload = gen_loc_find(x_pos, y_pos, z_pos, &lower, &upper);

	/* Access the old place in the gen_loc_list, or make the new one */
	if (reload) {
		location = &gen_loc_list[upper];
	} else {
		gen_loc_make(x_pos, y_pos, z_pos, upper);
		location = &gen_loc_list[upper];
	}

	/* Store the chunk reference */
	region = find_region(y_pos, x_pos);
	idx = chunk_store(0, 0, region, z_pos, y_pos, x_pos, upper, false);

	/* Get adjacent data */
	for (n = 0; n < DIR_MAX; n++) {
		struct chunk_ref ref1 = { 0 };

		/* Get the reference data for the adjacent chunk */
		chunk_adjacent_to_offset(n, &z_off, &y_off, &x_off);
		ref1.x_pos = x_pos;
		ref1.y_pos = y_pos;
		ref1.z_pos = z_pos;
		chunk_offset_data(&ref1, z_off, y_off, x_off);

		/* Look for old chunks and get connectors */
		if ((x_off == 0) || (y_off == 0)) {
			int low, high;
			bool exists = gen_loc_find(ref1.x_pos, ref1.y_pos, ref1.z_pos,
									   &low, &high);
			struct gen_loc *loc = NULL;
			struct connector *start = NULL;
			struct connector *current = NULL;

			if (exists) {
				/* Get the location */
				loc = &gen_loc_list[low];
				first = NULL;

				/* Find connectors */
				switch (n) {
					case DIR_UP:
					{
						for (start = loc->join; start; start = start->next) {
							current = &vertical[start->grid.y][start->grid.x];

							if (feat_is_downstair(start->feat) ||
								feat_is_chasm(start->feat)) {
								connector_copy(current, start);
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_S:
					{
						for (start = loc->join; start; start = start->next) {
							current = &south[start->grid.x];

							if (start->grid.y == 0) {
								connector_copy(current, start);
								current->grid.y = CHUNK_SIDE;
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_W:
					{
						for (start = loc->join; start; start = start->next) {
							current = &west[start->grid.y];

							if (start->grid.x == CHUNK_SIDE - 1) {
								connector_copy(current, start);
								current->grid.x = -1;
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_E:
					{
						for (start = loc->join; start; start = start->next) {
							current = &east[start->grid.y];

							if (start->grid.x == 0) {
								connector_copy(current, start);
								current->grid.x = CHUNK_SIDE;
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_N:
					{
						for (start = loc->join; start; start = start->next) {
							current = &north[start->grid.x];

							if (start->grid.y == CHUNK_SIDE - 1) {
								connector_copy(current, start);
								current->grid.y = -1;
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_DOWN:
					{
						for (start = loc->join; start; start = start->next) {
							current = &vertical[start->grid.y][start->grid.x];

							if (feat_is_upstair(start->feat)) {
								connector_copy(current, start);
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
				}
			}
		}
	}

	/* Place chunk */
	chunk_generate(c, location, ref, y_coord, x_coord, first);

	/* Do terrain changes */
	if (reload) {
		struct terrain_change *change;

		/* Change any terrain that has changed since first generation */
		for (change = location->change; change; change = change->next) {
			int y = y_coord * CHUNK_SIDE + change->grid.y;
			int x = x_coord * CHUNK_SIDE + change->grid.x;

			square_set_feat(c, loc(x, y), change->feat);
		}
	} else {
		/* Write connectors.
		 * Note that if a connector was loaded and used in generation from
		 * an adjacent chunk already, then we don't write one for that
		 * connection point, as the first generated chunk at a border affects
		 * the second and not vice versa. */
		int y, x;
		int y0 = CHUNK_SIDE * y_coord;
		int x0 = CHUNK_SIDE * x_coord;

		/* South, north and vertical */
		for (x = 0; x < CHUNK_SIDE; x++) {
			if (south[x].feat == 0) {
				struct loc grid = loc(x0 + x, y0 + CHUNK_SIDE - 1);
				struct connector *new = mem_zalloc(sizeof *new);
				new->grid.y = CHUNK_SIDE - 1;
				new->grid.x = x;
				new->feat = square(c, grid)->feat;
				sqinfo_copy(new->info, square(c, grid)->info);
				new->type = location->type;
				new->next = location->join;
				location->join = new;
			}
			if (north[x].feat == 0) {
				struct loc grid = loc(x0 + x, y0);
				struct connector *new = mem_zalloc(sizeof *new);
				new->grid.y = 0;
				new->grid.x = x;
				new->feat = square(c, grid)->feat;
				sqinfo_copy(new->info, square(c, grid)->info);
				new->type = location->type;
				new->next = location->join;
				location->join = new;
			}
			for (y = 0; y < CHUNK_SIDE; y++) {
				uint8_t feat = vertical[y][x].feat;
				struct loc grid = loc(x0 + x, y0 + y);
				if (feat == 0) {
					if (feat_is_stair(square(c, grid)->feat) ||
						feat_is_chasm(square(c, grid)->feat)) {
						struct connector *new = mem_zalloc(sizeof *new);
						new->grid.y = y;
						new->grid.x = x;
						new->feat = square(c, grid)->feat;
						sqinfo_copy(new->info, square(c, grid)->info);
						new->type = location->type;
						new->next = location->join;
						location->join = new;
					}
				}
			}
		}

		/* East and west */
		for (y = 0; y < CHUNK_SIDE; y++) {
			if (west[y].feat == 0) {
				struct loc grid = loc(x0, y0 + y);
				struct connector *new = mem_zalloc(sizeof *new);
				new->grid.y = y;
				new->grid.x = 0;
				new->feat = square(c, grid)->feat;
				sqinfo_copy(new->info, square(c, grid)->info);
				new->type = location->type;
				new->next = location->join;
				location->join = new;
			}
			if (east[y].feat == 0) {
				struct loc grid = loc(x0 + CHUNK_SIDE - 1, y0 + y);
				struct connector *new = mem_zalloc(sizeof *new);
				new->grid.y = y;
				new->grid.x = CHUNK_SIDE - 1;
				new->feat = square(c, grid)->feat;
				sqinfo_copy(new->info, square(c, grid)->info);
				new->type = location->type;
				new->next = location->join;
				location->join = new;
			}
		}
	}
	return idx;
}

/**
 * Deal with re-aligning the playing arena on the same z-level
 *
 * Used for walking off the edge of a chunk, currently only for the surface
 */
static void arena_realign(struct player *p, int y_offset, int x_offset)
{
	int i, x, y;
	bool chunk_exists[ARENA_CHUNKS][ARENA_CHUNKS] = { 0 };
	int new_dir;
	struct chunk *new = chunk_new(ARENA_SIDE, ARENA_SIDE);
	struct chunk *p_new = chunk_new(ARENA_SIDE, ARENA_SIDE);
	struct loc src_top_left;
	struct loc dest_top_left;
	int height, width;

	/* Get the direction of the new centre chunk */
	new_dir = chunk_offset_to_adjacent(0, y_offset, x_offset);
	assert(new_dir != -1);

	/* Unload chunks no longer required */
	for (y = 0; y < ARENA_CHUNKS; y++) {
		for (x = 0; x < ARENA_CHUNKS; x++) {
			struct chunk_ref *ref = NULL;
			int chunk_idx;
			int new_y = y - y_offset;
			int new_x = x - x_offset;

			/* Keep chunks close enough to the new centre */
			if ((new_x >= 0) && (new_x < ARENA_CHUNKS) &&
				(new_y >= 0) && (new_y < ARENA_CHUNKS)) {

				/* Record this one as existing */
				chunk_exists[new_y][new_x] = true;
				continue;
			}

			/* Access the chunk's placeholder in chunk_list */
			chunk_idx = chunk_get_idx(p, y, x);
			if (chunk_idx == MAX_CHUNKS) continue;
			ref = &chunk_list[chunk_idx];

			/* Store it */
			(void) chunk_store(y, x, ref->region, ref->z_pos, ref->y_pos,
							   ref->x_pos, ref->gen_loc_idx, true);

			/* Feature counts */
			for (i = 0; i < FEAT_MAX + 1; i++) {
				cave->feat_count[i] -= ref->chunk->feat_count[i];
			}
		}
	}

	/* Re-align current playing arena */
	if (y_offset == -1) {
		src_top_left.y = 0;
		dest_top_left.y = CHUNK_SIDE;
		height = (ARENA_CHUNKS - 1) * CHUNK_SIDE;
	} else if (y_offset == 0) {
		src_top_left.y = 0;
		dest_top_left.y = 0;
		height = ARENA_CHUNKS * CHUNK_SIDE;
	} else if (y_offset == 1) {
		src_top_left.y = CHUNK_SIDE;
		dest_top_left.y = 0;
		height = (ARENA_CHUNKS - 1) * CHUNK_SIDE;
	}
	if (x_offset == -1) {
		src_top_left.x = 0;
		dest_top_left.x = CHUNK_SIDE;
		width = (ARENA_CHUNKS - 1) * CHUNK_SIDE;
	} else if (x_offset == 0) {
		src_top_left.x = 0;
		dest_top_left.x = 0;
		width = ARENA_CHUNKS * CHUNK_SIDE;
	} else if (x_offset == 1) {
		src_top_left.x = CHUNK_SIDE;
		dest_top_left.x = 0;
		width = (ARENA_CHUNKS - 1) * CHUNK_SIDE;
	}
	chunk_copy_grid(p, cave, new, height, width, src_top_left,
					dest_top_left, CHUNK_CUR, 0, false, true);
	chunk_copy_grid(p, p->cave, p_new, height, width,	src_top_left,
					dest_top_left, CHUNK_CUR, 0, false, true);
	chunk_copy_objects_split(p, cave, p->cave, new, p_new, height,
							 width, dest_top_left);
	chunk_validate_objects(new);
	chunk_validate_objects(p_new);
	object_lists_check_integrity(new, p_new);

	/* Feature counts */
	for (i = 0; i < FEAT_MAX + 1; i++) {
		new->feat_count[i] = cave->feat_count[i];
	}

	chunk_wipe(cave);
	chunk_wipe(p->cave);
	cave = new;
	p->cave = p_new;

	/* Player has moved chunks */
	p->last_place = p->place;
	p->place = chunk_list[p->place].adjacent[new_dir];

	/* Reload or generate chunks to fill the playing area. 
	 * Note that chunk generation needs to write the adjacent[] entries */
	for (y = 0; y < ARENA_CHUNKS; y++) {
		for (x = 0; x < ARENA_CHUNKS; x++) {
			int chunk_idx;
			struct chunk_ref ref = { 0 };

			/* Already in the current playing area */
			if (chunk_exists[y][x]) continue;

			/* Load it if it is in the chunk list */
			chunk_idx = chunk_get_idx(p, y, x);
			if ((chunk_idx != MAX_CHUNKS) && chunk_list[chunk_idx].chunk) {
				chunk_read(p, chunk_idx, y, x);
			} else {
				/* Otherwise generate a new one */
				ref.y_pos = chunk_list[p->place].y_pos + y
					- ARENA_CHUNKS / 2;
				ref.x_pos = chunk_list[p->place].x_pos + x
					- ARENA_CHUNKS / 2;
				(void) chunk_fill(cave, &ref, y, x);
			}
		}
	}
	set_monster_place_current();
	cave_illuminate(cave, is_daytime());
	update_view(cave, p);
}

/**
 * Get the centre chunk from the playing arena
 * This is necessary in dungeons because the player is not kept central
 */
int chunk_get_centre(void)
{
	int idx = -1;
	int max_y = 0, max_x = 0;
	int min_y = CPM * MAX_Y_REGION, min_x = CPM * MAX_X_REGION;

	/* Find the min x and y positions */
	for (idx = 0; idx < MAX_CHUNKS; idx++) {
		struct chunk_ref ref = chunk_list[idx];
		if (!ref.chunk) {
			if (ref.y_pos > max_y) max_y = ref.y_pos;
			if (ref.x_pos > max_x) max_x = ref.x_pos;
			if (ref.y_pos < min_y) min_y = ref.y_pos;
			if (ref.x_pos < min_x) min_x = ref.x_pos;
			if ((max_y - min_y == ARENA_CHUNKS - 1) &&
				(max_x - min_x == ARENA_CHUNKS - 1)) break;
		}
	}

	/* Find the centre */
	for (idx = 0; idx < MAX_CHUNKS; idx++) {
		struct chunk_ref ref = chunk_list[idx];
		if (!ref.chunk && (ref.y_pos == min_y + ARENA_CHUNKS / 2) &&
			(ref.x_pos == min_x + ARENA_CHUNKS / 2))
			return idx;
	}

	/* Fail */
	idx = -1;

	return idx;
}

/**
 * Deal with moving the playing arena to a different z-level
 *
 * Used for stairs, teleport level, falling
 */
static void level_change(struct player *p, int z_offset)
{
	int y, x, new_idx;
	int centre = chunk_get_centre();

	/* Unload chunks no longer required */
	for (y = -ARENA_CHUNKS / 2; y <= ARENA_CHUNKS / 2; y++) {
		for (x = -ARENA_CHUNKS / 2; x <= ARENA_CHUNKS / 2; x++) {
			struct chunk_ref ref = chunk_list[centre];

			/* Get the location data */
			ref.z_pos = p->depth;
			chunk_offset_data(&ref, 0, y, x);

			/* Store it */
			(void) chunk_store(y + ARENA_CHUNKS / 2, x + ARENA_CHUNKS / 2,
							   ref.region, ref.z_pos, ref.y_pos,
							   ref.x_pos, ref.gen_loc_idx, true);
		}
	}

	/* Get the new chunk */
	new_idx = chunk_offset_to_adjacent(z_offset, 0, 0);

	/* Set the chunk (possibly invalid) */
	p->last_place = p->place;
	p->place = chunk_list[p->place].adjacent[new_idx];

	/* Leaving, make new level */
	p->upkeep->generate_level = true;

	/* Save the game when we arrive on the new level. */
	p->upkeep->autosave = true;

	/* Set depth */
	p->depth += z_offset;
}

/**
 * Handle the player moving from one chunk to an adjacent one.  This function
 * needs to handle moving in the eight surface directions, plus up or down
 * one level, and the consequent moving of chunks to and from chunk_list.
 */
void chunk_change(struct player *p, int z_offset, int y_offset, int x_offset)
{
	if (z_offset) {
		level_change(p, z_offset);
	} else {
		arena_realign(p, y_offset, x_offset);
	}
}
