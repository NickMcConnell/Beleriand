/** \file gen-wilderness.c 
    \brief Wilderness generation
 
    * Code for creation of wilderness.
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


/** 
 * Number and type of "vaults" in wilderness levels 
 * These need to be set at the start of each wilderness generation routine.
 */
int wild_type = 0;


/**
 * Specific levels on which there should never be a vault
 */
extern bool no_vault(void)
{
    /* No vaults on mountaintops */
    //if (stage_map[p_ptr->stage][STAGE_TYPE] == MOUNTAINTOP)
//	return (TRUE);
// BELE there will be some
    /* No vaults on dungeon entrances */
    //  if ((stage_map[p_ptr->stage][STAGE_TYPE] != CAVE)
//	&& (stage_map[p_ptr->stage][DOWN]))
//	return (TRUE);

    /* Anywhere else is OK */
    return (FALSE);
}



/*** Various wilderness helper routines  ***/


/**
 * Makes "paths to nowhere" from interstage paths toward the middle of the
 * current stage.  Adapted from tunnelling code.
 * BELE left for reference
 */

static coord path_start(int sy, int sx, int ty, int tx)
{

    int fy, fx, y, x, i, row_dir, col_dir;
    coord pathend;

    /* make sure targets are in bounds, reflect back in if not */
    ty += ABS(ty) - ty - ABS(ARENA_HGT - 1 - ty) + (ARENA_HGT - 1 - ty);
    tx += ABS(tx) - tx - ABS(ARENA_WID - 1 - tx) + (ARENA_WID - 1 - tx);

    /* Set last point in case of out of bounds */
    fy = sy;
    fx = sx;

    /* start */
    correct_dir(&row_dir, &col_dir, sy, sx, ty, tx);
    y = sy + row_dir;
    x = sx + col_dir;
    if (in_bounds_fully(y, x)) {
	cave_set_feat(y, x, FEAT_ROAD);
	fy = y;
	fx = x;
    } else {
	pathend.x = fx;
	pathend.y = fy;
	return (pathend);
    }

    /* 100 steps should be enough */
    for (i = 0; i < 50; i++) {
	/* one randomish step... */
	adjust_dir(&row_dir, &col_dir, y, x, ty, tx);
	y += row_dir;
	x += col_dir;
	if (in_bounds_fully(y, x)) {
	    cave_set_feat(y, x, FEAT_ROAD);
	    fy = y;
	    fx = x;
	} else {
	    pathend.x = fx;
	    pathend.y = fy;
	    return (pathend);
	}

	/* ...and one good one */
	correct_dir(&row_dir, &col_dir, y, x, ty, tx);
	y += row_dir;
	x += col_dir;
	if (in_bounds_fully(y, x)) {
	    cave_set_feat(y, x, FEAT_ROAD);
	    fy = y;
	    fx = x;
	} else {
	    pathend.x = fx;
	    pathend.y = fy;
	    return (pathend);
	}

	/* near enough is good enough */
	if ((ABS(x - tx) < 3) && (ABS(y - ty) < 3))
	    break;
    }

    /* return where we have finished */
    pathend.x = x;
    pathend.y = y;
    return (pathend);
}

/** Move the path if it might land in a river */
void river_move(int *xp)
{
    int x = (*xp), diff;

    diff = x - ARENA_WID / 2;
    if (ABS(diff) < 10)
	x = (diff < 0) ? (x - 10) : (x + 10);

    (*xp) = x;
    return;
}

/**
 * Places paths to adjacent surface stages, and joins them.  Does each 
 * direction separately, which is a bit repetitive -NRM-
 */
static void alloc_paths(int stage, int last_stage)
{
//BELE gutted - use FAangband version if necessary
}


/**
 * Make a formation - a randomish group of terrain squares. -NRM-
 * Care probably needed with declaring feat[].
 *
 * As of FAangband 0.2.2, wilderness "vaults" are now made here.  These
 * are less structured than cave vaults or webs; in particular other
 * formations or even "vaults" can bleed into them.
 *
 */

