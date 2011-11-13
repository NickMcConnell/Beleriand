/** \file gen-chunk.c 
    \brief World chunk generation and handling
 
 *
 * Copyright (c) 2011 Nick McConnell
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
#include "generate.h"
#include "monster.h"
#include "trap.h"

/**
 * The "current playing area" is the size of an old dungeon level, and is 
 * divided into nine chunks, indexed by x_offset and y_offset with (0, 0) 
 * being top left.
 */


/**
 * Write a world_chunk to memory and return a pointer to it
 */
world_chunk *chunk_write(int y_offset, int x_offset)
{
    int i;
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    world_chunk *new = (world_chunk*) mem_alloc(sizeof(world_chunk));

    /* Intialise */
    new->cave_info 
	= (grid_chunk *) mem_zalloc(CHUNK_HGT * sizeof(grid_chunk));
    new->cave_feat 
	= (byte_chunk *) mem_zalloc(CHUNK_HGT * sizeof(byte_chunk));
    new->cave_o_idx 
	= (s16b_chunk *) mem_zalloc(CHUNK_HGT * sizeof(s16b_chunk));
    new->cave_m_idx 
	= (s16b_chunk *) mem_zalloc(CHUNK_HGT * sizeof(s16b_chunk));
    new->o_list 
	= (object_type *) mem_zalloc(z_info->o_max * sizeof(object_type));
    new->m_list 
	= (monster_type *) mem_zalloc(z_info->m_max * sizeof(monster_type));
    new->trap_list 
	= (trap_type *) mem_zalloc(z_info->l_max * sizeof(trap_type));
    new->trap_max = 0;
    new->o_cnt = 0;
    new->m_cnt = 0;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_WID; x++)
	{
	    int this_o_idx, next_o_idx, held;
	    
	    /* Terrain */
	    new->cave_feat[y][x] = cave_feat[y0 + y][x0 + x];
	    for (i = 0; i < CAVE_SIZE; i++)		
		new->cave_info[y][x][i] = cave_info[y0 + y][x0 + x][i];
	    
	    /* Dungeon objects */
	    held = 0;
	    if (cave_o_idx[y0 + y][x0 + x])
	    {
		new->cave_o_idx[y][x] = new->o_cnt + 1;
		for (this_o_idx = cave_o_idx[y0 + y][x0 + x]; this_o_idx; 
		     this_o_idx = next_o_idx) 
		{
		    object_type *o_ptr = &o_list[this_o_idx];
		    object_type *j_ptr = &new->o_list[++new->o_cnt];
		
		    /* Copy over */
		    object_copy(j_ptr, o_ptr);

		    /* Adjust stuff */
		    j_ptr->iy = y;
		    j_ptr->ix = x;
		    next_o_idx = o_ptr->next_o_idx;
		    if (next_o_idx) 
			j_ptr->next_o_idx = new->o_cnt + 1;
		    delete_object_idx(this_o_idx);
		}
	    }

	    /* Monsters and held objects */
	    held = 0;
	    if (cave_m_idx[y0 + y][x0 + x] > 0)
	    {
		monster_type *m_ptr = &m_list[cave_m_idx[y0 + y][x0 + x]];
		monster_type *n_ptr = &new->m_list[++new->m_cnt];

		/* Copy over */
		new->cave_m_idx[y][x] = new->m_cnt;
		memcpy(n_ptr, m_ptr, sizeof(*m_ptr));

		/* Adjust stuff */
		n_ptr->fy = y;
		n_ptr->fx = x;
		if (m_ptr->hold_o_idx)
		{
		    for (this_o_idx = m_ptr->hold_o_idx; this_o_idx; 
			 this_o_idx = next_o_idx) 
		    {
			object_type *o_ptr = &o_list[this_o_idx];
			object_type *j_ptr = &new->o_list[++new->o_cnt];
			
			/* Copy over */
			object_copy(j_ptr, o_ptr);
			
			/* Adjust stuff */
			j_ptr->iy = y;
			j_ptr->ix = x;
			next_o_idx = o_ptr->next_o_idx;
			if (next_o_idx) 
			    j_ptr->next_o_idx = new->o_cnt + 1;
			j_ptr->held_m_idx = new->m_cnt;
			if (!held) held = new->o_cnt;
			delete_object_idx(this_o_idx);
		    }
		}
		n_ptr->hold_o_idx = held;
		if ((m_ptr->ty >= y0) && (m_ptr->ty < y0 + CHUNK_HGT) && 
		    (m_ptr->tx >= x0) && (m_ptr->tx < x0 + CHUNK_WID))
		{
		    n_ptr->ty = m_ptr->ty - y0;
		    n_ptr->tx = m_ptr->tx - x0;
		}
		if ((m_ptr->y_terr >= y0) && (m_ptr->y_terr < y0 + CHUNK_HGT)&& 
		    (m_ptr->x_terr >= x0) && (m_ptr->x_terr < x0 + CHUNK_WID))
		{
		    n_ptr->y_terr = m_ptr->y_terr - y0;
		    n_ptr->x_terr = m_ptr->x_terr - x0;
		}

		delete_monster(y0 + y, x0 + x);
	    }
	}
    }
	    
    /* Traps */
    for (i = 0; i < trap_max; i++)
    {
	/* Point to this trap */
	trap_type *t_ptr = &trap_list[new->trap_max + 1];
	trap_type *u_ptr = &new->trap_list[i];
	int ty = t_ptr->fy;
	int tx = t_ptr->fx;

	if ((ty >= y0) && (ty < y0 + CHUNK_HGT) && 
	    (tx >= x0) && (tx < x0 + CHUNK_WID))
	{
	    /* Copy over */
	    memcpy(u_ptr, t_ptr, sizeof(*t_ptr));

	    /* Adjust stuff */
	    new->trap_max++;
	    u_ptr->fy = ty - y0;
	    u_ptr->fx = tx - x0;
	    remove_trap(t_ptr->fy, t_ptr->fx, FALSE, i);
	}
    }

    /* Re-allocate memory to save space 
    new->o_list = (object_type *) 
	mem_realloc(new->o_list, new->o_cnt * sizeof(object_type));
    new->m_list = (monster_type *) 
	mem_realloc(new->m_list, new->m_cnt * sizeof(monster_type));
    new->trap_list = (trap_type *) 
	mem_realloc(new->trap_list, new->trap_max * sizeof(trap_type)); */
	
    return new;
}

