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
#include "monster.h"
#include "trap.h"

world_chunk *chunk_write(int y_offset, int x_offset, int idx, 
			 u16b region, byte y_pos, byte x_pos, byte z_pos)
{
    size_t i;
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    world_chunk *new = (world_chunk*) mem_alloc(sizeof(world_chunk));

    new->ch_idx = idx;

    /* Test for persistence */
    if (p_ptr->danger == 0) 
	new->age = 0;
    else
	new->age = 1;

    new->region = region;
    new->y_pos = y_pos;
    new->x_pos = x_pos;
    new->z_pos = z_pos;

    /* Intialise */
    new->cave_info  = C_ZNEW(CHUNK_HGT, grid_chunk);
    new->cave_feat  = C_ZNEW(CHUNK_HGT, byte_chunk);
    new->cave_o_idx = C_ZNEW(CHUNK_HGT, s16b_chunk);
    new->cave_m_idx = C_ZNEW(CHUNK_HGT, s16b_chunk);
    new->o_list     = C_ZNEW(z_info->o_max, object_type);
    new->m_list     = C_ZNEW(z_info->m_max, monster_type);
    new->trap_list  = C_ZNEW(z_info->l_max, trap_type);
    new->trap_cnt = 0;
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
		for (this_o_idx = cave_o_idx[y0 + y][x0 + x]; this_o_idx; 
		     this_o_idx = next_o_idx) 
		{
		    object_type *o_ptr = &o_list[this_o_idx];
		    object_type *j_ptr = &new->o_list[++new->o_cnt];
		
		    object_copy(j_ptr, o_ptr);
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

		memcpy(n_ptr, m_ptr, sizeof(*m_ptr));
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
	trap_type *t_ptr = &trap_list[new->trap_cnt + 1];
	trap_type *u_ptr = &new->trap_list[i];
	int ty = t_ptr->fy;
	int tx = t_ptr->fx;

	if ((ty >= y0) && (ty < y0 + CHUNK_HGT) && 
	    (tx >= x0) && (tx < x0 + CHUNK_WID))
	{
	    memcpy(u_ptr, t_ptr, sizeof(*t_ptr));
	    remove_trap(t_ptr->fy, t_ptr->fx, FALSE, i);
	    new->trap_cnt++;
	    u_ptr->fy = ty -y0;
	    u_ptr->fx = tx -x0;
	}
    }
	
    return new;
}


void chunk_store(int y_offset, int x_offset, u16b region, byte y_pos, 
		 byte x_pos, byte z_pos)
{
    int i;
    int max = 0, idx = 0;

    /* Too many chunks */
    if (chunk_cnt + 1 >= MAX_CHUNKS)
    {
	/* Find and delete the oldest chunk */
	for (i = 0; i < MAX_CHUNKS; i++)
	    if (chunk_list[i]->age > max)
	    {
		max = chunk_list[i]->age;
		idx = i;
	    }

	mem_free(chunk_list[idx]);
	chunk_list[idx] = NULL;
    }

    if (!idx)
	for (idx = 0; idx < chunk_max; idx++)
	    if (!chunk_list[idx]) break;
    
    /* Increment */
    chunk_cnt++;
    if (idx == chunk_max)
	chunk_max++;

    /* Write the chunk */
    chunk_list[idx] = chunk_write(y_offset, x_offset, idx, region, y_pos, 
				  x_pos, z_pos);
}
