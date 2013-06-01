/** \file gen-util.c 
    \brief Dungeon generation utilities
 
 * Helper functions making and stocking levels when generated.  
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


/**************************************************************/
/*                                                            */
/*            General dungeon-generation functions            */
/*                                                            */
/**************************************************************/


/**
 * Count the number of walls adjacent to the given grid.
 *
 * Note -- Assumes "in_bounds_fully(y, x)"
 */
int next_to_walls(int y, int x)
{
    int i, k = 0;
    feature_type *f_ptr;

    /* Count the adjacent wall grids */
    for (i = 0; i < 4; i++) 
    {
	/* Extract the terrain info */
	f_ptr = &f_info[cave_feat[y + ddy_ddd[i]][x + ddx_ddd[i]]];

	if (tf_has(f_ptr->flags, TF_WALL) &&
	    !tf_has(f_ptr->flags, TF_DOOR_ANY))
	    k++;
    }

    return (k);
}


/**
 * Returns co-ordinates for the player.  Player prefers to be near 
 * walls, because large open spaces are dangerous.
 */
void new_player_spot(void)
{
    /* Place the player */
    player_place(p_ptr->py, p_ptr->px);
}


/**
 * Convert existing terrain type to rubble
 */
static void place_rubble(int y, int x)
{
    /* Create rubble */
    cave_set_feat(y, x, FEAT_RUBBLE);
}


/**
 * Convert existing terrain type to "up stairs"
 */
static void place_up_stairs(int y, int x)
{
    /* Create up stairs */
    cave_set_feat(y, x, FEAT_LESS);
}


/**
 * Convert existing terrain type to "down stairs"
 */
static void place_down_stairs(int y, int x)
{
    /* Create down stairs */
    cave_set_feat(y, x, FEAT_MORE);
}


/**
 * Place an up/down staircase at given location
 */
void place_random_stairs(int y, int x)
{
    /* Paranoia */
    if (!cave_clean_bold(y, x))
	return;

    /* Choose a staircase */
    if (!p_ptr->danger) {
	place_down_stairs(y, x);
    } else if (randint0(100) < 50) {
	place_down_stairs(y, x);
    } else {
	place_up_stairs(y, x);
    }
}


/**
 * Place a secret door at the given location
 */
void place_secret_door(int y, int x)
{
    /* Create secret door */
    cave_set_feat(y, x, FEAT_SECRET);
}


/**
 * Place an unlocked door at the given location
 */
void place_unlocked_door(int y, int x)
{
    /* Create secret door */
    cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
}


/**
 * Place a random type of closed door at the given location.
 */