/**
 * Free a chunk
 */
void chunk_wipe(int idx)
{
    world_chunk *chunk = chunk_list[idx].chunk;

    /* Free everything */
    mem_free(chunk->cave_info);
    mem_free(chunk->cave_feat);
    mem_free(chunk->cave_o_idx);
    mem_free(chunk->cave_m_idx);
    mem_free(chunk->o_list);
    mem_free(chunk->m_list);
    mem_free(chunk->trap_list);
    mem_free(chunk);
}

/**
 * Find a chunk_ref in chunk_list
 */
int chunk_find(chunk_ref ref)
{
    size_t i;

    /* Search the list */
    for (i = 0; i < MAX_CHUNKS; i++)
    {
	/* Reject wrong values */
	if (ref.region != chunk_list[i].region) continue;
	if (ref.x_pos != chunk_list[i].x_pos) continue;
	if (ref.y_pos != chunk_list[i].y_pos) continue;
	if (ref.z_pos != chunk_list[i].z_pos) continue;

	break;
    }

    return (int) i;
}

/**
 * Store a chunk from the current playing area into the chunk list
 */
int chunk_store(int y_offset, int x_offset, u16b region, byte z_pos, byte y_pos,
		byte x_pos, bool write)
{
    int i;
    int max = 0, idx = 0;

    chunk_ref ref = CHUNK_EMPTY;

    /* Check for an existing one */
    ref.region = region;
    ref.x_pos = x_pos;
    ref.y_pos = y_pos;
    ref.z_pos = z_pos;

    idx = chunk_find(ref);

    /* We need a new slot */
    if (idx == MAX_CHUNKS)
    {
	/* Too many chunks */
	if (chunk_cnt + 1 >= MAX_CHUNKS)
	{
	    /* Find and delete the oldest chunk */
	    for (i = 0; i < MAX_CHUNKS; i++)
		if (chunk_list[i].age > max)
		{
		    max = chunk_list[i].age;
		    idx = i;
		}

	    chunk_wipe(idx);

	    /* Decrement the counters */
	    chunk_cnt--;
	    if (idx == chunk_max)
		chunk_max--;
	}

	/* Find the next free slot */
	else
	{
	    for (idx = 0; idx < chunk_max; idx++)
		if (!chunk_list[idx].region) break;
	}
    
	/* Increment the counters */
	chunk_cnt++;
	if (idx == chunk_max)
	    chunk_max++;
    }

    /* Set all the values */
    chunk_list[idx].ch_idx = idx;

    /* Test for persistence */
    if ((p_ptr->danger == 0) || !write)
	chunk_list[idx].age = 0;
    else
	chunk_list[idx].age = 1;

    chunk_list[idx].region = region;
    chunk_list[idx].y_pos = y_pos;
    chunk_list[idx].x_pos = x_pos;
    chunk_list[idx].z_pos = z_pos;
    chunk_list[idx].adjacent[5] = idx;

    /* Write the chunk */
    if (write) chunk_list[idx].chunk = chunk_write(y_offset, x_offset);

    return idx;
}

