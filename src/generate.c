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
 * Downstairs from the level above, used in cave_gen()
 */
int downstair_n;
edge_effect downstair[STAIR_MAX];


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
	for (y = 0; y < ARENA_HGT; y++) {
		for (x = 0; x < ARENA_WID; x++) {
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

		/* First turn */
		if (turn == 1) {
			for (y = 0; y < 3; y++) {
				for (x = 0; x < 3; x++) {
					chunk_ref ref = CHUNK_EMPTY;

					/* Get the location data */
					ref.region = chunk_list[p_ptr->stage].region;
					ref.z_pos = chunk_list[p_ptr->stage].z_pos;
					ref.y_pos = chunk_list[p_ptr->stage].y_pos;
					ref.x_pos = chunk_list[p_ptr->stage].x_pos;
					chunk_adjacent_data(&ref, 0, y, x);

					/* Generate a new chunk */
					chunk_generate(ref, y, x);
				}
			}
		}
		/* Stuff already generated - BELE assumes surface above dungeon
		 * exists */
		else {
			/* No existing level */
			if (p_ptr->stage == MAX_CHUNKS) {
				int y_offset = p_ptr->py / CHUNK_HGT;
				int x_offset = p_ptr->px / CHUNK_WID;
				/* BELE while whole level generation is still happening */
				bool completely_new = FALSE;

				/* No down stair effects yet */
				downstair_n = 0;

				/* Deal with location data */
				for (y = 0; y < 3; y++) {
					for (x = 0; x < 3; x++) {
						chunk_ref ref = CHUNK_EMPTY;
						int y0 = y - y_offset;
						int x0 = x - x_offset;
						int lower, upper;
						bool reload;
						gen_loc *uplevel;
						edge_effect *current;

						/* Get the location data */
						ref.region = chunk_list[p_ptr->last_stage].region;
						ref.z_pos = p_ptr->danger;
						ref.y_pos =
							chunk_list[p_ptr->last_stage].y_pos + y0;
						ref.x_pos =
							chunk_list[p_ptr->last_stage].x_pos + x0;

						/* See if we've been generated before */
						reload =
							gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos,
										 &lower, &upper);

						/* New gen_loc */
						if (!reload) {
							gen_loc_make(ref.x_pos, ref.y_pos, ref.z_pos,
										 lower, upper);
							completely_new = TRUE;
						}

						/* Store the chunk reference */
						(void) chunk_store(1, 1, ref.region, ref.z_pos,
										   ref.y_pos, ref.x_pos, FALSE);

						/* Get the edge effects from the level up */
						reload = gen_loc_find(ref.x_pos, ref.y_pos,
											  ref.z_pos - 1, &lower,
											  &upper);
						uplevel = &gen_loc_list[lower];
						current = uplevel->effect;
						while (current) {
							if (current->terrain != FEAT_MORE) {
								current = current->next;
								continue;
							}
							downstair[downstair_n].y =
								current->y + CHUNK_HGT * y;
							downstair[downstair_n].x =
								current->x + CHUNK_WID * x;
							downstair[downstair_n++].terrain = FEAT_LESS;
							current = current->next;
						}

						/* Is this where the player is? */
						if ((y0 == 0) && (x0 == 0)) {
							p_ptr->stage = chunk_find(ref);
							assert(p_ptr->stage != MAX_CHUNKS);
						}
					}
				}


				/* Set the RNG to give reproducible results */
				Rand_quick = TRUE;
				Rand_value =
					((chunk_list[p_ptr->last_stage].y_pos & 0x1fff) << 19);
				Rand_value |= ((p_ptr->danger & 0x3f) << 13);
				Rand_value |=
					(chunk_list[p_ptr->last_stage].x_pos & 0x1fff);
				Rand_value ^= seed_flavor;

				/* Generate the level */
				cave_gen();

				Rand_quick = FALSE;

				/* Chunk it */
				y_offset = p_ptr->py / CHUNK_HGT;
				x_offset = p_ptr->px / CHUNK_WID;
				for (y = 0; y < 3; y++) {
					for (x = 0; x < 3; x++) {
						chunk_ref ref = CHUNK_EMPTY;
						int y0 = y - y_offset;
						int x0 = x - x_offset;
						int lower, upper;
						bool reload;
						gen_loc *location;
						terrain_change *change;
						int num_effects = 0;
						int grid_y, grid_x;
						edge_effect *current = NULL;

						/* Get the location data */
						ref.region = chunk_list[p_ptr->last_stage].region;
						ref.z_pos = p_ptr->danger;
						ref.y_pos =
							chunk_list[p_ptr->last_stage].y_pos + y0;
						ref.x_pos =
							chunk_list[p_ptr->last_stage].x_pos + x0;

						/* Should have been generated before */
						reload =
							gen_loc_find(ref.x_pos, ref.y_pos, ref.z_pos,
										 &lower, &upper);

						/* Access the old place in the gen_loc_list */
						if (reload)
							location = &gen_loc_list[lower];
						else
							quit("Location failure!");

						/* Do terrain changes */
						for (change = location->change; change;
							 change = change->next) {
							grid_y = y * CHUNK_HGT + change->y;
							grid_x = x * CHUNK_WID + change->x;

							cave_set_feat(grid_y, grid_x, change->terrain);
						}

						/* Write edge effects if this is the first generation */
						if (completely_new) {
							/* Count the non-zero edge effects needed */
							for (grid_y = 0; grid_y < CHUNK_HGT; grid_y++) {
								for (grid_x = 0; grid_x < CHUNK_WID;
									 grid_x++) {
									byte feat =
										cave_feat[y * CHUNK_HGT +
												  grid_y][x * CHUNK_WID +
														  grid_x];
									if (tf_has
										(f_info[feat].flags, TF_DOWNSTAIR)
										|| tf_has(f_info[feat].flags,
												  TF_FALL))
										num_effects++;
								}
							}

							/* Now write them */
							gen_loc_list[upper].effect
								=
								mem_zalloc(num_effects *
										   sizeof(edge_effect));
							current = gen_loc_list[upper].effect;
							for (grid_x = 0; grid_x < CHUNK_WID; grid_x++) {
								for (grid_y = 0; grid_y < CHUNK_HGT;
									 grid_y++) {
									byte feat =
										cave_feat[y * CHUNK_HGT +
												  grid_y][x * CHUNK_WID +
														  grid_x];
									if (tf_has
										(f_info[feat].flags, TF_DOWNSTAIR)
										|| tf_has(f_info[feat].flags,
												  TF_FALL)) {
										current->y = grid_y;
										current->x = grid_x;
										current->terrain = feat;
										cave_copy(current->info,
												  cave_info[y * CHUNK_HGT +
															grid_y][x *
																	CHUNK_WID
																	+
																	grid_x]);
										num_effects--;
										if (num_effects != 0) {
											current->next = current + 1;
											current = current->next;
										}
									}
								}
							}
							if (current && (num_effects == 0))
								current->next = NULL;
						}
					}
				}
				chunk_list[p_ptr->last_stage].adjacent[DIR_DOWN] =
					p_ptr->stage;
			}
			/* Otherwise load up the chunks */
			else {
				int centre = chunk_get_centre();
				assert(centre != MAX_CHUNKS);

				for (y = 0; y < 3; y++) {
					for (x = 0; x < 3; x++) {
						int chunk_idx;
						chunk_ref ref = CHUNK_EMPTY;

						/* Get the location data */
						ref.region = chunk_list[centre].region;
						ref.z_pos = p_ptr->danger;
						ref.y_pos = chunk_list[centre].y_pos + y - 1;
						ref.x_pos = chunk_list[centre].x_pos + x - 1;

						/* Load it */
						chunk_idx = chunk_find(ref);
						if ((chunk_idx != MAX_CHUNKS) &&
							chunk_list[chunk_idx].chunk)
							chunk_read(chunk_idx, y, x);
						else
							quit("Failed to find chunk!");
					}
				}
				player_place(p_ptr->py, p_ptr->px);
			}
			chunk_fix_all();
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
			p_ptr->themed_level_appeared &=
				~(1L << (p_ptr->themed_level - 1));

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