int make_formation(int y, int x, int base_feat1, int base_feat2, int *feat,
		   int prob)
{
    int terrain, j, jj, i = 0, total = 0;
    int *all_feat = malloc(prob * sizeof(*all_feat));
    int ty = y;
    int tx = x;
    feature_type *f_ptr;

    /* Need to make some "wilderness vaults" */
    if (wild_vaults) {
	vault_type *v_ptr;
	int n, yy, xx;
	int v_cnt = 0;
	int *v_idx = malloc(z_info->v_max * sizeof(*v_idx));

	bool good_place = TRUE;

	/* Greater "vault" ? */
	if (randint0(100 - p_ptr->danger) < 9)
	    wild_type += 1;

	/* Examine each "vault" */
	for (n = 0; n < z_info->v_max; n++) {
	    /* Access the "vault" */
	    v_ptr = &v_info[n];

	    /* Accept each "vault" that is acceptable for this location */
	    if ((v_ptr->typ == wild_type) && (v_ptr->min_lev <= p_ptr->danger)
		&& (v_ptr->max_lev >= p_ptr->danger)) {
		v_idx[v_cnt++] = n;
	    }
	}

	/* If none appropriate, cancel vaults for this level */
	if (!v_cnt) {
	    wild_vaults = 0;
	    free(all_feat);
	    free(v_idx);
	    return (0);
	}

	/* Access a random "vault" record */
	v_ptr = &v_info[v_idx[randint0(v_cnt)]];

	/* Check to see if it will fit here (only avoid edges) */
	if ((in_bounds_fully(y - v_ptr->hgt / 2, x - v_ptr->wid / 2))
	    && (in_bounds_fully(y + v_ptr->hgt / 2, x + v_ptr->wid / 2))) {
	    for (yy = y - v_ptr->hgt / 2; yy < y + v_ptr->hgt / 2; yy++)
		for (xx = x - v_ptr->wid / 2; xx < x + v_ptr->wid / 2; xx++) {
		    f_ptr = &f_info[cave_feat[yy][xx]];
		    if ((tf_has(f_ptr->flags, TF_PERMANENT))
			|| (distance(yy, xx, p_ptr->py, p_ptr->px) < 20)
			|| cave_has(cave_info[yy][xx], CAVE_ICKY))
			good_place = FALSE;
		}
	} else
	    good_place = FALSE;

	/* We've found a place */
	if (good_place) {
	    /* Build the "vault" (never lit, icky) */
	    if (!build_vault
		(y, x, v_ptr->hgt, v_ptr->wid, v_ptr->text, FALSE,
		 TRUE, wild_type)) {
		free(all_feat);
		free(v_idx);
		return (0);
	    }

	    /* Message */
	    if (OPT(cheat_room))
		msg("%s. ", v_ptr->name);

	    /* One less to make */
	    wild_vaults--;

	    /* Takes up some space */
	    free(all_feat);
	    free(v_idx);
	    return (v_ptr->hgt * v_ptr->wid);
	}
    }



    /* Extend the array of terrain types to length prob */
    jj = 0;
    for (j = 0; j < prob - 1; j++) {
	if (feat[jj] == FEAT_NONE)
	    jj = 0;
	all_feat[j] = feat[jj];
	jj++;
    }

    /* Make a formation */
    while (i != (prob - 1)) {
	/* Avoid paths, stay in bounds */
	if (((cave_feat[ty][tx] != base_feat1)
	     && (cave_feat[ty][tx] != base_feat2)) || !(in_bounds_fully(ty, tx))
	    || cave_has(cave_info[ty][tx], CAVE_ICKY)) {
	    free(all_feat);
	    return (total);
	}

	/* Check for treasure */
	if ((all_feat[i] == FEAT_MAGMA) && (randint0(DUN_STR_MC) == 0))
	    all_feat[i] = FEAT_MAGMA_K;
	else if ((all_feat[i] == FEAT_QUARTZ) && (randint0(DUN_STR_QC) == 0))
	    all_feat[i] = FEAT_QUARTZ_K;

	/* Set the feature */
	cave_set_feat(ty, tx, all_feat[i]);
	cave_on(cave_info[ty][tx], CAVE_ICKY);

	/* Choose a random step for next feature, try to keep going */
	terrain = randint0(8) + 1;
	if (terrain > 4)
	    terrain++;
	for (j = 0; j < 100; j++) {
	    ty += ddy[terrain];
	    tx += ddx[terrain];
	    if (!cave_has(cave_info[ty][tx], CAVE_ICKY))
		break;
	}

	/* Count */
	total++;

	/* Pick the next terrain, or finish */
	i = randint0(prob);
    }

    free(all_feat);
    return (total);
}

/**
 * Find a given generation location in the list, or failing that the one
 * after it
 */
bool gen_loc_find(int x_pos, int y_pos, int z_pos, int *lower, int *upper)
{
    int idx = gen_loc_cnt / 2;

    /* Special case for before the array is populated */
    if (gen_loc_cnt == 0)
    {
	*upper = *lower = 0;
	return FALSE;
    }

    *upper = gen_loc_cnt;
    *lower = 0;

    while ((gen_loc_list[idx].x_pos != x_pos) ||
	   (gen_loc_list[idx].y_pos != y_pos) ||
	   (gen_loc_list[idx].z_pos != z_pos))
    {
	if (*lower + 1 == *upper)
	    break;

	if (gen_loc_list[idx].x_pos > x_pos)
	{
	    *upper = idx;
	    idx = (*upper + *lower) / 2;
	    continue;
	}
	else if (gen_loc_list[idx].x_pos < x_pos)
	{
	    *lower = idx;
	    idx = (*upper + *lower) / 2;
	    continue;
	}
	else if (gen_loc_list[idx].y_pos > y_pos)
	{
	    *upper = idx;
	    idx = (*upper + *lower) / 2;
	    continue;
	}
	else if (gen_loc_list[idx].y_pos < y_pos)
	{
	    *lower = idx;
	    idx = (*upper + *lower) / 2;
	    continue;
	}
	else if (gen_loc_list[idx].z_pos > z_pos)
	{
	    *upper = idx;
	    idx = (*upper + *lower) / 2;
	    continue;
	}
	else if (gen_loc_list[idx].z_pos < z_pos)
	{
	    *lower = idx;
	    idx = (*upper + *lower) / 2;
	    continue;
	}
    }

    /* Found without having to break */
    if (*lower + 1 != *upper)
    {
	*lower = idx;
	*upper = idx;
	return TRUE;
    }

    /* Check lower */
    if ((gen_loc_list[*lower].x_pos == x_pos) &&
	(gen_loc_list[*lower].y_pos == y_pos) &&
	(gen_loc_list[*lower].z_pos == z_pos))
    {
	*upper = *lower;
	return TRUE;
    }

    /* Needs to go after the last element */
    if ((gen_loc_list[*upper].x_pos <= x_pos) &&
	(gen_loc_list[*upper].y_pos <= y_pos) &&
	(gen_loc_list[*upper].z_pos <= z_pos))
    {
	*upper = gen_loc_cnt;
	return FALSE;
    }

    /* Needs to go before the first element */
    if ((gen_loc_list[*lower].x_pos >= x_pos) &&
	(gen_loc_list[*lower].y_pos >= y_pos) &&
	(gen_loc_list[*lower].z_pos >= z_pos))
    {
	*upper = 0;
	return FALSE;
    }

    /* Needs to go between upper and lower */
    return FALSE;

}

/**
 * Find a given generation location in the list, or failing that the one
 * after it
 */
void gen_loc_make(int x_pos, int y_pos, int z_pos, int lower, int upper)
{
    int i;

    /* Increase the count, extend the array if necessary */
    gen_loc_cnt++;
    if ((gen_loc_cnt % GEN_LOC_INCR) == 0)
    {
	gen_loc_max += GEN_LOC_INCR;
	gen_loc_list = mem_realloc(gen_loc_list, gen_loc_max * sizeof(gen_loc));
    }

    /* Move everything along one to make space */
    for (i = gen_loc_cnt; i > upper; i--)
	memcpy(&gen_loc_list[i], &gen_loc_list[i - 1], sizeof(gen_loc));

    /* Copy the new data in */
    gen_loc_list[upper].x_pos = x_pos;
    gen_loc_list[upper].y_pos = y_pos;
    gen_loc_list[upper].z_pos = z_pos;
    gen_loc_list[upper].change = NULL;
    gen_loc_list[upper].effect = NULL;
}