/**
 * Read a chunk from the chunk list and put it back into the current playing
 * area
 */
void chunk_read(int idx, int y_offset, int x_offset)
{
    int i;
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    world_chunk *chunk = chunk_list[idx].chunk;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_WID; x++)
	{
	    int this_o_idx, next_o_idx, held;
	    
	    /* Terrain */
	    cave_feat[y0 + y][x0 + x] = chunk->cave_feat[y][x];
	    for (i = 0; i < CAVE_SIZE; i++)		
		cave_info[y0 + y][x0 + x][i] = chunk->cave_info[y][x][i];
	    
	    /* Objects */
	    held = 0;
	    if (chunk->cave_o_idx[y][x])
	    {
		for (this_o_idx = chunk->cave_o_idx[y][x]; this_o_idx; 
		     this_o_idx = next_o_idx) 
		{
		    object_type *o_ptr = &chunk->o_list[this_o_idx];
		    object_type *j_ptr = NULL;
		    int o_idx; 
		    bool alloc = FALSE;

		    /* Make an object */
		    o_idx = o_pop();

		    /* Hope this never happens */
		    if (!o_idx) break;
		
		    /* Copy over */
		    j_ptr = &o_list[o_idx];
		    object_copy(j_ptr, o_ptr);

		    /* Adjust stuff */
		    j_ptr->iy = y + y0;
		    j_ptr->ix = x + x0;

		    if (o_ptr->held_m_idx)
		    { 
			if (!held) held = o_idx;
		    }

		    if (!alloc) 
		    {
			cave_o_idx[y + y0][x + x0] = o_idx;
			alloc = TRUE;
		    }

		    next_o_idx = o_ptr->next_o_idx;
		    if (next_o_idx) 
		    {
			o_idx = o_pop();
			if (!o_idx) break;
			j_ptr->next_o_idx = o_idx;
		    }
		}
	    }

	    /* Monsters */
	    if (chunk->cave_m_idx[y][x] > 0)
	    {
		monster_type *m_ptr = &chunk->m_list[cave_m_idx[y][x]];
		monster_type *n_ptr = NULL;
		int m_idx;

		/* Make a monster */
		m_idx = m_pop();

		/* Hope this never happens */
		if (!m_idx) break;

		/* Copy over */
		n_ptr = &m_list[m_idx];
		memcpy(n_ptr, m_ptr, sizeof(*m_ptr));
		cave_m_idx[y + y0][x + x0] = m_idx;

		/* Adjust stuff */
		n_ptr->fy = y + y0;
		n_ptr->fx = x + x0;
		n_ptr->hold_o_idx = held;
		o_list[held].held_m_idx = m_idx;

		n_ptr->ty = m_ptr->ty + y0;
		n_ptr->tx = m_ptr->tx + x0;
		
		n_ptr->y_terr = m_ptr->y_terr + y0;
		n_ptr->x_terr = m_ptr->x_terr + x0;
	    }
	}
    }
	    
    /* Traps */
    for (i = 0; i < chunk->trap_max; i++)
    {
	trap_type *t_ptr = &chunk->trap_list[i];
	size_t j;
	
	/* Scan the entire trap list */
	for (j = 1; j < z_info->l_max; j++)
	{
	    /* Point to this trap */
	    trap_type *u_ptr = &trap_list[j];

	    /* This space is available */
	    if (!u_ptr->t_idx)
	    {
		memcpy(u_ptr, t_ptr, sizeof(*t_ptr));

		/* Adjust trap count if necessary */
		if (i + 1 > trap_max) trap_max = i + 1;

		/* We created a rune */
		if (trf_has(u_ptr->flags, TRF_RUNE)) 
		    num_runes_on_level[t_ptr->t_idx - 1]++;

		/* We created a monster trap */
		if (trf_has(u_ptr->flags, TRF_M_TRAP)) 
		    num_trap_on_level++;

		/* Toggle on the trap marker */
		cave_on(cave_info[y + y0][x + x0], CAVE_TRAP);

		break;
	    }
	}
    }

    /* Wipe it */
    chunk_wipe(idx);   
}

