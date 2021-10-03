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
#include "obj-pile.h"
#include "obj-util.h"
#include "trap.h"

#define CHUNK_LIST_INCR 10
#define CHUNK_SIDE 22
#define MAX_CHUNKS 256
struct chunk **old_chunk_list;     /**< list of pointers to saved chunks */
u16b old_chunk_list_max = 0;      /**< current max actual chunk index */
u16b chunk_max = 1;				/* Number of allocated chunks */
u16b chunk_cnt = 0;				/* Number of live chunks */
u32b gen_loc_cnt = 0;			/* Number of actual generated locations */
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
 * Write the details of a chunk to another
 *
 * \param c chunk being written
 * \return the memory location of the chunk
 */
static void chunk_copy_details(struct player *p, struct chunk *source,
						struct chunk *dest, int height, int width,
						struct loc src_top_left, struct loc dest_top_left,
						int rotate, bool reflect, bool split)
{
	struct loc grid, trans = loc_diff(dest_top_left, src_top_left);
	int i;

	/* Write the location stuff (terrain, objects, traps) */
	for (grid.y = src_top_left.y; grid.y < height; grid.y++) {
		for (grid.x = src_top_left.x; grid.x < width; grid.x++) {
			/* Work out where we're going */
			struct loc dest_grid = grid;
			symmetry_transform(&dest_grid, trans.y, trans.x, height, width,
							   rotate, reflect);

			/* Terrain */
			dest->squares[dest_grid.y][dest_grid.x].feat =
				square(source, grid)->feat;
			sqinfo_copy(square(dest, dest_grid)->info,
						square(source, grid)->info);
			if (split) {
				dest->feat_count[square(source, grid)->feat]++;
			}

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

			/* Player */
			if (square(source, grid)->mon == -1) {
				dest->squares[dest_grid.y][dest_grid.x].mon = -1;
				source->squares[dest_grid.y][dest_grid.x].mon = 0;
				p->grid = dest_grid;
			}
		}
	}

	/* Monsters */
	if (split) {
		int midx = 1;

		/* Make a copy of monster and group arrays for later */
		struct monster *old_mons = mem_zalloc(z_info->level_monster_max
											  * sizeof(struct monster));
		int *leaders = mem_zalloc(z_info->level_monster_max * sizeof(int));
		memcpy(old_mons, source->monsters, z_info->level_monster_max
			   * sizeof(struct monster));
		memcpy(dest->monster_groups, source->monster_groups,
			   z_info->level_monster_max * sizeof(struct monster_group*));
		for (i = 1; i < z_info->level_monster_max; i++) {
			if (source->monster_groups[i]) {
				leaders[i] =
					monster_group_leader_idx(source->monster_groups[i]);
			}
		}

		/* Move the individual monsters */
		for (i = 1; i < source->mon_max; i++) {
			struct monster *source_mon = &source->monsters[i];
			struct monster *dest_mon = &dest->monsters[midx];

			/* Mark old version */
			old_mons[i].grid.y = 0;

			/* Valid monster */
			if (!source_mon->race) continue;
			if ((source_mon->grid.y < src_top_left.y) ||
				(source_mon->grid.y >= src_top_left.y + height)) continue;
			if ((source_mon->grid.x < src_top_left.x) ||
				(source_mon->grid.x >= src_top_left.x + width)) continue;

			/* Copy */
			memcpy(dest_mon, source_mon, sizeof(struct monster));

			/* Adjust monster index and counts */
			dest_mon->midx = midx;
			midx++;
			monster_group_change_index(dest, midx, i);
			dest->mon_cnt++;
			dest->mon_max++;

			/* Move grid */
			symmetry_transform(&dest_mon->grid, trans.y, trans.x, height, width,
							   rotate, reflect);
			dest->squares[dest_mon->grid.y][dest_mon->grid.x].mon
				= dest_mon->midx;

			/* Re-mark old version, add new index */
			old_mons[i].grid.y = dest_mon->grid.y;
			old_mons[i].midx = midx;

			/* Held or mimicked objects */
			if (source_mon->held_obj) {
				struct object *obj;
				dest_mon->held_obj = source_mon->held_obj;
				for (obj = source_mon->held_obj; obj; obj = obj->next) {
					obj->held_m_idx = dest_mon->midx;
				}
				source_mon->held_obj = NULL;
			}
			monster_remove_from_group(source, source_mon);
			memset(source_mon, 0, sizeof(struct monster));
		}
		compact_monsters(source, 0);

		/* Now handle destination groups */
		for (i = 1; i < z_info->level_monster_max; i++) {
			/* For each group, remove everyone left behind but the leader */
			struct mon_group_list_entry *entry, *prev = NULL;
			entry = dest->monster_groups[i]->member_list;
			while (entry) {
				if (old_mons[entry->midx].grid.y == 0) {
					if (entry->midx != leaders[i]) {
						if (prev) {
							prev->next = entry->next;
							mem_free(entry);
							entry = entry->next;
						} else {
							dest->monster_groups[i]->member_list = entry->next;
							entry = entry->next;
						}
					} else {
						prev = entry;
						entry = entry->next;
					}
				} else {
					entry->midx = old_mons[entry->midx].midx;
					prev = entry;
					entry = entry->next;
				}
			}

			/* Now remove the leader if necessary */
			if (old_mons[leaders[i]].grid.y == 0) {
				monster_group_remove_leader(dest, &old_mons[leaders[i]],
											dest->monster_groups[i]);
				entry = dest->monster_groups[i]->member_list;
				prev = NULL;
				while (entry) {
					if (old_mons[entry->midx].grid.y == 0) {
						if (prev) {
							prev->next = entry->next;
							mem_free(entry);
							entry = entry->next;
						} else {
							dest->monster_groups[i]->member_list = entry->next;
							entry = entry->next;
						}
					}
				}
			}
			if (!dest->monster_groups[i]->member_list) {
				monster_group_free(dest, dest->monster_groups[i]);
				dest->monster_groups[i] = NULL;
			}
		}
		monster_groups_verify(dest);
		mem_free(old_mons);
		mem_free(leaders);
	} else {
		int max_group_id = 0;
		int mon_skip = dest->mon_max - 1;

		dest->mon_max += source->mon_max;
		dest->mon_cnt += source->mon_cnt;
		for (i = 1; i < source->mon_max; i++) {
			struct monster *source_mon = &source->monsters[i];
			struct monster *dest_mon = &dest->monsters[mon_skip + i];

			/* Valid monster */
			if (!source_mon->race) continue;

			/* Copy */
			memcpy(dest_mon, source_mon, sizeof(struct monster));

			/* Adjust monster index */
			dest_mon->midx += mon_skip;

			/* Move grid */
			symmetry_transform(&dest_mon->grid, trans.y, trans.x, height, width,
							   rotate, reflect);
			dest->squares[dest_mon->grid.y][dest_mon->grid.x].mon
				= dest_mon->midx;

			/* Held or mimicked objects */
			if (source_mon->held_obj) {
				struct object *obj;
				dest_mon->held_obj = source_mon->held_obj;
				for (obj = source_mon->held_obj; obj; obj = obj->next) {
					obj->held_m_idx = dest_mon->midx;
				}
			}
		}

		/* Find max monster group id */
		for (i = 1; i < z_info->level_monster_max; i++) {
			if (dest->monster_groups[i]) max_group_id = i;
		}

		/* Copy monster groups */
		for (i = 1; i < z_info->level_monster_max - max_group_id; i++) {
			struct monster_group *group = source->monster_groups[i];
			struct mon_group_list_entry *entry;

			/* Copy monster group list */
			dest->monster_groups[i + max_group_id] = source->monster_groups[i];

			/* Adjust monster group indices */
			if (!group) continue;
			entry = group->member_list;
			group->index += max_group_id;
			group->leader += mon_skip;
			while (entry) {
				int idx = entry->midx;
				struct monster *mon = &dest->monsters[mon_skip + idx];
				entry->midx = mon->midx;
				assert(entry->midx == mon_skip + idx);
				mon->group_info.index += max_group_id;
				entry = entry->next;
			}
		}
		monster_groups_verify(dest);
	}

	/* Copy object list */
	if (split) {
		dest->obj_max = source->obj_max;
		dest->objects = mem_zalloc((dest->obj_max + 1) *
								   sizeof(struct object*));

		/* List/delist objects */
		for (grid = dest_top_left; grid.y < height; grid.y++) {
			for (; grid.x < width; grid.x++) {
				struct object *obj = square_object(dest, grid);
				while (obj) {
					dest->objects[obj->oidx] = obj;
					source->objects[obj->oidx] = NULL;
					obj = obj->next;
				}
			}
		}
		object_lists_check_integrity(dest);
	} else {
		dest->objects = mem_realloc(dest->objects,
									(dest->obj_max + source->obj_max + 2)
									* sizeof(struct object*));
		for (i = 0; i <= source->obj_max; i++) {
			dest->objects[dest->obj_max + i] = source->objects[i];
			if (dest->objects[dest->obj_max + i] != NULL)
				dest->objects[dest->obj_max + i]->oidx = dest->obj_max + i;
			source->objects[i] = NULL;
		}
		dest->obj_max += source->obj_max + 1;
		source->obj_max = 1;
		object_lists_check_integrity(dest);
	}

	/* Miscellany */
	if (!split) {
		for (i = 0; i < z_info->f_max + 1; i++) {
			dest->feat_count[i] += source->feat_count[i];
		}
	}
}