extern void plain_gen(chunk_ref ref, int y_offset, int x_offset, 
		      edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_GRASS);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void forest_gen(chunk_ref ref, int y_offset, int x_offset, 
		       edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_TREE);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void ocean_gen(chunk_ref ref, int y_offset, int x_offset, 
		      edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_WATER);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void lake_gen(chunk_ref ref, int y_offset, int x_offset, 
		     edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_WATER);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void moor_gen(chunk_ref ref, int y_offset, int x_offset, 
		     edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_GRASS);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void mtn_gen(chunk_ref ref, int y_offset, int x_offset, 
		    edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_MTN);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void swamp_gen(chunk_ref ref, int y_offset, int x_offset, 
		      edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_REED);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void dark_gen(chunk_ref ref, int y_offset, int x_offset, 
		     edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_TREE);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void impass_gen(chunk_ref ref, int y_offset, int x_offset, 
		       edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_MTN);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void desert_gen(chunk_ref ref, int y_offset, int x_offset, 
		       edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_DUNE);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void snow_gen(chunk_ref ref, int y_offset, int x_offset, 
		     edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_SNOW);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void town_gen(chunk_ref ref, int y_offset, int x_offset, 
		     edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_ROAD);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}

extern void landmk_gen(chunk_ref ref, int y_offset, int x_offset, 
		       edge_effect *first)
{
    int x, y;
    int y0 = y_offset * CHUNK_HGT;
    int x0 = x_offset * CHUNK_WID;

    /* Write the location stuff */
    for (y = 0; y < CHUNK_HGT; y++)
    {
	for (x = 0; x < CHUNK_HGT; x++)
	{
	    /* Terrain */
	    cave_set_feat(y0 + y, x0 + x, FEAT_ROAD);
	}
    }

    if (!character_dungeon)
	player_place(ARENA_HGT/2, ARENA_WID/2);
}


/**
 * Generate a new plain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void plain_gen_old(void)
{
    int i, j, k, y, x;
    int stage = p_ptr->stage;
    int last_stage = p_ptr->last_stage;
    int form_grids = 0;

    int form_feats[8] = { FEAT_TREE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID,
			  FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE
    };
    int ponds[2] = { FEAT_WATER, FEAT_NONE };

    bool dummy;
    feature_type *f_ptr;

    /* Hack -- Start with basic grass */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create grass */
	    cave_feat[y][x] = FEAT_GRASS;
	}
    }


    /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
    alloc_paths(stage, last_stage);

    /* Special boundary walls -- Top */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = 0; y < i; y++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }


    /* Special boundary walls -- Bottom */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = ARENA_HGT - 1; y > ARENA_HGT - 1 - i; y--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Left */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = 0; x < i; x++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Right */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = ARENA_WID - 1; x > ARENA_WID - 1 - i; x--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Place some formations */
    while (form_grids < (50 * p_ptr->danger + 1000)) {
	/* Set the "vault" type */
	wild_type = ((randint0(5) == 0) ? 26 : 14);

	/* Choose a place */
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids +=
	    make_formation(y, x, FEAT_GRASS, FEAT_GRASS, form_feats,
			   p_ptr->danger + 1);
    }

    /* And some water */
    form_grids = 0;
    while (form_grids < 300) {
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids += make_formation(y, x, FEAT_GRASS, FEAT_GRASS, ponds, 10);
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }

    /* Basic "amount" */
    k = (p_ptr->danger / 2);

    /* Gets hairy north of the mountains */
    if (p_ptr->danger > 40)
	k += 10;


    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k / 2));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);

	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1)) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }
}

void mtn_connect(int y, int x, int y1, int x1)
{
    u16b gp[512];
    int path_grids, j;

    /* Find the shortest path */
    path_grids = project_path(gp, 512, y, x, y1, x1, TRUE);

    /* Make the path, adding an adjacent grid 8/9 of the time */
    for (j = 0; j < path_grids; j++) {
	if ((cave_feat[GRID_Y(gp[j])][GRID_X(gp[j])] == FEAT_ROAD)
	    || (!in_bounds_fully(GRID_Y(gp[j]), GRID_X(gp[j]))))
	    break;
	cave_set_feat(GRID_Y(gp[j]), GRID_X(gp[j]), FEAT_ROAD);
	cave_on(cave_info[GRID_Y(gp[j])][GRID_X(gp[j])], CAVE_ICKY);
    }
}

