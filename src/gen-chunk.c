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
 * Wipe the actual details of a chunk
 */
void chunk_wipe(int idx)
{
    world_chunk *chunk = chunk_list[idx].chunk;

    /* Free everything */
    if (chunk->cave_info)
    {
	mem_free(chunk->cave_info);
	chunk->cave_info = NULL;
    }
    if (chunk->cave_feat)
    {
	mem_free(chunk->cave_feat);
	chunk->cave_feat = NULL;
    }
    if (chunk->cave_o_idx)
    {
	mem_free(chunk->cave_o_idx);
	chunk->cave_o_idx = NULL;
    }
    if (chunk->cave_m_idx)
    {
	mem_free(chunk->cave_m_idx);
	chunk->cave_m_idx = NULL;
    }
    if (chunk->o_list)
    {
	mem_free(chunk->o_list);
	chunk->o_list = NULL;
    }
    if (chunk->m_list)
    {
	mem_free(chunk->m_list);
	chunk->m_list = NULL;
    }
    if (chunk->trap_list)
    {
	mem_free(chunk->trap_list);
	chunk->trap_list = NULL;
    }
    if (chunk)
    {
	mem_free(chunk);
	chunk_list[idx].chunk = NULL;
    }
}

/**
 * Delete a chunk ref from the chunk_list
 */
void chunk_delete(int idx)
{
    int i, j;
    chunk_ref *chunk;
    chunk_ref *ref = &chunk_list[idx];

    ref->age = 0;
    ref->region = 0;
    ref->z_pos = 0;
    ref->y_pos = 0;
    ref->x_pos = 0;
    ref->gen_loc_idx = 0;
    if (ref->chunk)
	chunk_wipe(idx);
    for (i = 0; i < DIR_MAX; i++)
	ref->adjacent[i] = MAX_CHUNKS;

    /* Repair chunks */
    for (i = 0; i < chunk_max; i++)
    {
	/* Get the chunk */
	chunk = &chunk_list[i];

	/* Skip dead chunks */
	if (!chunk->region)
	    continue;

	/* Repair adjacencies */
	for (j = 0; j < DIR_MAX; j++)
	    if (chunk->adjacent[j] == idx)
		chunk->adjacent[j] = MAX_CHUNKS;

    }
}

/*
 * Move a chunk ref from index i1 to index i2 in the chunk list
 */
static void compact_chunks_aux(int i1, int i2)
{
    int i, j;

    chunk_ref *chunk;

    /* Do nothing */
    if (i1 == i2)
	return;

    /* Repair chunks */
    for (i = 0; i < chunk_max; i++)
    {
	/* Get the chunk */
	chunk = &chunk_list[i];

	/* Skip dead chunks */
	if (!chunk->region)
	    continue;

	/* Repair adjacencies */
	for (j = 0; j < DIR_MAX; j++)
	    if (chunk->adjacent[j] == i1)
		chunk->adjacent[j] = i2;

    }

    /* Fix index */
    chunk = &chunk_list[i1];
    chunk->ch_idx = i2;

    /* Move chunk */
    memcpy(&chunk_list[i2], &chunk_list[i1], sizeof(chunk_ref));

    /* Delete the old one */
    chunk_delete(i1);
    chunk_cnt--;
}

/**
 * Move all chunk refs to the start of the array
 */
void compact_chunks(void)
{
    int i;

    /* Excise dead chunks (backwards!) */
    for (i = chunk_max - 1; i >= 0; i--) {
	chunk_ref *chunk = &chunk_list[i];

	/* Skip real chunks */
	if (chunk->region)
	    continue;

	/* Move last chunk into open hole */
	compact_chunks_aux(chunk_max - 1, i);

	/* Compress chunk_max */
	chunk_max--;
    }

    /* Repair the list */
    chunk_fix_all();
}

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
	    cave_copy(new->cave_info[y][x], cave_info[y0 + y][x0 + x]);
	    
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
		monster_type *n_ptr = NULL;

		/* Valid monster */
		if (m_ptr->r_idx <= 0) continue;

		/* Copy over */
		new->cave_m_idx[y][x] = ++new->m_cnt;
		n_ptr = &new->m_list[new->m_cnt];
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

    return new;
}

/**
 * Age off a chunk from the chunk_list
 */