/**
 * Write the terrain info of a chunk to memory and return a pointer to it
 *
 * \param c chunk being written
 * \return the memory location of the chunk
 */
struct chunk *old_chunk_write(struct chunk *c)
{
	int x, y;

	struct chunk *new = cave_new(c->height, c->width);

	/* Write the location stuff */
	for (y = 0; y < new->height; y++) {
		for (x = 0; x < new->width; x++) {
			/* Terrain */
			new->squares[y][x].feat = square(c, loc(x, y))->feat;
			sqinfo_copy(square(new, loc(x, y))->info, square(c, loc(x, y))->info);
		}
	}

	return new;
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
	int h = source->height, w = source->width;

	/* Check bounds */
	if (rotate % 1) {
		if ((w + y0 > dest->height) || (h + x0 > dest->width))
			return false;
	} else {
		if ((h + y0 > dest->height) || (w + x0 > dest->width))
			return false;
	}

	chunk_copy_details(p, source, dest, source->height, source->width,
					   loc(0, 0), loc(x0, y0), rotate, reflect, false);

	return true;
}

/**
 * Read a chunk from the chunk list and put it back into the current playing
 * area
 */
static void chunk_read(int idx, int y_offset, int x_offset)
{
	int y0 = y_offset * CHUNK_SIDE;
	int x0 = x_offset * CHUNK_SIDE;
	struct chunk *chunk = chunk_list[idx].chunk;

	chunk_copy_details(player, chunk, cave, CHUNK_SIDE, CHUNK_SIDE,
					   loc(0, 0), loc(x0, y0), 0, false, false);

	/* Reset the turn */
	chunk_list[idx].turn = turn;

	/* Wipe it */
	chunk_wipe(chunk);
	chunk = NULL;
}