/**
 * Translate from offsets to adjacent index.  0 is up, 10 is down, 1-9 are 
 * the keypad directions
 */
int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset)
{
    if (z_offset == -1) return 0;
    else if (z_offset == 1) return 10;
    else if ((y_offset >= 0) && (y_offset <= 2) &&
	     (x_offset >= 0) && (x_offset <= 2))
	return (7 - 3 * y_offset + x_offset);
    else return -1;
}

/**
 * Translate from adjacent index to offsets
 */
void chunk_adjacent_to_offset(int adjacent, int *z_off, int *y_off, int *x_off)
{
    if (adjacent == 0)
    {
	*z_off = -1;
	*y_off = 0;
	*x_off = 0;
    }
    else if (adjacent == 10)
    {
	*z_off = -1;
	*y_off = 0;
	*x_off = 0;
    }
    else
    {
	*z_off = 0;
	*y_off = 2 - ((adjacent - 1) / 3);
	*x_off = (adjacent - 1) % 3;
    }
}

/**
 * Translate offset from current chunk into a chunk_list index
 */
int chunk_get_idx(int z_offset, int y_offset, int x_offset)
{
    int adj_index = chunk_offset_to_adjacent(z_offset, y_offset, x_offset);

    if (adj_index == -1)
	quit_fmt("No chunk at y offset %d, x offset %d", y_offset, x_offset);

    return chunk_list[p_ptr->stage].adjacent[adj_index];
}

/**
 * Get the location data for a chunk
 */
