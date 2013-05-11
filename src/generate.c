/** \file generate.c 
    \brief Dungeon generation
 
 * Code generating a new level.  Level feelings and other 
 * messages, autoscummer behavior.  Creation of the town.  
 *
 * Copyright (c) 2011
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
#include "generate.h"
#include "monster.h"
#include "trap.h"


/*
 * Level generation is not an important bottleneck, though it can be 
 * annoyingly slow on older machines...  Thus we emphasize simplicity 
 * and correctness over speed.  See individual functions for notes.
 *
 * This entire file is only needed for generating levels.
 * This may allow smart compilers to only load it when needed.
 *
 * The "v_info.txt" file is used to store vault generation info.
 */


/**
 * Dungeon generation data -- see  cave_gen()
 */
dun_data *dun;

/**
 * Is the level moria-style?
 */
bool moria_level;

/**
 * Number of wilderness vaults
 */
int wild_vaults;



/**
 * Clear the dungeon, ready for generation to begin.
 */
static void clear_cave(void)
{
    int x, y;

    wipe_o_list();
    wipe_m_list();
    wipe_trap_list();

    /* Clear flags and flow information. */
    for (y = 0; y < ARENA_HGT; y++)
    {
	for (x = 0; x < ARENA_WID; x++)
	{
	    /* No features */
	    cave_feat[y][x] = 0;

	    /* No flags */
	    cave_wipe(cave_info[y][x]);

	    /* No flow */
	    cave_cost[y][x] = 0;
	    cave_when[y][x] = 0;

	    /* Clear any left-over monsters (should be none) and the player. */
	    cave_m_idx[y][x] = 0;
	}
    }

    /* Mega-Hack -- no player in dungeon yet */
    p_ptr->px = p_ptr->py = 0;

    /* Hack -- illegal panel */
    Term->offset_y = ARENA_HGT;
}



/**
 * Generate a random dungeon level
 *
 * Hack -- regenerate any "overflow" levels
 *
 * Hack -- allow auto-scumming via a gameplay option.
 *
 * Note that this function resets flow data and grid flags directly.
 * Note that this function does not reset features, monsters, or objects.  
 * Features are left to the town and dungeon generation functions, and 
 * wipe_m_list() and wipe_o_list() handle monsters and objects.
 */
void generate_cave(void)
{
    int y, x, num;

    level_hgt = ARENA_HGT;
    level_wid = ARENA_WID;
    clear_cave();

    /* The dungeon is not ready */
    character_dungeon = FALSE;

    /* Assume level is not themed. */
    p_ptr->themed_level = 0;

    /* Generate */
    for (num = 0; TRUE; num++) {
	bool okay = TRUE;
	const char *why = NULL;

	/* Reset monsters and objects */
	o_max = 1;
	m_max = 1;


	/* Clear flags and flow information. */
	for (y = 0; y < ARENA_HGT; y++) {
	    for (x = 0; x < ARENA_WID; x++) {
		/* No flags */
		cave_wipe(cave_info[y][x]);

		/* No flow */
		cave_cost[y][x] = 0;
		cave_when[y][x] = 0;

	    }
	}


	/* Mega-Hack -- no player in dungeon yet */
	cave_m_idx[p_ptr->py][p_ptr->px] = 0;
	p_ptr->px = p_ptr->py = 0;

	/* Reset the monster generation level */
	monster_level = p_ptr->danger;

	/* Reset the object generation level */
	object_level = p_ptr->danger;

	/* Only group is the player */
	group_id = 1;

	/* Set the number of wilderness "vaults" */
	wild_vaults = 0;
	if (p_ptr->danger > 10)
	    wild_vaults += randint0(2);
	if (p_ptr->danger > 20)
	    wild_vaults += randint0(2);
	if (p_ptr->danger > 30)
	    wild_vaults += randint0(2);
	if (p_ptr->danger > 40)
	    wild_vaults += randint0(2);
	if (no_vault())
	    wild_vaults = 0;

	for (y = 0; y < 3; y++)
	{
	    for (x = 0; x < 3; x++)
	    {
		//int chunk_idx;
		//int adj_index = chunk_offset_to_adjacent(0, y, x);
		chunk_ref ref = CHUNK_EMPTY;
		
		/* Get the location data */
		ref.region = chunk_list[p_ptr->stage].region;
		ref.z_pos = chunk_list[p_ptr->stage].z_pos;
		ref.y_pos = chunk_list[p_ptr->stage].y_pos;
		ref.x_pos = chunk_list[p_ptr->stage].x_pos;
		chunk_adjacent_data(&ref, 0, y, x);
	    
		/* Load it if it already exists
		chunk_idx = chunk_find(ref);
		if (chunk_idx != MAX_CHUNKS)
		    chunk_read(chunk_idx, y, x); */

		/* Generate a new chunk */
		//else 
		//{
		    chunk_generate(ref, y, x);
		    //}
	    }
	}

	okay = TRUE;


	/* Prevent object over-flow */
	if (o_max >= z_info->o_max) {
	    /* Message */
	    why = "too many objects";

	    /* Message */
	    okay = FALSE;
	}

	/* Prevent monster over-flow */
	if (m_max >= z_info->m_max) {
	    /* Message */
	    why = "too many monsters";

	    /* Message */
	    okay = FALSE;
	}

	/* Message */
	if ((OPT(cheat_room)) && (why))
	    msg("Generation restarted (%s)", why);

	/* Accept */
	if (okay)
	    break;

	/* Wipe the objects */
	wipe_o_list();

	/* Wipe the monsters */
	wipe_m_list();

	/* A themed level was generated */
	if (p_ptr->themed_level) {
	    /* Allow the themed level to be generated again */
	    p_ptr->themed_level_appeared &= ~(1L << (p_ptr->themed_level - 1));

	    /* This is not a themed level */
	    p_ptr->themed_level = 0;
	}
    }


    /* The dungeon is ready */
    character_dungeon = TRUE;

    /* Reset path_coord */
    p_ptr->path_coord = 0;

    /* Verify the panel */
    verify_panel();

    /* Apply illumination */
    illuminate();

    /* Reset the number of traps, runes, and thefts on the level. */
    num_trap_on_level = 0;
    number_of_thefts_on_level = 0;
    for (num = 0; num < RUNE_TAIL; num++)
	num_runes_on_level[num] = 0;
}