/**
 * Write a chunk to memory and return a pointer to it
 */
static struct chunk *chunk_write(int y_offset, int x_offset)
{
	struct loc from = loc(x_offset * CHUNK_SIDE, y_offset * CHUNK_SIDE);
	struct chunk *new = chunk_new(CHUNK_SIDE, CHUNK_SIDE);

	chunk_copy_details(player, cave, new, CHUNK_SIDE, CHUNK_SIDE, from,
					   loc(0, 0), 0, false, true);

	return new;
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
			for (obj = square_object(c, grid); obj; obj = obj->next)
				assert(obj->tval != 0);
			if (square(c, grid)->mon > 0) {
				struct monster *mon = square_monster(c, grid);
				if (mon->held_obj)
					for (obj = mon->held_obj; obj; obj = obj->next)
						assert(obj->tval != 0);
			}
		}
	}
}

/**
 * ------------------------------------------------------------------------
 * Chunk placement utilities
 *
 * Note that the playing arena is a 3x3 grid of chunks, indexed 0, 1, 2
 * in the x and y directions from the top left, so for example the centre
 * chunk of the arena has x offset 1, y offset 1
 * ------------------------------------------------------------------------ */
/**
 * Translate from offsets to adjacent index.  0 is up, 10 is down, 1-9 are 
 * the keypad directions
 */
static int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset)
{
	if (z_offset == -1) {
		return DIR_UP;
	} else if (z_offset == 1) {
		return DIR_DOWN;
	} else if ((y_offset >= 0) && (y_offset <= 2) &&
			   (x_offset >= 0) && (x_offset <= 2)) {
		return (7 - 3 * y_offset + x_offset);
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
		*y_off = 1;
		*x_off = 1;
	} else if (adjacent == DIR_DOWN) {
		*z_off = 1;
		*y_off = 1;
		*x_off = 1;
	} else {
		*z_off = 0;
		*y_off = 2 - ((adjacent - 1) / 3);
		*x_off = (adjacent - 1) % 3;
	}
}