/**
 * Generate a new mountain level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void mtn_gen_old(void)
{
    bool made_plat;

    int i, j, k, y, x;
    int plats, a, b;
    int stage = p_ptr->stage;
    int last_stage = p_ptr->last_stage;
    int form_grids = 0;
    int min, dist, floors = 0;
    int randpoints[20];
    coord pathpoints[20];
    coord nearest_point = { ARENA_HGT / 2, ARENA_WID / 2 };
    coord stairs[3];

    /* Amusing hack to make paths work */
    int form_feats[8] =
	{ FEAT_DOOR_HEAD, FEAT_DOOR_HEAD + 1, FEAT_DOOR_HEAD + 2,
	  FEAT_DOOR_HEAD + 3, FEAT_DOOR_HEAD + 4, FEAT_NONE
	};

    bool dummy;
    bool amon_rudh = FALSE;


    /* Hack -- Start with basic grass (lets paths work -NRM-) */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create grass */
	    cave_feat[y][x] = FEAT_GRASS;
	}
    }


    /* Special boundary walls -- Top */
    for (x = 0; x < ARENA_WID; x++) {
	y = 0;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Special boundary walls -- Bottom */
    for (x = 0; x < ARENA_WID; x++) {
	y = ARENA_HGT - 1;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Special boundary walls -- Left */
    for (y = 0; y < ARENA_HGT; y++) {
	x = 0;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Special boundary walls -- Right */
    for (y = 0; y < ARENA_HGT; y++) {
	x = ARENA_WID - 1;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Place 2 or 3 paths to neighbouring stages, make the paths through the
     * stage, place the player -NRM- */
    alloc_paths(stage, last_stage);

    /* Dungeon entrance */
    if ((stage_map[stage][DOWN])
	&& (stage_map[stage_map[stage][DOWN]][LOCALITY] != UNDERWORLD)) {
	/* Set the flag */
	amon_rudh = TRUE;

	/* Mim's cave on Amon Rudh */
	i = 3;
	while (i) {
	    y = randint0(ARENA_HGT - 2) + 1;
	    x = randint0(ARENA_WID - 2) + 1;
	    if ((cave_feat[y][x] == FEAT_ROAD)
		|| (cave_feat[y][x] == FEAT_GRASS)) {
		cave_set_feat(y, x, FEAT_MORE);
		i--;
		stairs[2 - i].y = y;
		stairs[2 - i].x = x;
		if ((i == 0)
		    && (stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE))
		    player_place(y, x);
	    }
	}
    }


    /* Make paths permanent */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++)
	    if (cave_feat[y][x] == FEAT_ROAD) {
		/* Hack - prepare for plateaux, connecting */
		cave_on(cave_info[y][x], CAVE_ICKY);
		floors++;
	    }
    }

    /* Pick some joining points */
    for (j = 0; j < 20; j++)
	randpoints[j] = randint0(floors);
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    if (cave_feat[y][x] == FEAT_ROAD)
		floors--;
	    else
		continue;
	    for (j = 0; j < 20; j++) {
		if (floors == randpoints[j]) {
		    pathpoints[j].y = y;
		    pathpoints[j].x = x;
		}
	    }
	}
    }

    /* Find the staircases, if any */
    if (amon_rudh) {
	for (j = 0; j < 3; j++) {
	    y = stairs[j].y;
	    x = stairs[j].x;

	    /* Now join them up */
	    min = ARENA_WID + ARENA_HGT;
	    for (i = 0; i < 20; i++) {
		dist = distance(y, x, pathpoints[i].y, pathpoints[i].x);
		if (dist < min) {
		    min = dist;
		    nearest_point = pathpoints[i];
		}
	    }
	    mtn_connect(y, x, nearest_point.y, nearest_point.x);
	}
    }

    /* Make a few "plateaux" */
    plats = rand_range(2, 4);

    /* Try fairly hard */
    for (j = 0; j < 50; j++) {
	/* Try for a plateau */
	a = randint0(6) + 4;
	b = randint0(5) + 4;
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	made_plat =
	    generate_starburst_room(y - b, x - a, y + b, x + a, FALSE,
				    FEAT_DOOR_HEAD + 2, TRUE);

	/* Success ? */
	if (made_plat) {
	    plats--;

	    /* Now join it up */
	    min = ARENA_WID + ARENA_HGT;
	    for (i = 0; i < 20; i++) {
		dist = distance(y, x, pathpoints[i].y, pathpoints[i].x);
		if (dist < min) {
		    min = dist;
		    nearest_point = pathpoints[i];
		}
	    }
	    mtn_connect(y, x, nearest_point.y, nearest_point.x);
	}


	/* Done ? */
	if (!plats)
	    break;
    }



    while (form_grids < 50 * (p_ptr->danger)) {
	/* Set the "vault" type */
	wild_type = ((randint0(5) == 0) ? 26 : 16);

	/* Choose a place */
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids +=
	    make_formation(y, x, FEAT_GRASS, FEAT_GRASS, form_feats,
			   p_ptr->danger * 2);
	/* Now join it up */
	min = ARENA_WID + ARENA_HGT;
	for (i = 0; i < 20; i++) {
	    dist = distance(y, x, pathpoints[i].y, pathpoints[i].x);
	    if (dist < min) {
		min = dist;
		nearest_point = pathpoints[i];
	    }
	}
	mtn_connect(y, x, nearest_point.y, nearest_point.x);

    }

    /* Now change all the terrain to what we really want */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create grass */
	    switch (cave_feat[y][x]) {
	    case FEAT_GRASS:
	    {
		cave_set_feat(y, x, FEAT_WALL_SOLID);
		break;
	    }
	    case FEAT_DOOR_HEAD:
	    {
		cave_set_feat(y, x, FEAT_RUBBLE);
		break;
	    }
	    case FEAT_DOOR_HEAD + 1:
	    {
		cave_set_feat(y, x, FEAT_MAGMA);
		break;
	    }
	    case FEAT_DOOR_HEAD + 2:
	    {
		cave_set_feat(y, x, FEAT_GRASS);
		break;
	    }
	    case FEAT_DOOR_HEAD + 3:
	    {
		if (randint1(p_ptr->danger + HIGHLAND_TREE_CHANCE)
		    > HIGHLAND_TREE_CHANCE)
		    cave_set_feat(y, x, FEAT_TREE2);
		else
		    cave_set_feat(y, x, FEAT_TREE);
		break;
	    }
	    case FEAT_DOOR_HEAD + 4:
	    {
		cave_set_feat(y, x, FEAT_ROAD);
		break;
	    }
	    }
	}
    }


    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);

	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1)) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }




    /* Basic "amount" */
    k = (p_ptr->danger / 2);

    /* Gets hairy north of the mountains */
    if (p_ptr->danger > 40)
	k += 10;

    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	}
    }
}

/**
 * Generate a new mountaintop level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 */