void chunk_adjacent_data(chunk_ref *ref, int y_offset, int x_offset)
{
    region_type *region = &region_info[ref->region];
    int y_new = ref->y_pos;
    int x_new = ref->x_pos;
    int y, x, min_y, min_x, max_y, max_x, y_frac, x_frac;
    char terrain, new_terrain;
    int i, new_region;

    /* Get the new position */
    y_new += (y_offset - 1);
    x_new += (x_offset - 1);

    /* See what's there */
    terrain = region->text[region->width * y_new + x_new];

    /* Still in the same region */
    if ((terrain < '0') || (terrain > '8'))
    {
	ref->y_pos = y_new;
	ref->x_pos = x_new;
	return;
    }

    /* Empty region */
    if (terrain == '0')
    {
	ref->region = 0;
	return;
    }

    /* Get the region border */
    min_y = region->height;
    min_x = region->width;
    max_y = 0;
    max_x = 0;
    for (y = 0; y < region->height; y++)
	for (x = 0; x < region->width; x++)
	{
	    if (region->text[region->width * y + x] == terrain)
	    {
		min_y = MIN(y, min_y);
		min_x = MIN(x, min_x);
		max_y = MAX(y, min_y);
		max_x = MAX(x, min_x);
	    }
	}

    /* Get the y and x distances along the border (as tenths of the total) */
    y_frac = max_y - min_y ? ((y_new - min_y) * 10) / (max_y - min_y) : 0;
    x_frac = max_x - min_x ? ((x_new - min_x) * 10) / (max_x - min_x) : 0;

    /* Switch to the new region */
    new_region = region->adjacent[D2I(terrain) - 1];
    region = &region_info[new_region];

    /* Find the new terrain */
    for (i = 0; i < 8; i++)
    {
	if (region->adjacent[i] == ref->region)
	{
	    terrain = I2D(i + 1);
	    break;
	}
    }

    /* Get the region border */
    min_y = region->height;
    min_x = region->width;
    max_y = 0;
    max_x = 0;
    for (y = 0; y < region->height; y++)
	for (x = 0; x < region->width; x++)
	{
	    if (region->text[region->width * y + x] == terrain)
	    {
		min_y = MIN(y, min_y);
		min_x = MIN(x, min_x);
		max_y = MAX(y, min_y);
		max_x = MAX(x, min_x);
	    }
	}

    /* Pick the new y, x co-ordinates */
    ref->region = new_region;
    ref->y_pos = max_y - min_y ? min_y + (y_frac * (max_y - min_y)) / 10 : 0;
    ref->x_pos = max_x - min_x ? min_x + (x_frac * (max_x - min_x)) / 10 : 0;
    new_terrain = region->text[region->width * ref->y_pos + ref->x_pos];

    /* Move if it's still border */
    while ((new_terrain >= '0') && (new_terrain <= '8'))
    {
	ref->y_pos += (y_offset - 1);
	ref->x_pos += (x_offset - 1);
	new_terrain = region->text[region->width * ref->y_pos + ref->x_pos];
    }
}

/**
 * Generate a chunk
 */
void chunk_generate(chunk_ref ref, int y_offset, int x_offset)
{
    int y, x, n, z_off, y_off, x_off;
    
    /* Store the chunk reference */
    int idx = chunk_store(1, 1, ref.region, ref.z_pos, ref.y_pos, ref.x_pos, 
			  FALSE);
    
    /* Set adjacencies */
    for (n = 0; n < 11; n++)
    {
	chunk_ref ref1 = CHUNK_EMPTY;
	int chunk_idx;

	/* Set the reference data for the adjacent chunk */
	chunk_adjacent_to_offset(n, &z_off, &y_off, &x_off);
	ref1.region = ref.region;
	ref1.z_pos = ref.z_pos;
	ref1.y_pos = ref.y_pos;
	ref1.x_pos = ref.x_pos;
	if (z_off == 0) 
	    chunk_adjacent_data(&ref1, y_off, x_off);
	else
	    ref1.z_pos += z_off;

	/* Self-reference (not strictly necessary) */
	if (n == 5)
	{
	    ref1.adjacent[n] = idx;
	    continue;
	}

	/* Deal with existing chunks */
	chunk_idx = chunk_find(ref1);
	if (chunk_idx < MAX_CHUNKS)
	{
	    chunk_list[idx].adjacent[n] = chunk_idx;
	    chunk_list[chunk_idx].adjacent[10 - n] = idx;
	}
	else
	    chunk_list[idx].adjacent[n] = MAX_CHUNKS;
    }

    plain_gen(ref, y_offset, x_offset);
}

/**
 * Handle the player moving from one chunk to an adjacent one.  This function
 * needs to handle moving in the eight surface directions, plus up or down
 * one level, and the consequent moving of chunks to and from chunk_list.
 */