/**
 * Translate offset from current chunk into a chunk_list index
 */
static int chunk_get_idx(int z_offset, int y_offset, int x_offset)
{
	int adj_index = chunk_offset_to_adjacent(z_offset, y_offset, x_offset);

	if (adj_index == -1) {
		quit_fmt("No chunk at y offset %d, x offset %d", y_offset, x_offset);
	}

	return chunk_list[player->place].adjacent[adj_index];
}

/**
 * Find the region a set of coordinates is in - dungeons are treated as part
 * of the region they are directly below
 */
static int find_region(int y_pos, int x_pos)
{
	int i;

	for (i = 0; i < z_info->region_max; i++) {
		struct world_region *region = &region_info[i];
		int entry;

		if ((y_pos / 10 < region->y_offset) ||
			(y_pos / 10 >= region->y_offset + region->height)) {
			continue;
		}

		if ((x_pos / 10 < region->x_offset) ||
			(x_pos / 10 >= region->x_offset + region->width)) {
			continue;
		}

		entry =	region->width * ((y_pos / 10) - region->y_offset) + x_pos / 10;
		assert(entry >= 0);
		if (region->text[entry] == ' ') {
			continue;
		}

		break;
	}

	return i;
}

/**
 * Get the location data for a chunk
 */
static void chunk_adjacent_data(struct chunk_ref *ref, int z_offset,
								int y_offset, int x_offset)
{
	if (((ref->y_pos == 0) && (y_offset == 0)) ||
		((ref->y_pos >= 10 * MAX_Y_REGION - 1) && (y_offset == 2)) ||
		((ref->x_pos == 0) && (x_offset == 0)) ||
		((ref->x_pos >= 10 * MAX_X_REGION - 1) && (x_offset == 2))) {
		ref->region = 0;
	} else {
		ref->z_pos += z_offset;
		ref->y_pos += (y_offset - 1);
		ref->x_pos += (x_offset - 1);
		if (z_offset == 0)
			ref->region = find_region(ref->y_pos, ref->x_pos);
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
 * ------------------------------------------------------------------------
 * Chunk list operations
 * ------------------------------------------------------------------------ */
/**
 * Add an entry to the chunk list - any problems with the length of this will
 * be more in the memory used by the chunks themselves rather than the list
 * \param c the chunk being added to the list
 */
void old_chunk_list_add(struct chunk *c)
{
	int newsize = (old_chunk_list_max + CHUNK_LIST_INCR) *	sizeof(struct chunk *);

	/* Lengthen the list if necessary */
	if ((old_chunk_list_max % CHUNK_LIST_INCR) == 0)
		old_chunk_list = (struct chunk **) mem_realloc(old_chunk_list, newsize);

	/* Add the new one */
	old_chunk_list[old_chunk_list_max++] = c;
}

/**
 * Remove an entry from the chunk list, return whether it was found
 * \param name the name of the chunk being removed from the list
 * \return whether it was found; success means it was successfully removed
 */
bool old_chunk_list_remove(const char *name)
{
	int i;

	/* Find the match */
	for (i = 0; i < old_chunk_list_max; i++) {
		if (streq(name, old_chunk_list[i]->name)) {
			/* Copy all the succeeding chunks back one */
			int j;
			for (j = i + 1; j < old_chunk_list_max; j++) {
				old_chunk_list[j - 1] = old_chunk_list[j];
			}

			/* Shorten the list and return */
			old_chunk_list_max--;
			old_chunk_list[old_chunk_list_max] = NULL;
			return true;
		}
	}

	return false;
}

/**
 * Find a chunk by name
 * \param name the name of the chunk being sought
 * \return the pointer to the chunk
 */
struct chunk *chunk_find_name(const char *name)
{
	int i;

	for (i = 0; i < old_chunk_list_max; i++)
		if (streq(name, old_chunk_list[i]->name))
			return old_chunk_list[i];

	return NULL;
}

/**
 * Allocate a new chunk of the world
 */
struct chunk *chunk_new(int height, int width)
{
	int y, x;

	struct chunk *c = mem_zalloc(sizeof *c);
	c->height = height;
	c->width = width;
	c->feat_count = mem_zalloc((z_info->f_max + 1) * sizeof(int));

	c->squares = mem_zalloc(c->height * sizeof(struct square*));
	//chunk rename c->noise.grids = heatmap_new(c);
	//chunk rename c->scent.grids = heatmap_new(c);
	for (y = 0; y < c->height; y++) {
		c->squares[y] = mem_zalloc(c->width * sizeof(struct square));
		for (x = 0; x < c->width; x++) {
			c->squares[y][x].info = mem_zalloc(SQUARE_SIZE * sizeof(bitflag));
		}
	}

	c->objects = mem_zalloc(OBJECT_LIST_SIZE * sizeof(struct object*));
	c->obj_max = OBJECT_LIST_SIZE - 1;

	c->monsters = mem_zalloc(z_info->level_monster_max *sizeof(struct monster));
	c->mon_max = 1;
	c->mon_current = -1;

	c->monster_groups = mem_zalloc(z_info->level_monster_max *
								   sizeof(struct monster_group*));

	return c;
}

/**
 * Wipe the actual details of a chunk
 */
void chunk_wipe(struct chunk *c)
{
	int y, x, i;

	/* Look for orphaned objects and delete them. */
	for (i = 1; i < c->obj_max; i++) {
		if (c->objects[i] && loc_is_zero(c->objects[i]->grid)) {
			object_delete(c, &c->objects[i]);
		}
	}

	for (y = 0; y < c->height; y++) {
		for (x = 0; x < c->width; x++) {
			mem_free(c->squares[y][x].info);
			if (c->squares[y][x].trap)
				//chunk rename square_free_trap(c, loc(x, y));
			if (c->squares[y][x].obj)
				object_pile_free(c, c->squares[y][x].obj);
		}
		mem_free(c->squares[y]);
	}
	mem_free(c->squares);
	//chunk rename heatmap_free(c, c->noise);
	//chunk rename heatmap_free(c, c->scent);

	mem_free(c->feat_count);
	mem_free(c->objects);
	mem_free(c->monsters);
	mem_free(c->monster_groups);
	mem_free(c);
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
static int chunk_find(struct chunk_ref ref)
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
		if (!ref->region) {
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
			chunk_adjacent_data(&ref1, z_off, y_off, x_off);

			/* Deal with existing chunks */
			chunk_idx = chunk_find(ref1);
			if (chunk_idx < MAX_CHUNKS)
				ref->adjacent[n] = chunk_idx;
		}
	}
}

/**
 * Store a chunk from the current playing area into the chunk list
 */
static int chunk_store(int y_offset, int x_offset, u16b region, u16b z_pos,
				u16b y_pos, u16b x_pos, bool write)
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
		if (chunk_cnt >= MAX_CHUNKS) {
			/* Find and delete the oldest chunk */
			idx = 0;
			for (i = 0; i < MAX_CHUNKS; i++)
				if (chunk_list[i].turn < max) {
					max = chunk_list[i].turn;
					idx = i;
				}

			chunk_delete(idx);

			/* Delete whole levels at once */
			if (chunk_list[idx].z_pos > 0)
				chunk_delete_level(max);

			/* Decrement the counter, and the maximum if necessary */
			chunk_cnt--;
			if (idx == chunk_max)
				chunk_max--;
		}

		/* Find the next free slot */
		else {
			for (idx = 0; idx < chunk_max; idx++)
				if (!chunk_list[idx].region)
					break;
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
	chunk_list[idx].adjacent[5] = idx;

	/* Write the chunk */
	if (write)
		chunk_list[idx].chunk = chunk_write(y_offset, x_offset);

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
static void chunk_generate(struct chunk_ref ref, int y_offset, int x_offset)
{
	int n, z_off, y_off, x_off;
	int z_pos = ref.z_pos, y_pos = ref.y_pos, x_pos = ref.x_pos;
	int lower, upper;
	//char terrain;
	bool reload;
	struct gen_loc *location;
	struct connector east[CHUNK_SIDE] = {{{0}, 0, {0}, 0}};
	struct connector west[CHUNK_SIDE] = {{{0}, 0, {0}, 0}};
	struct connector north[CHUNK_SIDE] = {{{0}, 0, {0}, 0}};
	struct connector south[CHUNK_SIDE] = {{{0}, 0, {0}, 0}};
	struct connector vertical[CHUNK_SIDE][CHUNK_SIDE] = {{{{0}, 0, {0}, 0}}};
	struct connector *first = NULL;
	struct connector *latest = NULL;

	/* If no region, return */
	if (!ref.region)
		return;

	/* See if we've been generated before */
	reload = gen_loc_find(x_pos, y_pos, z_pos, &lower, &upper);

	/* Access the old place in the gen_loc_list, or make the new one */
	if (reload) {
		location = &gen_loc_list[lower];
	} else {
		gen_loc_make(x_pos, y_pos, z_pos, upper);
	}

	/* Store the chunk reference */
	//idx = chunk_store(1, 1, ref.region, z_pos, y_pos, x_pos, false);
	(void) chunk_store(1, 1, ref.region, z_pos, y_pos, x_pos, false);

	/* Get adjacent data */
	for (n = 0; n < DIR_MAX; n++) {
		struct chunk_ref ref1 = { 0 };

		/* Get the reference data for the adjacent chunk */
		chunk_adjacent_to_offset(n, &z_off, &y_off, &x_off);
		ref1.x_pos = x_pos;
		ref1.y_pos = y_pos;
		ref1.z_pos = z_pos;
		chunk_adjacent_data(&ref1, z_off, y_off, x_off);

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
						for (start = loc->join; start->next;
							 start = start->next) {
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
						for (start = loc->join; start->next;
							 start = start->next) {
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
						for (start = loc->join; start->next;
							 start = start->next) {
							current = &west[start->grid.y];

							if (start->grid.x == CHUNK_SIDE - 1) {
								connector_copy(current, start);
								current->grid.x = 255;
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_E:
					{
						for (start = loc->join; start->next;
							 start = start->next) {
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
						for (start = loc->join; start->next;
							 start = start->next) {
							current = &north[start->grid.x];

							if (start->grid.y == CHUNK_SIDE - 1) {
								connector_copy(current, start);
								current->grid.y = 255;
								connector_add(&first, &latest, &current);
							}
						}
						break;
					}
					case DIR_DOWN:
					{
						for (start = loc->join; start->next;
							 start = start->next) {
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
#if 0
		build_landmark(n, y_pos, x_pos, y_offset, x_offset);
#endif
	} else {
		/* ...or generate the chunk */
		//terrain = region_terrain[y_pos / 10][x_pos / 10];

		/* Set the RNG to give reproducible results */
		Rand_quick = true;
		Rand_value = ((y_pos & 0x1fff) << 19);
		Rand_value |= ((z_pos & 0x3f) << 13);
		Rand_value |= (x_pos & 0x1fff);
		Rand_value ^= seed_flavor;
#if 0
		switch (terrain) {
		case '.':
			{
				plain_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '+':
			{
				forest_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '-':
			{
				lake_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '~':
			{
				ocean_gen(ref, y_offset, x_offset, first);
				break;
			}
		case ',':
			{
				moor_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '^':
			{
				mtn_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '_':
			{
				swamp_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '|':
			{
				dark_gen(ref, y_offset, x_offset, first);
				break;
			}
		case 'X':
			{
				impass_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '/':
			{
				desert_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '*':
			{
				snow_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '=':
			{
				town_gen(ref, y_offset, x_offset, first);
				break;
			}
		case '&':
			{
				landmk_gen(ref, y_offset, x_offset, first);
				break;
			}
		default:
			{
				ocean_gen(ref, y_offset, x_offset, first);
				break;
			}
		}
#endif
		Rand_quick = false;
	}

	/* Do terrain changes */
	if (reload) {
		struct terrain_change *change;

		/* Change any terrain that has changed since first generation */
		for (change = location->change; change; change = change->next) {
			int y = y_offset * CHUNK_SIDE + change->grid.y;
			int x = x_offset * CHUNK_SIDE + change->grid.x;

			square_set_feat(cave, loc(x, y), change->feat);
		}
	} else {
		/* Write connectors */
		int num_joins = 0;
		int y, x;
		int y0 = CHUNK_SIDE * y_offset;
		int x0 = CHUNK_SIDE * x_offset;
		struct connector *current = NULL;

		/* Count the non-zero connectors needed */
		for (x = 0; x < CHUNK_SIDE; x++) {
			if (south[x].feat == 0) num_joins++;
			if (north[x].feat == 0) num_joins++;
		}
		for (y = 0; y < CHUNK_SIDE; y++) {
			if (west[y].feat == 0) num_joins++;
			if (east[y].feat == 0) num_joins++;
			for (x = 0; x < CHUNK_SIDE; x++) {
				byte feat = vertical[y][x].feat;
				if (feat != 0) {
					if (feat_is_stair(feat)	|| feat_is_chasm(feat)) num_joins++;
				}
			}
		}

		/* Now write them */
		gen_loc_list[upper].join = mem_zalloc(num_joins *
											  sizeof(struct connector));
		current = gen_loc_list[upper].join;
		for (x = 0; x < CHUNK_SIDE; x++) {
			if (south[x].feat == 0) {
				struct loc grid = loc(x0 + x, y0 + CHUNK_SIDE - 1);
				current->grid.y = CHUNK_SIDE - 1;
				current->grid.x = x;
				current->feat = square(cave, grid)->feat;
				sqinfo_copy(current->info, square(cave, grid)->info);
				num_joins--;
				if (num_joins != 0) {
					current->next = current + 1;
					current = current->next;
				}
			}
			if (north[x].feat == 0) {
				struct loc grid = loc(x0 + x, y0);
				current->grid.y = 0;
				current->grid.x = x;
				current->feat = square(cave, grid)->feat;
				sqinfo_copy(current->info, square(cave, grid)->info);
				num_joins--;
				if (num_joins != 0) {
					current->next = current + 1;
					current = current->next;
				}
			}
			for (y = 0; y < CHUNK_SIDE; y++) {
				byte feat = vertical[y][x].feat;
				struct loc grid = loc(x0 + x, y0 + y);
				if (feat == 0) {
					if (feat_is_stair(feat)	|| feat_is_chasm(feat)) {
						current->grid.y = y;
						current->grid.x = x;
						current->feat = square(cave, grid)->feat;
						sqinfo_copy(current->info, square(cave, grid)->info);
						num_joins--;
						if (num_joins != 0) {
							current->next = current + 1;
							current = current->next;
						}
					}
				}
			}
		}
		for (y = 0; y < CHUNK_SIDE; y++) {
			if (west[y].feat == 0) {
				struct loc grid = loc(x0, y0 + y);
				current->grid.y = y;
				current->grid.x = 0;
				current->feat = square(cave, grid)->feat;
				sqinfo_copy(current->info, square(cave, grid)->info);
				num_joins--;
				if (num_joins != 0) {
					current->next = current + 1;
					current = current->next;
				}
			}
			if (east[y].feat == 0) {
				struct loc grid = loc(x0 + CHUNK_SIDE - 1, y0 + y);
				current->grid.y = y;
				current->grid.x = CHUNK_SIDE - 1;
				current->feat = square(cave, grid)->feat;
				sqinfo_copy(current->info, square(cave, grid)->info);
				num_joins--;
				if (num_joins != 0) {
					current->next = current + 1;
					current = current->next;
				}
			}
		}
		if (num_joins == 0) {
			current->next = NULL;
		}
	}
}

/**
 * Deal with re-aligning the playing arena on the same z-level
 *
 * Used for walking off the edge of a chunk
 */
static void arena_realign(int y_offset, int x_offset)
{
	int x, y;
	bool chunk_exists[10] = { 0 };
	int new_idx;
	struct chunk *new = chunk_new(CHUNK_SIDE * 3, CHUNK_SIDE * 3);
	struct loc src_top_left;
	struct loc dest_top_left;
	int height, width;

	/* Get the new centre chunk */
	new_idx = chunk_offset_to_adjacent(0, y_offset, x_offset);
	assert(new_idx < MAX_CHUNKS);

	/* Unload chunks no longer required */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			struct chunk_ref *ref = NULL;
			int chunk_idx;

			/* Keep chunks adjacent to the new centre */
			if ((ABS(x_offset - x) < 2) && (ABS(y_offset - y) < 2)) {
				int adj_index;
				int new_y = y + 1 - y_offset;
				int new_x = x + 1 - x_offset;

				if ((new_y < 0) || (new_x < 0))
					continue;

				/* Record this one as existing */
				adj_index = chunk_offset_to_adjacent(0, new_y, new_x);
				if (adj_index == -1)
					quit_fmt("Bad chunk index at y offset %d, x offset %d",
							 new_y, new_x);
				chunk_exists[adj_index] = true;
				continue;
			}

			/* Access the chunk's placeholder in chunk_list */
			chunk_idx = chunk_get_idx(0, y, x);
			ref = &chunk_list[chunk_idx];

			/* Store it */
			(void) chunk_store(y, x, ref->region, ref->z_pos, ref->y_pos,
							   ref->x_pos, true);
		}
	}

	/* Re-align current playing arena */
	if (y_offset == 0) {
		src_top_left.y = CHUNK_SIDE;
		dest_top_left.y = 0;
		height = 2 * CHUNK_SIDE;
	} else if (y_offset == 1) {
		src_top_left.y = 0;
		dest_top_left.y = 0;
		height = 3 * CHUNK_SIDE;
	} else if (y_offset == 2) {
		src_top_left.y = 0;
		dest_top_left.y = CHUNK_SIDE;
		height = 2 * CHUNK_SIDE;
	}
	if (x_offset == 0) {
		src_top_left.x = CHUNK_SIDE;
		dest_top_left.x = 0;
		width = 2 * CHUNK_SIDE;
	} else if (x_offset == 1) {
		src_top_left.x = 0;
		dest_top_left.x = 0;
		width = 3 * CHUNK_SIDE;
	} else if (x_offset == 2) {
		src_top_left.x = 0;
		dest_top_left.x = CHUNK_SIDE;
		width = 2 * CHUNK_SIDE;
	}
	chunk_copy_details(player, cave, new, height, width, src_top_left,
					   dest_top_left, 0, false, false);
	chunk_wipe(cave);
	cave = new;

	/* Player has moved chunks */
	player->last_place = player->place;
	player->place = chunk_list[player->place].adjacent[new_idx];

	/* Reload or generate chunks to fill the playing area. 
	 * Note that chunk generation needs to write the adjacent[] entries */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			int chunk_idx;
			int adj_index = chunk_offset_to_adjacent(0, y, x);
			struct chunk_ref ref = { 0 };

			/* Already in the current playing area */
			if (chunk_exists[adj_index])
				continue;

			/* Get the location data */
			ref.region = chunk_list[player->place].region;
			ref.z_pos = 0;
			ref.y_pos = chunk_list[player->place].y_pos;
			ref.x_pos = chunk_list[player->place].x_pos;
			chunk_adjacent_data(&ref, 0, y, x);

			/* Load it if it already exists */
			chunk_idx = chunk_find(ref);
			if ((chunk_idx != MAX_CHUNKS) && chunk_list[chunk_idx].chunk) {
				chunk_read(chunk_idx, y, x);
			} else {
				/* Otherwise generate a new one */
				chunk_generate(ref, y, x);
			}
		}
	}
	cave_illuminate(cave, is_daytime());
	update_view(cave, player);
}

/**
 * Get the centre chunk from the playing arena
 * This is necessary in dungeons because the player is not kept central
 */
int chunk_get_centre(void)
{
	int centre = player->place;

	/* Only check if we're not on the surface */
	if (player->depth) {
		/* Find the centre */
		for (centre = 0; centre < MAX_CHUNKS; centre++) {
			if (chunk_list[centre].turn == chunk_list[player->place].turn) {
				int j;
				for (j = 0; j < 9; j++) {
					if (chunk_list[centre].adjacent[j] == MAX_CHUNKS) break;
				}
				if (j == 9)
					break;
			}
		}
	}

	return centre;
}

/**
 * Deal with moving the playing arena to a different z-level
 *
 * Used for stairs, teleport level, falling
 */
static void level_change(int z_offset)
{
	int y, x, new_idx;
	int centre = chunk_get_centre();

	/* Unload chunks no longer required */
	for (y = 0; y < 3; y++) {
		for (x = 0; x < 3; x++) {
			struct chunk_ref ref = chunk_list[centre];

			/* Get the location data */
			chunk_adjacent_data(&ref, 0, y, x);
			ref.z_pos = player->depth;

			/* Store it */
			(void) chunk_store(y, x, ref.region, ref.z_pos, ref.y_pos,
							   ref.x_pos, true);
		}
	}

	/* Get the new chunk */
	new_idx = chunk_offset_to_adjacent(z_offset, 0, 0);

	/* Set the chunk (possibly invalid) */
	player->last_place = player->place;
	player->place = chunk_list[player->place].adjacent[new_idx];

	/* Leaving, make new level */
	player->upkeep->generate_level = true;

	/* Save the game when we arrive on the new level. */
	player->upkeep->autosave = true;

	/* Set depth */
	player->depth += z_offset;
}

/**
 * Handle the player moving from one chunk to an adjacent one.  This function
 * needs to handle moving in the eight surface directions, plus up or down
 * one level, and the consequent moving of chunks to and from chunk_list.
 */
void chunk_change(int z_offset, int y_offset, int x_offset)
{
	if (z_offset) {
		level_change(z_offset);
	} else {
		arena_realign(y_offset, x_offset);
	}
}