extern void mtntop_gen_old(void)
{
    bool made_plat;

    int i, j, k, y, x, y1, x1;
    int plats, a, b;
    int spot, floors = 0;
    bool placed = FALSE;

    /* Hack -- Start with void */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create void */
	    cave_feat[y][x] = FEAT_VOID;
	}
    }


    /* Special boundary walls -- Top */
    for (x = 0; x < ARENA_WID; x++) {
	y = 0;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Special boundary walls -- Bottom */
    for (x = 0; x < ARENA_WID; x++) {
	y = ARENA_HGT - 1;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Special boundary walls -- Left */
    for (y = 0; y < ARENA_HGT; y++) {
	x = 0;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Special boundary walls -- Right */
    for (y = 0; y < ARENA_HGT; y++) {
	x = ARENA_WID - 1;

	/* Clear previous contents, add "solid" perma-wall */
	cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Make the main mountaintop */
    while (!placed) {
	a = randint0(6) + 4;
	b = randint0(5) + 4;
	y = ARENA_HGT / 2;
	x = ARENA_WID / 2;
	placed =
	    generate_starburst_room(y - b, x - a, y + b, x + a, FALSE,
				    FEAT_ROAD, FALSE);
    }

    /* Summit */
    for (i = -1; i <= 1; i++) {
	cave_feat[y + i][x] = FEAT_WALL_SOLID;
	cave_feat[y][x + i] = FEAT_WALL_SOLID;
    }

    /* Count the floors */
    for (y1 = y - b; y1 < y + b; y1++)
	for (x1 = x - a; x1 < x + a; x1++)
	    if (cave_feat[y1][x1] == FEAT_ROAD)
		floors++;

    /* Choose the player place */
    spot = randint0(floors);

    /* Can we get down? */
    if (randint0(2) == 0) {
	y1 = rand_range(y - b, y + b);
	if (cave_feat[y1][x] != FEAT_VOID) {
	    i = randint0(2);
	    if (i == 0)
		i = -1;
	    for (x1 = x; x1 != (x + i * (a + 1)); x1 += i)
		if (cave_feat[y1][x1] == FEAT_VOID)
		    break;
	    cave_set_feat(y1, x1, FEAT_MORE);
	}
    }


    /* Adjust the terrain, place the player */
    for (y1 = y - b; y1 < y + b; y1++)
	for (x1 = x - a; x1 < x + a; x1++) {
	    /* Only change generated stuff */
	    if (cave_feat[y1][x1] == FEAT_VOID)
		continue;

	    /* Leave rock */
	    if (cave_feat[y1][x1] == FEAT_WALL_SOLID)
		continue;

	    /* Leave stair */
	    if (cave_feat[y1][x1] == FEAT_MORE)
		continue;

	    /* Place the player? */
	    if (cave_feat[y1][x1] == FEAT_ROAD) {
		floors--;
		if (floors == spot) {
		    player_place(y1, x1);
		    cave_on(cave_info[y1][x1], CAVE_ICKY);
		    continue;
		}
	    }

	    /* Place some rock */
	    if (randint0(10) < 2) {
		cave_set_feat(y1, x1, FEAT_WALL_SOLID);
		continue;
	    }

	    /* rubble */
	    if (randint0(8) == 0) {
		cave_set_feat(y1, x1, FEAT_RUBBLE);
		continue;
	    }

	    /* and the odd tree */
	    if (randint0(20) == 0) {
		cave_set_feat(y1, x1, FEAT_TREE2);
		continue;
	    }
	}

    /* Make a few "plateaux" */
    plats = randint0(4);

    /* Try fairly hard */
    for (j = 0; j < 10; j++) {
	/* Try for a plateau */
	a = randint0(6) + 4;
	b = randint0(5) + 4;
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	made_plat =
	    generate_starburst_room(y - b, x - a, y + b, x + a, FALSE,
				    FEAT_ROAD, FALSE);

	/* Success ? */
	if (made_plat) {
	    plats--;

	    /* Adjust the terrain a bit */
	    for (y1 = y - b; y1 < y + b; y1++)
		for (x1 = x - a; x1 < x + a; x1++) {
		    /* Only change generated stuff */
		    if (cave_feat[y1][x1] == FEAT_VOID)
			continue;

		    /* Place some rock */
		    if (randint0(10) < 2) {
			cave_set_feat(y1, x1, FEAT_WALL_SOLID);
			continue;
		    }

		    /* rubble */
		    if (randint0(8) == 0) {
			cave_set_feat(y1, x1, FEAT_RUBBLE);
			continue;
		    }

		    /* and the odd tree */
		    if (randint0(20) == 0) {
			cave_set_feat(y1, x1, FEAT_TREE2);
			continue;
		    }
		}

	}

	/* Done ? */
	if (!plats)
	    break;
    }


    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);

	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1)) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }




    /* Basic "amount" */
    k = p_ptr->danger;

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = k; j > 0; j--) {
	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Put some objects in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	}
    }
}

/**
 * Generate a new forest level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void forest_gen_old(void)
{
    bool made_plat;

    int i, j, k, y, x;
    int plats, a, b;
    int stage = p_ptr->stage;
    int last_stage = p_ptr->last_stage;
    int form_grids = 0;

    int form_feats[8] = { FEAT_GRASS, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID,
			  FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE
    };
    int ponds[2] = { FEAT_WATER, FEAT_NONE };

    bool dummy;
    feature_type *f_ptr;




    /* Hack -- Start with basic grass so paths work */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create grass */
	    cave_feat[y][x] = FEAT_GRASS;
	}
    }


    /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
    alloc_paths(stage, last_stage);

    /* Special boundary walls -- Top */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = 0; y < i; y++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }


    /* Special boundary walls -- Bottom */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = ARENA_HGT - 1; y > ARENA_HGT - 1 - i; y--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Left */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = 0; x < i; x++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Right */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = ARENA_WID - 1; x > ARENA_WID - 1 - i; x--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Now place trees */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create trees */
	    if (cave_feat[y][x] == FEAT_GRASS) {
		if (randint1(p_ptr->danger + HIGHLAND_TREE_CHANCE)
		    > HIGHLAND_TREE_CHANCE)
		    cave_set_feat(y, x, FEAT_TREE2);
		else
		    cave_set_feat(y, x, FEAT_TREE);
	    } else
		/* Hack - prepare for clearings */
		cave_on(cave_info[y][x], CAVE_ICKY);

	    /* Mega hack - remove paths if emerging from Nan Dungortheb */
	    if ((last_stage == q_list[2].stage)
		&& (cave_feat[y][x] == FEAT_MORE_NORTH))
		cave_set_feat(y, x, FEAT_GRASS);
	}
    }

    /* Make a few clearings */
    plats = rand_range(2, 4);

    /* Try fairly hard */
    for (j = 0; j < 50; j++) {
	/* Try for a clearing */
	a = randint0(6) + 4;
	b = randint0(5) + 4;
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	made_plat =
	    generate_starburst_room(y - b, x - a, y + b, x + a, FALSE,
				    FEAT_GRASS, TRUE);

	/* Success ? */
	if (made_plat)
	    plats--;

	/* Done ? */
	if (!plats)
	    break;
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }


    /* Place some formations */
    while (form_grids < (50 * p_ptr->danger + 1000)) {
	/* Set the "vault" type */
	wild_type = ((randint0(5) == 0) ? 26 : 18);

	/* Choose a place */
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids +=
	    make_formation(y, x, FEAT_TREE, FEAT_TREE2, form_feats,
			   p_ptr->danger + 1);
    }

    /* And some water */
    form_grids = 0;
    while (form_grids < 300) {
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids += make_formation(y, x, FEAT_TREE, FEAT_TREE2, ponds, 10);
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }

    /* Basic "amount" */
    k = (p_ptr->danger / 2);

    /* Gets hairy north of the mountains */
    if (p_ptr->danger > 40)
	k += 10;

    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1))
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}