void chunk_age_off(int idx)
{
    /* Delete the chunk */
    chunk_delete(idx);

    /* Decrement the counter */
    chunk_cnt--;

    /* Repair the list */
    chunk_fix_all();
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
int chunk_store(int y_offset, int x_offset, u16b region, u16b z_pos, u16b y_pos,
		u16b x_pos, bool write)
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
	if (chunk_cnt >= MAX_CHUNKS)
	{
	    /* Find and delete the oldest chunk */
	    idx = 0;
	    for (i = 0; i < MAX_CHUNKS; i++)
		if (chunk_list[i].age > max)
		{
		    max = chunk_list[i].age;
		    idx = i;
		}

	    chunk_delete(idx);

	    /* Decrement the counter, and the maximum if necessary */
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
    
	/* Increment the counter, and the maximum if necessary */
	chunk_cnt++;
	assert (chunk_max <= MAX_CHUNKS);
	if (idx == chunk_max)
	    chunk_max++;
    }

    /* Set all the values */
    chunk_list[idx].ch_idx = idx;

    chunk_list[idx].age = 1;

    chunk_list[idx].region = region;
    chunk_list[idx].y_pos = y_pos;
    chunk_list[idx].x_pos = x_pos;
    chunk_list[idx].z_pos = z_pos;
    chunk_list[idx].adjacent[5] = idx;

    /* Write the chunk */
    if (write) chunk_list[idx].chunk = chunk_write(y_offset, x_offset);

    /* Repair the list */
    chunk_fix_all();

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
	    cave_copy(cave_info[y0 + y][x0 + x], chunk->cave_info[y][x]);
	    
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

    /* Reset the age */
    chunk_list[idx].age = 1;

    /* Wipe it */
    chunk_wipe(idx);
}

/**
 * Translate from offsets to adjacent index.  0 is up, 10 is down, 1-9 are 
 * the keypad directions
 */