void chunk_change(int z_offset, int y_offset, int x_offset)
{
    int i, x, y;
    bool y_reverse = FALSE, x_reverse = FALSE;
    bool chunk_exists[10] = { 0 };
    int new_idx;

    /* Go in the right direction */
    if (y_offset == 0)
	y_reverse = TRUE;
    if (x_offset == 0)
	x_reverse = TRUE;

    /* Get the new centre chunk */
    if (z_offset == 0) 
	new_idx = chunk_offset_to_adjacent(0, y_offset, x_offset);

    /* Unload chunks no longer required */
    for (y = 0; y < 3; y++)
    {
	for (x = 0; x < 3; x++)
	{
	    chunk_ref *ref = NULL;
	    int chunk_idx;

	    /* Same level, so some chunks remain */
	    if (z_offset == 0)
	    {
		/* Keep chunks adjacent to the new centre */
		if ((ABS(x_offset - x) < 2) && (ABS(y_offset - y) < 2))
		{
		    int adj_index;
		    int new_y = y + 1 - y_offset;
		    int new_x = x + 1 - x_offset;

		    if ((new_y < 0) || (new_x < 0)) continue;

		    /* Record this one as existing */
		    adj_index = chunk_offset_to_adjacent(0, new_y, new_x);
		    if (adj_index == -1)
			quit_fmt("Bad chunk index at y offset %d, x offset %d",
				 new_y, new_x);
		    chunk_exists[adj_index] = TRUE;
		    continue;
		}
	    }

	    /* Access the chunk's placeholder in chunk_list.
	    * Quit if it isn't valid */
	    chunk_idx = chunk_get_idx(0, y, x);
	    ref = &chunk_list[chunk_idx];

	    /* Store it */
	    (void) chunk_store(y, x, ref->region, ref->z_pos, ref->y_pos, 
			       ref->x_pos, TRUE);
	}
    }

    /* Re-align current playing area */
    if (z_offset == 0)
    {
	for (y = 0; y < DUNGEON_HGT - ABS(y_offset - 1) * CHUNK_HGT; y++)
	{
	    int y_read, y_write;

	    /* Work out what to copy */
	    if (y_reverse)
	    {
		y_read = (DUNGEON_HGT - CHUNK_HGT - 1) - y;
		y_write = (DUNGEON_HGT - 1) - y;
	    }
	    else
	    {
		y_read = y + CHUNK_HGT * (y_offset - 1);
		y_write = y;
	    }

	    for (x = 0; x < DUNGEON_WID - ABS(x_offset - 1) * CHUNK_WID; x++)
	    {
		int x_read, x_write;
		int this_o_idx, next_o_idx;

		/* Work out what to copy */
		if (x_reverse)
		{
		    x_read = (DUNGEON_WID - CHUNK_WID - 1) - x;
		    x_write = (DUNGEON_WID - 1) - x;
		}
		else
		{
		    x_read = x + CHUNK_WID * (x_offset - 1);
		    x_write = x;
		}

		/* Terrain */
		cave_feat[y_write][x_write] = cave_feat[y_read][x_read];
		cave_feat[y_read][x_read] = 0;
		for (i = 0; i < CAVE_SIZE; i++)
		{
		    cave_info[y_write][x_write][i]
			= cave_info[y_read][x_read][i];
		    cave_wipe(cave_info[y_read][x_read]);
		}

		/* Objects */
		if (cave_o_idx[y_read][x_read])
		{
		    cave_o_idx[y_write][x_write] = cave_o_idx[y_read][x_read];
		    for (this_o_idx = cave_o_idx[y_read][x_read]; this_o_idx;
			 this_o_idx = next_o_idx)
		    {
			object_type *o_ptr = &o_list[this_o_idx];

			/* Adjust stuff */
			o_ptr->iy = y_write;
			o_ptr->ix = x_write;
			next_o_idx = o_ptr->next_o_idx;
		    }
		    cave_o_idx[y_read][x_read] = 0;
		}

		/* Monsters */
		if (cave_m_idx[y_read][x_read] > 0)
		{
		    monster_type *m_ptr = &m_list[cave_m_idx[y_read][x_read]];

		    /* Adjust stuff */
		    m_ptr->fy = y_write;
		    m_ptr->fx = x_write;

		    if ((m_ptr->ty >= -((y_offset - 1) * CHUNK_HGT)) &&
			(m_ptr->ty < (4 - y_offset) * CHUNK_HGT) &&
			(m_ptr->tx >= -((x_offset - 1) * CHUNK_WID)) &&
			(m_ptr->tx < (4 - x_offset) * CHUNK_WID))
		    {
			m_ptr->ty = m_ptr->ty + (y_offset - 1) * CHUNK_HGT;
			m_ptr->tx = m_ptr->tx + (x_offset - 1) * CHUNK_WID;
		    }
		    if ((m_ptr->y_terr >= -((y_offset - 1) * CHUNK_HGT)) &&
			(m_ptr->y_terr < (4 - y_offset) * CHUNK_HGT) &&
			(m_ptr->x_terr >= -((x_offset - 1) * CHUNK_WID)) &&
			(m_ptr->x_terr < (4 - x_offset) * CHUNK_WID))
		    {
			m_ptr->y_terr =
			    m_ptr->y_terr + (y_offset - 1) * CHUNK_HGT;
			m_ptr->x_terr =
			    m_ptr->x_terr + (x_offset - 1) * CHUNK_WID;
		    }
		    cave_m_idx[y_read][x_read] = 0;
		}
		/* Remove the player for now */
		else if (cave_m_idx[y_read][x_read] < 0)
		    cave_m_idx[y_read][x_read] = 0;
	    }
	}

	/* Traps */
	for (i = 0; i < trap_max; i++)
	{
	    /* Point to this trap */
	    trap_type *t_ptr = &trap_list[i];
	    int ty = t_ptr->fy;
	    int tx = t_ptr->fx;

	    if ((ty >= -((y_offset - 1) * CHUNK_HGT)) &&
		(ty < (4 - y_offset) * CHUNK_HGT) &&
		(tx >= -((x_offset - 1) * CHUNK_WID)) &&
		(tx < (4 - x_offset) * CHUNK_WID))
	    {
		/* Adjust stuff */
		t_ptr->fy = ty + (y_offset - 1) * CHUNK_HGT;
		t_ptr->fx = tx + (x_offset - 1) * CHUNK_WID;
	    }
	    else
		/* Shouldn't happen */
		remove_trap(t_ptr->fy, t_ptr->fx, FALSE, i);
	}

	/* Move the player */
	p_ptr->py -= CHUNK_HGT * (y_offset - 1);
	p_ptr->px -= CHUNK_WID * (x_offset - 1);
	cave_m_idx[p_ptr->py][p_ptr->px] = -1;
	p_ptr->last_stage = p_ptr->stage;
	p_ptr->stage = chunk_list[p_ptr->stage].adjacent[new_idx];
    }


    /* Reload or generate chunks to fill the playing area. 
     * Note that chunk generation needs to write the adjacent[] entries */
    if (z_offset == 0)
    {
	for (y = 0; y < 3; y++)
	{
	    for (x = 0; x < 3; x++)
	    {
		int chunk_idx;
		int adj_index = chunk_offset_to_adjacent(0, y, x);
		chunk_ref ref = CHUNK_EMPTY;
		
		/* Already in the current playing area */
		if (chunk_exists[adj_index])
		    continue;
	    
		/* Get the location data */
		ref.region = chunk_list[p_ptr->stage].region;
		ref.z_pos = 0;
		ref.y_pos = chunk_list[p_ptr->stage].y_pos;
		ref.x_pos = chunk_list[p_ptr->stage].x_pos;
		chunk_adjacent_data(&ref, y, x);
	    
		/* Load it if it already exists */
		chunk_idx = chunk_find(ref);
		if ((chunk_idx != MAX_CHUNKS) && chunk_list[chunk_idx].chunk)
		    chunk_read(chunk_idx, y, x);

		/* Otherwise generate a new one */
		else 
		{
		    int xx, yy;
		    int x0 = x * CHUNK_WID, y0 = y * CHUNK_HGT;

		    for (yy = y0; yy < y0 + CHUNK_HGT; yy++)
			for (xx = x0; xx < x0 + CHUNK_WID; xx++)
			    cave_off(cave_info[yy][xx], CAVE_MARK);

		    chunk_generate(ref, y, x);
		}
	    }
	}
    }
    illuminate();
    update_view();
}