/**
 * Generate a new swamp level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void swamp_gen_old(void)
{
    int i, j, k, y, x;
    int stage = p_ptr->stage;
    int last_stage = p_ptr->last_stage;
    int form_grids = 0;


    int form_feats[8] = { FEAT_TREE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID,
			  FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE
    };

    bool dummy;
    feature_type *f_ptr;




    /* Hack -- Start with grass */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_feat[y][x] = FEAT_GRASS;
	}
    }


    /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
    alloc_paths(stage, last_stage);

    /* Special boundary walls -- Top */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = 0; y < i; y++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }


    /* Special boundary walls -- Bottom */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = ARENA_HGT - 1; y > ARENA_HGT - 1 - i; y--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Left */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = 0; x < i; x++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Right */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = ARENA_WID - 1; x > ARENA_WID - 1 - i; x--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Hack -- add water */
    for (y = 1; y < ARENA_HGT - 1; y++) {
	for (x = 1; x < ARENA_WID - 1; x++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    if (tf_has(f_ptr->flags, TF_PERMANENT))
		continue;
	    if (((p_ptr->py == y) && (p_ptr->px == x)) || (randint0(100) < 50))
		cave_set_feat(y, x, FEAT_GRASS);
	    else
		cave_set_feat(y, x, FEAT_WATER);
	}
    }


    /* Place some formations (but not many, and less for more danger) */
    while (form_grids < 20000 / p_ptr->danger) {
	/* Set the "vault" type */
	wild_type = ((randint0(5) == 0) ? 26 : 20);

	/* Choose a place */
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids +=
	    make_formation(y, x, FEAT_GRASS, FEAT_WATER, form_feats,
			   p_ptr->danger);
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }

    /* Basic "amount" */
    k = (p_ptr->danger / 2);

    /* Gets hairy north of the mountains */
    if (p_ptr->danger > 40)
	k += 10;

    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1))
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}

/**
 * Generate a new desert level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void desert_gen_old(void)
{
    bool made_plat;

    int i, j, k, y, x, d;
    int plats, a, b;
    int stage = p_ptr->stage;
    int last_stage = p_ptr->last_stage;
    int form_grids = 0;

    int form_feats[8] = { FEAT_GRASS, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID,
			  FEAT_DUNE, FEAT_QUARTZ, FEAT_NONE
    };
    bool dummy;
    bool made_gate = FALSE;
    feature_type *f_ptr;



    /* Hack -- Start with basic grass so paths work */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create grass */
	    cave_feat[y][x] = FEAT_GRASS;
	}
    }


    /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
    alloc_paths(stage, last_stage);

    /* Special boundary walls -- Top */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = 0; y < i; y++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }


    /* Special boundary walls -- Bottom */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = ARENA_HGT - 1; y > ARENA_HGT - 1 - i; y--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Left */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = 0; x < i; x++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Right */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = ARENA_WID - 1; x > ARENA_WID - 1 - i; x--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Dungeon entrance */
    if ((stage_map[stage][DOWN])
	&& (stage_map[stage_map[stage][DOWN]][LOCALITY] != UNDERWORLD)) {
	/* Hack - no vaults */
	wild_vaults = 0;

	/* Angband! */
	for (d = 0; d < ARENA_WID; d++) {
	    for (y = 0; y < d; y++) {
		x = d - y;
		if (!in_bounds_fully(y, x))
		    continue;
		if (cave_feat[y][x] == FEAT_ROAD) {
		    /* The gate of Angband */
		    cave_set_feat(y, x, FEAT_MORE);
		    made_gate = TRUE;
		    if ((stage_map[p_ptr->last_stage][STAGE_TYPE] == CAVE)
			|| (turn < 10))
			player_place(y, x);
		    break;
		} else {
		    /* The walls of Thangorodrim */
		    cave_set_feat(y, x, FEAT_WALL_SOLID);
		}
	    }
	    if (made_gate)
		break;
	}
    }

    /* Now place rubble, sand and magma */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create desert */
	    if (cave_feat[y][x] == FEAT_GRASS) {
		if (randint0(100) < 50)
		    cave_set_feat(y, x, FEAT_DUNE);
		else if (randint0(100) < 50)
		    cave_set_feat(y, x, FEAT_RUBBLE);
		else
		    cave_set_feat(y, x, FEAT_MAGMA);
	    } else
		/* Hack - prepare for clearings */
		cave_on(cave_info[y][x], CAVE_ICKY);
	}
    }

    /* Make a few clearings */
    plats = rand_range(2, 4);

    /* Try fairly hard */
    for (j = 0; j < 50; j++) {
	/* Try for a clearing */
	a = randint0(6) + 4;
	b = randint0(5) + 4;
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	made_plat =
	    generate_starburst_room(y - b, x - a, y + b, x + a, FALSE,
				    FEAT_GRASS, TRUE);

	/* Success ? */
	if (made_plat)
	    plats--;

	/* Done ? */
	if (!plats)
	    break;
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }


    /* Place some formations */
    while (form_grids < 20 * p_ptr->danger) {
	/* Set the "vault" type */
	wild_type = ((randint0(5) == 0) ? 26 : 22);

	/* Choose a place */
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids +=
	    make_formation(y, x, FEAT_RUBBLE, FEAT_MAGMA, form_feats,
			   p_ptr->danger);
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }

    /* Basic "amount" */
    k = (p_ptr->danger / 2);

    /* Gets hairy north of the mountains */
    if (p_ptr->danger > 40)
	k += 10;

    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1))
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}