void place_closed_door(int y, int x)
{
    int tmp;

    /* Choose an object */
    tmp = randint0(400);

    /* Closed doors (300/400) */
    if (tmp < 300) {
	/* Create closed door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x00);
    }

    /* Locked doors (99/400) */
    else if (tmp < 399) {
	/* Create locked door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + randint1(7));
    }

    /* Stuck doors (1/400) */
    else {
	/* Create jammed door */
	cave_set_feat(y, x, FEAT_DOOR_HEAD + 0x08 + randint0(8));
    }
}


/**
 * Place a random type of door at the given location.
 */
void place_random_door(int y, int x)
{
    int tmp;

    /* Choose an object */
    tmp = randint0(1000);

    /* Open doors (300/1000) */
    if (tmp < 300) {
	/* Create open door */
	cave_set_feat(y, x, FEAT_OPEN);
    }

    /* Broken doors (100/1000) */
    else if (tmp < 400) {
	/* Create broken door */
	cave_set_feat(y, x, FEAT_BROKEN);
    }

    /* Secret doors (200/1000) */
    else if (tmp < 600) {
	/* Create secret door */
	cave_set_feat(y, x, FEAT_SECRET);
    }

    /* Closed, locked, or stuck doors (400/1000) */
    else {
	/* Create closed door */
	place_closed_door(y, x);
    }
}






/**
 * Places some staircases near walls
 */
void alloc_stairs(int feat, int num, int walls)
{
    int y, x, i, j;
    feature_type *f_ptr;
    bool no_down_shaft = //(!stage_map[stage_map[p_ptr->stage][DOWN]][DOWN]
	//|| is_quest(stage_map[p_ptr->stage][DOWN]) BELE lowest level needed
	(is_quest(p_ptr->stage));
    bool no_up_shaft = (chunk_list[p_ptr->stage].z_pos <= 1);
    bool morgy = is_quest(p_ptr->stage);
    //BELE M is only quest? && stage_map[p_ptr->stage][DEPTH] == 100;


    /* Place "num" stairs */
    for (i = 0; i < num; i++) {
	/* Try hard to place the stair */
	for (j = 0; j < 3000; j++) {
	    /* Cut some slack if necessary. */
	    if ((j > dun->stair_n) && (walls > 2))
		walls = 2;
	    if ((j > 1000) && (walls > 1))
		walls = 1;
	    if (j > 2000)
		walls = 0;

	    /* Use the stored stair locations first. */
	    if (j < dun->stair_n) {
		y = dun->stair[j].y;
		x = dun->stair[j].x;
	    }

	    /* Then, search at random. */
	    else {
		/* Pick a random grid */
		y = randint0(ARENA_HGT);
		x = randint0(ARENA_WID);
	    }

	    /* Require "naked" floor grid */
	    f_ptr = &f_info[cave_feat[y][x]];
	    if (!(cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR)))
		continue;

	    /* Require a certain number of adjacent walls */
	    if (next_to_walls(y, x) < walls)
		continue;

	    /* If we've asked for a shaft and they're forbidden, fail */
	    if (no_down_shaft && (feat == FEAT_MORE_SHAFT))
		return;
	    if (no_up_shaft && (feat == FEAT_LESS_SHAFT))
		return;

	    /* No way up -- must go down */
	    if (chunk_list[p_ptr->stage].z_pos == 0) 
	    {
		/* Clear previous contents, add down stairs */
		if (feat != FEAT_MORE_SHAFT)
		    cave_set_feat(y, x, FEAT_MORE);
	    }

	    /* Bottom of dungeon, Morgoth or underworld -- must go up */
	    //else if ((!stage_map[p_ptr->stage][DOWN]) || morgy) BELE 
	    else if (morgy) 
	    {
		/* Clear previous contents, add up stairs */
		if (feat != FEAT_LESS_SHAFT)
		    cave_set_feat(y, x, FEAT_LESS);
	    }

	    /* Requested type */
	    else {
		/* Clear previous contents, add stairs */
		cave_set_feat(y, x, feat);
	    }

	    /* Finished with this staircase. */
	    break;
	}
    }
}


/**
 * Allocates some objects (using "place" and "type")
 */
void alloc_object(int set, int typ, int num)
{
    int y, x, k;
    feature_type *f_ptr;

    /* Place some objects */
    for (k = 0; k < num; k++) {
	/* Pick a "legal" spot */
	while (TRUE) {
	    bool room;

	    /* Location */
	    y = randint0(ARENA_HGT);
	    x = randint0(ARENA_WID);
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Paranoia - keep objects out of the outer walls */
	    if (!in_bounds_fully(y, x))
		continue;

	    /* Require "naked" floor grid */
	    f_ptr = &f_info[cave_feat[y][x]];
	    if (!(cave_naked_bold(y, x) && tf_has(f_ptr->flags, TF_FLOOR)))
		continue;

	    /* Check for "room" */
	    room = cave_has(cave_info[y][x], CAVE_ROOM) ? TRUE : FALSE;

	    /* Require corridor? */
	    if ((set == ALLOC_SET_CORR) && room)
		continue;

	    /* Require room? */
	    if ((set == ALLOC_SET_ROOM) && !room)
		continue;

	    /* Accept it */
	    break;
	}

	/* Place something */
	switch (typ) {
	case ALLOC_TYP_RUBBLE:
	    {
		place_rubble(y, x);
		break;
	    }

	case ALLOC_TYP_TRAP:
	    {
		place_trap(y, x, -1, p_ptr->danger);
		break;
	    }

	case ALLOC_TYP_GOLD:
	    {
		place_gold(y, x);
		break;
	    }

	case ALLOC_TYP_OBJECT:
	    {
		place_object(y, x, FALSE, FALSE, FALSE, ORIGIN_FLOOR);
		break;
	    }
	}
    }
}

/**
 * Read terrain from a text file.  Allow for picking a smaller rectangle out of
 * a large rectangle.
 *
 * Used for vaults and landmarks.  Note that some vault codes are repurposed
 * here to allow more terrain for landmarks
 */
void get_terrain(int y_total, int x_total, int y_start, int x_start,
		 int y_stop, int x_stop, int y_place, int x_place,
		 const char *data, bool icky, bool light)
{
    int x, y;
    const char *t;
    bool landmark = (p_ptr->themed_level != 0);

    for (t = data, y = y_place - y_start; y < y_total + y_place - y_start; y++)
    {
	for (x = x_place - x_start; x < x_total + x_place - x_start; x++, t++)
	{
	    /* Hack -- skip "non-grids" */
	    if (*t == ' ')
		continue;

	    /* Restrict to from start to stop */
	    if ((y < y_place) || (y >= y_place + y_stop - y_start) ||
		(x < x_place) || (x >= x_place + x_stop - x_start))
		continue;

	    /* Lay down a floor or grass */
	    // BELE  vault floors all floor for now
	    cave_set_feat(y, x, FEAT_FLOOR);

	    /* Part of a vault.  Can be lit.  May be "icky". */
	    if (icky)
	    {
		cave_on(cave_info[y][x], CAVE_ICKY);
		cave_on(cave_info[y][x], CAVE_ROOM);
	    }
	    else //BELE if (stage_map[p_ptr->stage][STAGE_TYPE] == CAVE)
		cave_on(cave_info[y][x], CAVE_ROOM);
	    if (light)
		cave_on(cave_info[y][x], CAVE_GLOW);

	    /* Analyze the grid */
	    switch (*t) 
	    {
	    /* Granite wall (outer) or web. */
	    case '%':
	    {
		/* Hack - Nan Dungortheb */
		if (chunk_list[p_ptr->stage].region == 35)
		{
		    if (randint1(3) == 1)
			cave_set_feat(y, x, FEAT_FLOOR);
		    else if (randint1(2) == 1)
			cave_set_feat(y, x, FEAT_TREE);
		    else
			cave_set_feat(y, x, FEAT_TREE2);

		    place_trap(y, x, OBST_WEB, 0);
		}
		else
		    cave_set_feat(y, x, FEAT_WALL_OUTER);
		break;
	    }
	    /* Granite wall (inner) */
	    case '#':
	    {
		cave_set_feat(y, x, FEAT_WALL_INNER);
		break;
	    }
	    /* Permanent wall (inner) */
	    case 'X':
	    {
		cave_set_feat(y, x, FEAT_PERM_INNER);
		break;
	    }
	    /* Treasure seam, in either magma or quartz. */
	    case '*':
	    {
		if (randint1(2) == 1)
		    cave_set_feat(y, x, FEAT_MAGMA_K);
		else
		    cave_set_feat(y, x, FEAT_QUARTZ_K);
		break;
	    }
	    /* Lava. */
	    case '@':
	    {
		cave_set_feat(y, x, FEAT_LAVA);
		break;
	    }
	    /* Water. */
	    case 'x':
	    {
		cave_set_feat(y, x, FEAT_WATER);
		break;
	    }
	    /* Tree. */
	    case ';':
	    {
		if (randint1(p_ptr->danger + HIGHLAND_TREE_CHANCE)
		    > HIGHLAND_TREE_CHANCE)
		    cave_set_feat(y, x, FEAT_TREE2);
		else
		    cave_set_feat(y, x, FEAT_TREE);
		break;
	    }
	    /* Rubble. */
	    case ':':
	    {
		cave_set_feat(y, x, FEAT_RUBBLE);
		break;
	    }
	    /* Sand dune */
	    case '/':
	    {
		cave_set_feat(y, x, FEAT_DUNE);
		break;
	    }
	    /* Doors */
	    case '+':
	    {
		if (landmark)
		    place_unlocked_door(y, x);
		else
		    place_secret_door(y, x);
		break;
	    }
	    /* Up stairs.  */
	    case '<':
	    {
		if (chunk_list[p_ptr->stage].z_pos > 0)
		    cave_set_feat(y, x, FEAT_LESS);

		break;
	    }
	    /* Down stairs. */
	    case '>':
	    {
		/* No down stairs at bottom or on quests */
		if (is_quest(p_ptr->stage))
//BELE need lowest level || (!stage_map[p_ptr->stage][DOWN]))
		    break;

		cave_set_feat(y, x, FEAT_MORE);
		break;
	    }
	    }

	    /* Analyze again for landmark-specific terrain */
	    if (landmark)
	    {
		switch (*t) 
		{
		    /* Grass */
		case '1':
		{
		    cave_set_feat(y, x, FEAT_GRASS);
		    break;
		}
		/* Road */
		case '2':
		{
		    cave_set_feat(y, x, FEAT_ROAD);
		    break;
		}
		/* Void */
		case '3':
		{
		    cave_set_feat(y, x, FEAT_VOID);
		    break;
		}
		/* Pit */
		case '4':
		{
		    cave_set_feat(y, x, FEAT_PIT);
		    break;
		}
		/* Reed */
		case '5':
		{
		    cave_set_feat(y, x, FEAT_REED);
		    break;
		}
		/* Mountain */
		case '6':
		{
		    cave_set_feat(y, x, FEAT_MTN);
		    break;
		}
		/* Snow */
		case '7':
		{
		    cave_set_feat(y, x, FEAT_SNOW);
		    break;
		}
		/* Battlement */
		case '8':
		{
		    cave_set_feat(y, x, FEAT_BTLMNT);
		    break;
		}
		/* Ice */
		case '9':
		{
		    cave_set_feat(y, x, FEAT_ICE);
		    break;
		}
		}
	    }
	}
    }
}