int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset)
{
    if (z_offset == -1) return DIR_UP;
    else if (z_offset == 1) return DIR_DOWN;
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
    if (adjacent == DIR_UP)
    {
	*z_off = -1;
	*y_off = 0;
	*x_off = 0;
    }
    else if (adjacent == DIR_DOWN)
    {
	*z_off = 1;
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
 * Check and repair all the entries in the chunk_list
 */
void chunk_fix_all(void)
{
    int n, z_off, y_off, x_off, idx;

    for (idx = 0; idx < MAX_CHUNKS; idx++)
    {
	/* Get the chunk ref */
	chunk_ref *ref = &chunk_list[idx];

	/* Remove dead chunks */
	if (!ref->region)
	{
	    chunk_delete(idx);
	    continue;
	}

	/* Set the index */
	ref->ch_idx = idx;

	/* Set adjacencies */
	for (n = 0; n < DIR_MAX; n++)
	{
	    chunk_ref ref1 = CHUNK_EMPTY;
	    int chunk_idx;

	    /* Self-reference (not strictly necessary) */
	    if (n == DIR_NONE)
	    {
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
 * Find the region a set of coordinates is in - dungeons are treated as part
 * of the region they are directly below
 */
int find_region(int y_pos, int x_pos)
{
    int i;

    for (i = 0; i < z_info->region_max; i++)
    {
	region_type *region = &region_info[i];
	int entry;

	if ((y_pos / 10 < region->y_offset) ||
	    (y_pos / 10 >= region->y_offset + region->height))
	    continue;

	if ((x_pos / 10 < region->x_offset) ||
	    (x_pos / 10 >= region->x_offset + region->width))
	    continue;

	entry = region->width * ((y_pos / 10) - region->y_offset) + x_pos / 10;
	assert(entry >= 0);
	if (region->text[entry] == ' ')
	    continue;

	break;
    }

    return i;
}

/**
 * Get the location data for a chunk
 */
void chunk_adjacent_data(chunk_ref *ref, int z_offset, int y_offset, 
			 int x_offset)
{
    if (((ref->y_pos == 0) && (y_offset == 0)) || 
	((ref->y_pos >= 10 * MAX_Y_REGION - 1) && (y_offset == 2)) || 
	((ref->x_pos == 0) && (x_offset == 0)) ||
	((ref->x_pos >= 10 * MAX_X_REGION - 1) && (x_offset == 2)))
    {
	ref->region = 0;
    }
    else
    {
	ref->z_pos += z_offset;
	ref->y_pos += (y_offset - 1);
	ref->x_pos += (x_offset - 1);
	if (z_offset == 0) 
	    ref->region = find_region(ref->y_pos, ref->x_pos);
    }
}

/**
 * Copy an edge effect
 */
void edge_copy(edge_effect *dest, edge_effect *source)
{
    dest->y = source->y;
    dest->x = source->x;
    dest->terrain = source->terrain;
    cave_copy(dest->info, source->info);
    dest->next = NULL;
}

/**
 * Add an edge effect to the list
 */
void edge_add(edge_effect **first, edge_effect **latest, edge_effect **current)
{
    if (*first)
    {
	(*latest)->next = *current;
	*latest = (*latest)->next;
    }
    else
    {
	*first = *current;
	*latest = *first;
    }
    (*latest)->next = NULL;
}

/**
 * Generate a chunk
 */
void chunk_generate(chunk_ref ref, int y_offset, int x_offset)
{
    int n, z_off, y_off, x_off, idx;
    int z_pos = ref.z_pos, y_pos = ref.y_pos, x_pos = ref.x_pos;
    int lower, upper;
    char terrain;
    bool reload;
    gen_loc *location;
    edge_effect east[CHUNK_HGT] = {{0}};
    edge_effect west[CHUNK_HGT] = {{0}};
    edge_effect north[CHUNK_WID] = {{0}};
    edge_effect south[CHUNK_WID] = {{0}};
    edge_effect vertical[CHUNK_HGT][CHUNK_WID] = {{{0}}};
    edge_effect *first = NULL;
    edge_effect *latest = NULL;

    /* If no region, return */
    if (!ref.region)
	return;

    /* See if we've been generated before */
    reload = gen_loc_find(x_pos, y_pos, z_pos, &lower, &upper);

    /* Access the old place in the gen_loc_list, or make the new one */
    if (reload)
	location = &gen_loc_list[lower];
    else
	gen_loc_make(x_pos, y_pos, z_pos, lower, upper);

    /* Store the chunk reference */
    idx = chunk_store(1, 1, ref.region, z_pos, y_pos, x_pos, FALSE);
    
    /* Get adjacent data */
    for (n = 0; n < DIR_MAX; n++)
    {
	chunk_ref ref1 = CHUNK_EMPTY;

	/* Get the reference data for the adjacent chunk */
	chunk_adjacent_to_offset(n, &z_off, &y_off, &x_off);
	ref1.x_pos = x_pos;
	ref1.y_pos = y_pos;
	ref1.z_pos = z_pos;
	chunk_adjacent_data(&ref1, z_off, y_off, x_off);

	/* Look for old chunks and get edge effects */
	if ((x_off == 0) || (y_off == 0))
	{
	    int low, high;
	    bool exists = gen_loc_find(ref1.x_pos, ref1.y_pos, ref1.z_pos, 
				       &low, &high);
	    gen_loc *loc = NULL;
	    edge_effect *start = NULL;
	    edge_effect *current = NULL;

	    if (exists)
	    {
		/* Get the location */
		loc = &gen_loc_list[low];
		first = NULL;

		/* Find edge effects */
		switch (n)
		{
		case DIR_UP:
		{
		    for (start = loc->effect; start->next; start = start->next)
		    {
			current = &vertical[start->y][start->x];

			if (tf_has(f_info[start->terrain].flags, TF_DOWNSTAIR) 
			    || tf_has(f_info[start->terrain].flags, TF_FALL))
			{
			    edge_copy(current, start);
			    edge_add(&first, &latest, &current);
			}
		    }
		    break;
		}
		case DIR_S:
		{
		    for (start = loc->effect; start->next; start = start->next)
		    {
			current = &south[start->x];

			if (start->y == 0)
			{
			    edge_copy(current, start);
			    current->y = CHUNK_HGT;
			    edge_add(&first, &latest, &current);
			}
		    }
		    break;
		}
		case DIR_W:
		{
		    for (start = loc->effect; start->next; start = start->next)
		    {
			current = &west[start->y];

			if (start->x == CHUNK_WID - 1)
			{
			    edge_copy(current, start);
			    current->x = 255;
			    edge_add(&first, &latest, &current);
			}
		    }
		    break;
		}
		case DIR_E:
		{
		    for (start = loc->effect; start->next; start = start->next)
		    {
			current = &east[start->y];

			if (start->x == 0)
			{
			    edge_copy(current, start);
			    current->x = CHUNK_WID;
			    edge_add(&first, &latest, &current);
			}
		    }
		    break;
		}
		case DIR_N:
		{
		    for (start = loc->effect; start->next; start = start->next)
		    {
			current = &north[start->x];

			if (start->y == CHUNK_HGT - 1)
			{
			    edge_copy(current, start);
			    current->y = 255;
			    edge_add(&first, &latest, &current);
			}
		    }
		    break;
		}
		case DIR_DOWN:
		{
		    for (start = loc->effect; start->next; start = start->next)
		    {
			current = &vertical[start->y][start->x];

			if (tf_has(f_info[start->terrain].flags, TF_UPSTAIR))
			{
			    edge_copy(current, start);
			    edge_add(&first, &latest, &current);
			}
		    }
		    break;
		}
		}
	    }
	}
    }

    /* Generate the chunk */
    terrain = region_terrain[y_pos / 10][x_pos / 10];

    /* Set the RNG to give reproducible results */
    Rand_quick = TRUE;
    Rand_value = ((y_pos & 0x1fff) << 19);
    Rand_value |= ((z_pos & 0x3f) << 13);
    Rand_value |= (x_pos & 0x1fff);
    Rand_value ^= seed_flavor;

    switch (terrain)
    {
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

    Rand_quick = FALSE;

    /* Do terrain changes */
    if (reload)
    {
	terrain_change *change;

	/* Change any terrain that has changed since first generation */
	for (change = location->change; change; change = change->next)
	{
	    int y = y_offset * CHUNK_HGT + change->y;
	    int x = x_offset * CHUNK_WID + change->x;

	    cave_set_feat(y, x, change->terrain);
	}
    }

    /* Write edge effects */
    else
    {
	int num_effects = 0;
	int y, x;
	int y0 = CHUNK_HGT * y_offset;
	int x0 = CHUNK_WID * x_offset;
	edge_effect *current = NULL;

	/* Count the non-zero edge effects needed */
	for (x = 0; x < CHUNK_WID; x++)
	{
	    if (south[x].terrain == 0) num_effects++;
	    if (north[x].terrain == 0) num_effects++;
	}
	for (y = 0; y < CHUNK_HGT; y++)
	{
	    if (west[y].terrain == 0) num_effects++;
	    if (east[y].terrain == 0) num_effects++;
	    for (x = 0; x < CHUNK_WID; x++)
	    {
		byte feat = vertical[y][x].terrain;
		if (feat == 0)
		{
		    if (tf_has(f_info[feat].flags, TF_DOWNSTAIR) 
			|| tf_has(f_info[feat].flags, TF_FALL)
			||tf_has(f_info[feat].flags, TF_UPSTAIR))
			num_effects++;
		}
	    }
	}

	/* Now write them */
	gen_loc_list[upper].effect
	    = mem_zalloc(num_effects * sizeof(edge_effect));
	current = gen_loc_list[upper].effect;
	for (x = 0; x < CHUNK_WID; x++)
	{
	    if (south[x].terrain == 0)
	    {
		current->y = CHUNK_HGT - 1;
		current->x = x;
		current->terrain = cave_feat[y0 + CHUNK_HGT - 1][x0 + x];
		cave_copy(current->info, cave_info[y0 + CHUNK_HGT - 1][x0 + x]);
		num_effects--;
		if (num_effects != 0)
		{
		    current->next = current + 1;
		    current = current->next;
		}
	    }
	    if (north[x].terrain == 0)
	    {
		current->y = 0;
		current->x = x;
		current->terrain = cave_feat[y0][x0 + x];
		cave_copy(current->info, cave_info[y0][x0 + x]);
		num_effects--;
		if (num_effects != 0)
		{
		    current->next = current + 1;
		    current = current->next;
		}
	    }
	    for (y = 0; y < CHUNK_HGT; y++)
	    {
		byte feat = vertical[y][x].terrain;
		if (feat == 0)
		{
		    if (tf_has(f_info[feat].flags, TF_DOWNSTAIR)
			|| tf_has(f_info[feat].flags, TF_FALL)
			||tf_has(f_info[feat].flags, TF_UPSTAIR))
		    {
			current->y = y;
			current->x = x;
			current->terrain = cave_feat[y0 + y][x0 + x];
			cave_copy(current->info, cave_info[y0 + y][x0 + x]);
			num_effects--;
			if (num_effects != 0)
			{
			    current->next = current + 1;
			    current = current->next;
			}
		    }
		}
	    }
	}
	for (y = 0; y < CHUNK_HGT; y++)
	{
	    if (west[y].terrain == 0)
	    {
		current->y = y;
		current->x = 0;
		current->terrain = cave_feat[y0 + y][x0];
		cave_copy(current->info, cave_info[y0 + y][x0]);
		num_effects--;
		if (num_effects != 0)
		{
		    current->next = current + 1;
		    current = current->next;
		}
	    }
	    if (east[y].terrain == 0)
	    {
		current->y = y;
		current->x = CHUNK_WID - 1;
		current->terrain = cave_feat[y0 + y][x0 + CHUNK_WID - 1];
		cave_copy(current->info, cave_info[y0 + y][x0 + CHUNK_WID - 1]);
		num_effects--;
		if (num_effects != 0)
		{
		    current->next = current + 1;
		    current = current->next;
		}
	    }
	}
	if (num_effects == 0)
	    current->next = NULL;
    }
}

/**
 * Deal with re-aligning the playing arena on the same z-level
 *
 * Used for walking off the edge of a chunk
 */
void arena_realign(int y_offset, int x_offset)
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
    new_idx = chunk_offset_to_adjacent(0, y_offset, x_offset);
    assert(new_idx < MAX_CHUNKS);

    /* Unload chunks no longer required */
    for (y = 0; y < 3; y++)
    {
	for (x = 0; x < 3; x++)
	{
	    chunk_ref *ref = NULL;
	    int chunk_idx;

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

	    /* Access the chunk's placeholder in chunk_list.
	    * Quit if it isn't valid */
	    chunk_idx = chunk_get_idx(0, y, x);
	    ref = &chunk_list[chunk_idx];

	    /* Store it */
	    (void) chunk_store(y, x, ref->region, ref->z_pos, ref->y_pos, 
			       ref->x_pos, TRUE);
	}
    }

    forget_view();

    /* Re-align current playing arena */
    for (y = 0; y < ARENA_HGT - ABS(y_offset - 1) * CHUNK_HGT; y++)
    {
	int y_read, y_write;

	/* Work out what to copy */
	if (y_reverse)
	{
	    y_read = (ARENA_HGT - CHUNK_HGT - 1) - y;
	    y_write = (ARENA_HGT - 1) - y;
	}
	else
	{
	    y_read = y + CHUNK_HGT * (y_offset - 1);
	    y_write = y;
	}

	for (x = 0; x < ARENA_WID - ABS(x_offset - 1) * CHUNK_WID; x++)
	{
	    int x_read, x_write;
	    int this_o_idx, next_o_idx;

	    /* Work out what to copy */
	    if (x_reverse)
	    {
		x_read = (ARENA_WID - CHUNK_WID - 1) - x;
		x_write = (ARENA_WID - 1) - x;
	    }
	    else
	    {
		x_read = x + CHUNK_WID * (x_offset - 1);
		x_write = x;
	    }

	    /* Terrain */
	    cave_feat[y_write][x_write] = cave_feat[y_read][x_read];
	    cave_feat[y_read][x_read] = 0;
	    cave_copy(cave_info[y_write][x_write],
		      cave_info[y_read][x_read]);
	    cave_wipe(cave_info[y_read][x_read]);

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
		cave_m_idx[y_write][x_write] = cave_m_idx[y_read][x_read];
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


    /* Reload or generate chunks to fill the playing area. 
     * Note that chunk generation needs to write the adjacent[] entries */
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
	    chunk_adjacent_data(&ref, 0, y, x);
	    
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
			cave_wipe(cave_info[yy][xx]);

		chunk_generate(ref, y, x);
	    }
	}
    }
    illuminate();
    update_view();
}

/**
 * Handle the player moving from one chunk to an adjacent one.  This function
 * needs to handle moving in the eight surface directions, plus up or down
 * one level, and the consequent moving of chunks to and from chunk_list.
 */
void chunk_change(int z_offset, int y_offset, int x_offset)
{
    if (z_offset == 0)
    {
	arena_realign(y_offset, x_offset);
	return;
    }

    /* Where we came from */
    p_ptr->last_stage = p_ptr->stage;

    /* Leaving */
    p_ptr->leaving = TRUE;

    /* Store the old surface chunks */
    if (p_ptr->danger == 0)
    {
	int x, y;

	/* Unload chunks no longer required */
	for (y = 0; y < 3; y++)
	{
	    for (x = 0; x < 3; x++)
	    {
		chunk_ref *ref = NULL;
		int chunk_idx;

		/* Access the chunk's placeholder in chunk_list.
		 * Quit if it isn't valid */
		chunk_idx = chunk_get_idx(0, y, x);
		ref = &chunk_list[chunk_idx];

		/* Store it */
		(void) chunk_store(y, x, ref->region, ref->z_pos, ref->y_pos,
				   ref->x_pos, TRUE);
	    }
	}
    }

    /* Set danger level */
    p_ptr->danger += z_offset;

    /* New level */
    //generate_cave();
}