/**
 * Generate a new river level. Place stairs, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void river_gen_old(void)
{
    int i, j, k, y, x, y1 = ARENA_HGT / 2;
    int mid[ARENA_HGT];
    int stage = p_ptr->stage;
    int last_stage = p_ptr->last_stage;
    int form_grids = 0;
    int path;

    int form_feats[8] = { FEAT_TREE, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID,
			  FEAT_TREE2, FEAT_QUARTZ, FEAT_NONE
    };

    bool dummy;
    feature_type *f_ptr;




    /* Hack -- Start with basic grass */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create grass */
	    cave_feat[y][x] = FEAT_GRASS;
	}
    }


    /* Place 2 or 3 paths to neighbouring stages, place player -NRM- */
    alloc_paths(stage, last_stage);

    /* Hack - remember the path in case it has to move */
    path = cave_feat[p_ptr->py][p_ptr->px];

    /* Special boundary walls -- Top */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = 0; y < i; y++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }


    /* Special boundary walls -- Bottom */
    i = 4;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 7)
	    i = 7;
	if (i < 0)
	    i = 0;
	for (y = ARENA_HGT - 1; y > ARENA_HGT - 1 - i; y--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Left */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = 0; x < i; x++) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Special boundary walls -- Right */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = ARENA_WID - 1; x > ARENA_WID - 1 - i; x--) {
	    f_ptr = &f_info[cave_feat[y][x]];

	    /* Clear previous contents, add "solid" perma-wall */
	    if ((cave_feat[y][x] != FEAT_ROAD)
		&& !(tf_has(f_ptr->flags, TF_PERMANENT))) {
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	    }
	}
    }

    /* Place the river, start in the middle third */
    i = ARENA_WID / 3 + randint0(ARENA_WID / 3);
    for (y = 1; y < ARENA_HGT - 1; y++) {
	/* Remember the midpoint */
	mid[y] = i;

	for (x = i - randint0(5) - 10; x < i + randint0(5) + 10; x++) {
	    /* Make the river */
	    cave_set_feat(y, x, FEAT_WATER);
	    cave_on(cave_info[y][x], CAVE_ICKY);
	}
	/* Meander */
	i += randint0(3) - 1;
    }

    /* Place some formations */
    while (form_grids < 50 * p_ptr->danger + 1000) {
	/* Set the "vault" type */
	wild_type = ((randint0(5) == 0) ? 26 : 24);

	/* Choose a place */
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;

	form_grids +=
	    make_formation(y, x, FEAT_GRASS, FEAT_GRASS, form_feats,
			   p_ptr->danger / 2);
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }

    /* Hack - move the player out of the river */
    y = p_ptr->py;
    x = p_ptr->px;
    while ((cave_feat[p_ptr->py][p_ptr->px] == FEAT_WATER)
	   || (cave_feat[p_ptr->py][p_ptr->px] == FEAT_PERM_SOLID))
	p_ptr->px++;

    /* Place player if they had to move */
    if (x != p_ptr->px) {
	cave_m_idx[p_ptr->py][p_ptr->px] = -1;
	cave_m_idx[y][x] = 0;
	cave_set_feat(y, x, path);
	for (y = p_ptr->py; y > 0; y--)
	    if (tf_has(f_info[cave_feat[y][p_ptr->px]].flags, TF_WALL))
		cave_set_feat(y, p_ptr->px, FEAT_ROAD);
    }

    /* Basic "amount" */
    k = (p_ptr->danger / 2);

    /* Gets hairy north of the mountains */
    if (p_ptr->danger > 40)
	k += 10;

    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));


    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1))
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}

/**
 * Attempt to place a web of the required type
 */
bool place_web(int type)
{
    vault_type *v_ptr;
    int i, y, x = ARENA_WID / 2, cy, cx;
    int *v_idx = malloc(z_info->v_max * sizeof(v_idx));
    int v_cnt = 0;

    bool no_good = FALSE;

    /* Examine each web */
    for (i = 0; i < z_info->v_max; i++) {
	/* Access the web */
	v_ptr = &v_info[i];

	/* Accept each web that is acceptable for this depth. */
	if ((v_ptr->typ == type) && (v_ptr->min_lev <= p_ptr->danger)
	    && (v_ptr->max_lev >= p_ptr->danger)) {
	    v_idx[v_cnt++] = i;
	}
    }

    /* None to be found */
    if (v_cnt == 0) {
	free(v_idx);
	return (FALSE);
    }

    /* Access a random vault record */
    v_ptr = &v_info[v_idx[randint0(v_cnt)]];

    /* Look for somewhere to put it */
    for (i = 0; i < 25; i++) {
	/* Random top left corner */
	cy = randint1(ARENA_HGT - 1 - v_ptr->hgt);
	cx = randint1(ARENA_WID - 1 - v_ptr->wid);

	/* Check to see if it will fit (only avoid big webs and edges) */
	for (y = cy; y < cy + v_ptr->hgt; y++)
	    for (x = cx; x < cx + v_ptr->wid; x++)
		if ((cave_feat[y][x] == FEAT_VOID)
		    || (cave_feat[y][x] == FEAT_PERM_SOLID)
		    || (cave_feat[y][x] == FEAT_MORE_SOUTH) || 
		    ((y == p_ptr->py) && (x == p_ptr->px))
		    || cave_has(cave_info[y][x], CAVE_ICKY))
		    no_good = TRUE;

	/* Try again, or stop if we've found a place */
	if (no_good) {
	    no_good = FALSE;
	    continue;
	} else
	    break;
    }

    /* Give up if we couldn't find anywhere */
    if (no_good) {
	free(v_idx);
	return (FALSE);
    }

    /* Build the vault (never lit, not icky unless full size) */
    if (!build_vault
	(y, x, v_ptr->hgt, v_ptr->wid, v_ptr->text, FALSE,
	 (type == 13), type)) {
	free(v_idx);
	return (FALSE);
    }

    free(v_idx);
    return (TRUE);
}





/**
 * Generate a new valley level. Place down slides, 
 * and random monsters, objects, and traps.  Place any quest monsters.
 *
 * We mark grids "temp" to prevent random monsters being placed there.
 * 
 * No rooms outside the dungeons (for now, at least) -NRM
 */
