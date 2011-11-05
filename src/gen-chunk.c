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
	    
	    /* Objects */
	    held = 0;
	    if (cave_o_idx[y0 + y][x0 + x])
	    {
		new->cave_o_idx[y][x] = ++new->o_cnt;
		for (this_o_idx = cave_o_idx[y0 + y][x0 + x]; this_o_idx; 
		     this_o_idx = next_o_idx) 
		{
		    object_type *o_ptr = &o_list[this_o_idx];
		    object_type *j_ptr = &new->o_list[new->o_cnt];
		
		    /* Copy over */
		    object_copy(j_ptr, o_ptr);

		    /* Adjust stuff */
		    j_ptr->iy = y;
		    j_ptr->ix = x;
		    next_o_idx = o_ptr->next_o_idx;
		    if (next_o_idx) 
			j_ptr->next_o_idx = new->o_cnt + 1;
		    if (o_ptr->held_m_idx)
		    { 
			j_ptr->held_m_idx = new->m_cnt + 1;
			if (!held) held = new->o_cnt;
		    }
		    delete_object_idx(this_o_idx);
		}
	    }

	    /* Monsters */
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

    /* Re-allocate memory to save space */
    new->o_list = (object_type *) 
	mem_realloc(new->o_list, new->o_cnt * sizeof(object_type));
    new->m_list = (monster_type *) 
	mem_realloc(new->m_list, new->m_cnt * sizeof(monster_type));
    new->trap_list = (trap_type *) 
	mem_realloc(new->trap_list, new->trap_max * sizeof(trap_type));
	
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
void chunk_store(int y_offset, int x_offset, u16b region, byte z_pos, 
		 byte y_pos, byte x_pos)
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
		if (!chunk_list[idx].chunk) break;
	}
    
	/* Increment the counters */
	chunk_cnt++;
	if (idx == chunk_max)
	    chunk_max++;
    }

    /* Set all the values */
    chunk_list[idx].ch_idx = idx;

    /* Test for persistence */
    if (p_ptr->danger == 0) 
	chunk_list[idx].age = 0;
    else
	chunk_list[idx].age = 1;

    chunk_list[idx].region = region;
    chunk_list[idx].y_pos = y_pos;
    chunk_list[idx].x_pos = x_pos;
    chunk_list[idx].z_pos = z_pos;

    /* Write the chunk */
    chunk_list[idx].chunk = chunk_write(y_offset, x_offset);
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
 * Translate from offsets to adjacent index
 */
int chunk_offset_to_adjacent(int z_offset, int y_offset, int x_offset)
{
    if (z_offset == -1) return 0;
    else if (z_offset == 1) return 10;
    else return (7 - 3 * y_offset + x_offset);
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

    return chunk_list[p_ptr->stage].adjacent[adj_index];
}


/**
 * Handle the player moving from one chunk to an adjacent one.  This function
 * needs to handle moving in the eight surface directions, plus up or down
 * one level, and the consequent moving of chunks to and from chunk_list.
 */
void chunk_change(int z_offset, int y_offset, int x_offset)
{
    size_t i;
    int x, y;
    int chunk_idx;
    chunk_ref *ref = NULL;
    bool y_reverse = FALSE, x_reverse = FALSE;

    /* Go in the right direction */
    if (y_offset == 0)
	y_reverse = TRUE;
    if (x_offset == 0)
	x_reverse = TRUE;

    /* Check for existing chunks - not needed ?
    for (i = 0; i < MAX_CHUNKS; i++)
    {
	if (chunk_idx == chunk_list[i].ch_idx)
	    break;
    }*/

    /* Unload chunks no longer required */
    for (y = 0; y < 3; y++)
    {
	for (x = 0; x < 3; x++)
	{
	    /* Same level, so some chunks remain */
	    if (z_offset == 0)
	    {
		/* Keep chunks adjacent to the new centre */
		if ((ABS(x_offset - x) < 2) && (ABS(y_offset - y) < 2))
		    continue;
	    }

	    /* Access the chunk's placeholder in chunk_list */
	    chunk_idx = chunk_get_idx(0, y, x);
	    ref = &chunk_list[chunk_idx];

	    /* Store it */
	    chunk_store(y, x, ref->region, ref->z_pos, ref->y_pos, ref->x_pos);
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
		for (i = 0; i < CAVE_SIZE; i++)
		    cave_info[y_write][x_write][i]
			= cave_info[y_read][x_read][i];

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
		}
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
    }

    /* Reload or generate chunks to fill the playing area */
    /* Note chunk generation needs to write the adjacent[] entries */
}