extern void valley_gen_old(void)
{
    bool made_plat;

    int i, j, k, y, x;
    int plats, a, b, num = 2;
    int form_grids = 0;
    int path_x[3];
    int form_feats[8] = { FEAT_GRASS, FEAT_RUBBLE, FEAT_MAGMA, FEAT_WALL_SOLID,
			  FEAT_GRASS, FEAT_QUARTZ, FEAT_NONE
    };
    bool dummy;


    /* Hack -- Start with trees */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    /* Create trees */
	    if (randint1(p_ptr->danger + HIGHLAND_TREE_CHANCE)
		> HIGHLAND_TREE_CHANCE)
		cave_set_feat(y, x, FEAT_TREE2);
	    else
		cave_set_feat(y, x, FEAT_TREE);
	}
    }

    /* Prepare places for down slides */
    num += randint0(2);
    for (i = 0; i < num; i++)
	path_x[i] = 1 + randint0(ARENA_WID / num - 2) + i * ARENA_WID / num;

    /* Special boundary walls -- Top */
    i = 5;
    for (x = 0; x < ARENA_WID; x++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (y = 0; y < i; y++)

	    /* Clear previous contents, add "solid" perma-wall */
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
	if ((x > 0) && (x == p_ptr->path_coord)) {
	    if (y == 0)
		y++;
	    cave_set_feat(y, x, FEAT_RUBBLE);
	    player_place(y, x);
	}
    }


    /* Special boundary walls -- Bottom */
    i = 5;
    j = 0;
    if (p_ptr->danger != 70) {
	for (x = 0; x < ARENA_WID; x++) {
	    i += 1 - randint0(3);
	    if (i > 10)
		i = 10;
	    if (i < 0)
		i = 0;
	    for (y = ARENA_HGT - 1; y > ARENA_HGT - 1 - i; y--)

		/* Clear previous contents, add empty space */
		cave_set_feat(y, x, FEAT_VOID);

	    /* Down slides */
	    if (j < num)
		if (x == path_x[j]) {
		    cave_set_feat(y, x, FEAT_MORE_SOUTH);
		    j++;
		}
	}
    }

    /* Special boundary walls -- Left */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = 0; x < i; x++)

	    /* Clear previous contents, add "solid" perma-wall */
	    cave_set_feat(y, x, FEAT_PERM_SOLID);

    }

    /* Special boundary walls -- Right */
    i = 5;
    for (y = 0; y < ARENA_HGT; y++) {
	i += 1 - randint0(3);
	if (i > 10)
	    i = 10;
	if (i < 0)
	    i = 0;
	for (x = ARENA_WID - 1; x > ARENA_WID - 1 - i; x--)

	    /* Clear previous contents, add "solid" perma-wall */
	    cave_set_feat(y, x, FEAT_PERM_SOLID);
    }

    /* Make a few clearings */
    plats = rand_range(2, 4);

    /* Try fairly hard */
    for (j = 0; j < 50; j++) {
	/* Try for a clearing */
	a = randint0(6) + 4;
	b = randint0(5) + 4;
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	if (cave_feat[y][x] == FEAT_VOID)
	    continue;
	made_plat =
	    generate_starburst_room(y - b, x - a, y + b, x + a, FALSE,
				    FEAT_GRASS, TRUE);

	/* Success ? */
	if (made_plat)
	    plats--;

	/* Done ? */
	if (!plats)
	    break;
    }

    /* Place some formations */
    while (form_grids < (40 * p_ptr->danger)) {
	y = randint0(ARENA_HGT - 1) + 1;
	x = randint0(ARENA_WID - 1) + 1;
	form_grids +=
	    make_formation(y, x, FEAT_TREE, FEAT_TREE2, form_feats,
			   p_ptr->danger + 1);
    }

    /* No longer "icky" */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_ICKY);
	}
    }

    if (!p_ptr->path_coord) {
	y = ARENA_HGT / 2 - 10 + randint0(20);
	x = ARENA_WID / 2 - 15 + randint0(30);
	cave_set_feat(y, x, FEAT_GRASS);
	player_place(y, x);
	p_ptr->path_coord = 0;

	/* Make sure a web can't be placed on the player */
	cave_on(cave_info[y][x], CAVE_ICKY);
    }

    /* Basic "amount" */
    k = (p_ptr->danger / 2);
    if (k > 30)
	k = 30;

    /* Pick a base number of monsters */
    i = MIN_M_ALLOC_LEVEL + randint1(8);

    /* Build the monster probability table. */
    monster_level = p_ptr->danger;
    (void) get_mon_num(monster_level);

    /* Put some monsters in the dungeon */
    for (j = i + k; j > 0; j--) {
	/* Always have some random monsters */
	if ((get_mon_num_hook) && (j < 5)) {
	    /* Remove all monster restrictions. */
	    mon_restrict('\0', (byte) p_ptr->danger, &dummy, TRUE);

	    /* Build the monster probability table. */
	    (void) get_mon_num(p_ptr->danger);
	}

	/* 
	 * Place a random monster (quickly), but not in grids marked 
	 * "CAVE_TEMP".
	 */
	(void) alloc_monster(10, TRUE, TRUE);
    }


    /* Place some traps in the dungeon. */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_TRAP, randint1(k));

    /* Put some objects in rooms */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ROOM, 3));

    /* Put some objects/gold in the dungeon */
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_OBJECT,
		 Rand_normal(DUN_AMT_ITEM, 3));
    alloc_object(ALLOC_SET_BOTH, ALLOC_TYP_GOLD, Rand_normal(DUN_AMT_GOLD, 3));

    /* Place some webs */
    for (i = 0; i < damroll(k / 20, 4); i++)
	place_web(11);

    if (randint0(2) == 0)
	place_web(12);

    if (randint0(10) == 0)
	place_web(13);

    /* Clear "temp" flags. */
    for (y = 0; y < ARENA_HGT; y++) {
	for (x = 0; x < ARENA_WID; x++) {
	    cave_off(cave_info[y][x], CAVE_TEMP);
	    /* Paranoia - remake the dungeon walls */

	    if ((y == 0) || (x == 0) || (y == ARENA_HGT - 1)
		|| (x == ARENA_WID - 1))
		cave_set_feat(y, x, FEAT_PERM_SOLID);
	}
    }
}


