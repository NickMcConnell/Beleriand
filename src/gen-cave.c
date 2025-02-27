/**
 * \file gen-cave.c
 * \brief Generation of dungeon levels
 *
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2013 Erik Osheim, Nick McConnell
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
 * In this file, we use the SQUARE_WALL flags to the info field in
 * cave->squares.  Those are applied and tested on granite.  SQUARE_WALL_SOLID
 * fills the areas between rooms and can be carved out by tunneling.
 * SQUARE_WALL_INNER is used in rooms, either for exterior walls that can not
 * be carved out by tunneling or for interior walls.  SQUARE_WALL_OUTER is
 * used in rooms for exterior walls that can be carved out by tunneling.
 *
 * Note that a tunnel which attempts to leave a room near the edge of the
 * dungeon in a direction toward that edge will cause "silly" wall piercings,
 * but will have no permanently incorrect effects, as long as the tunnel can
 * eventually exit from another side. And note that the wall may not come back
 * into the room by the hole it left through, so it must bend to the left or
 * right and then optionally re-enter the room (at least 2 grids away). This is
 * not a problem since every room that is large enough to block the passage of
 * tunnels is also large enough to allow the tunnel to pierce the room itself
 * several times.
 *
 * Note that no two corridors may enter a room through adjacent grids, they
 * must either share an entryway or else use entryways at least two grids
 * apart. This prevents large (or "silly") doorways.
 *
 * Traditionally, to create rooms in the dungeon, it was divided up into
 * "blocks" of 11x11 grids each, and all rooms were required to occupy a
 * rectangular group of blocks.  As long as each room type reserved a
 * sufficient number of blocks, the room building routines would not need to
 * check bounds. Note that in classic generation most of the normal rooms
 * actually only use 23x11 grids, and so reserve 33x11 grids.
 *
 * Note that a lot of the original motivation for the block system was the
 * fact that there was only one size of map available, 22x66 grids, and the
 * dungeon level was divided up into nine of these in three rows of three.
 * Now that the map can be resized and enlarged, and dungeon levels themselves
 * can be different sizes, much of this original motivation has gone.  Blocks
 * can still be used, but different cave profiles can set their own block
 * sizes.  The classic generation method still uses the traditional blocks; the
 * main motivation for using blocks now is for the aesthetic effect of placing
 * rooms on a grid.
 */

#include "angband.h"
#include "cave.h"
#include "datafile.h"
#include "game-event.h"
#include "game-world.h"
#include "generate.h"
#include "init.h"
#include "mon-group.h"
#include "mon-make.h"
#include "mon-spell.h"
#include "mon-util.h"
#include "player-util.h"
#include "trap.h"
#include "z-queue.h"
#include "z-type.h"

/* ------------------ UTILITIES ---------------- */
//static char dumpname[30];
//strnfmt(dumpname, sizeof(dumpname), "%s", whatevs);
//dump_level_simple(dumpname, "Test Level", c);

/**
 * Check whether a square has one of the tunnelling helper flags
 * \param c is the current chunk
 * \param y are the co-ordinates
 * \param x are the co-ordinates
 * \param flag is the relevant flag
 */
static bool square_is_granite_with_flag(struct chunk *c, struct loc grid,
										int flag)
{
	if (square(c, grid)->feat != FEAT_GRANITE) return false;
	if (!sqinfo_has(square(c, grid)->info, flag)) return false;

	return true;
}

/**
 * Determines whether the player can pass through a given feature
 * icky locations (inside vaults) are all considered passable.
 */
static bool player_pass(struct chunk *c, struct loc grid, bool ignore_rubble)
{
	bool vault_interior = square_isvault(c, grid) &&
		square_isvault(c, loc(grid.x - 1, grid.y)) &&
		square_isvault(c, loc(grid.x + 1, grid.y)) &&
		square_isvault(c, loc(grid.x, grid.y - 1)) &&
		square_isvault(c, loc(grid.x, grid.y + 1));
	return square_ispassable(c, grid) || square_issecretdoor(c, grid) ||
		square_iscloseddoor(c, grid) ||
		(square_isrubble(c, grid) && ignore_rubble) || vault_interior;
}

/**
 * Floodfills access through the dungeon, marking all accessible squares true
 */
static void flood_access(struct chunk *c, struct loc grid, bool **access,
						 bool ignore_rubble)
{
	int i;

	/* First check the map bounds */
	if (!square_in_bounds_fully(c, grid)) return;
	
	access[grid.y][grid.x] = true;
	for (i = 0; i < 8; i++) {
		struct loc check = loc_sum(grid, ddgrid_ddd[i]);
		if (player_pass(c, check, ignore_rubble) && !access[check.y][check.x]) {
			flood_access(c, check, access, ignore_rubble);
		}
	}
	return;
}

/**
 * Places a thread of some feature from one grid to another.
 *
 * \param c is the current chunk
 * \param feat is the feature
 * \param grid1 is the start grid
 * \param grid2 is the finish grid
 */
static void build_thread(struct chunk *c, int feat, struct loc grid1,
						 struct loc grid2)
{
	struct loc grid = grid1, offset;

	while (!loc_eq(grid, grid2)) {
		/* Get the basic directions */
		offset.x = CMP(grid2.x, grid.x);
		offset.y = CMP(grid2.y, grid.y);

		/* Favour cardinal directions slightly */
		if (offset.x && offset.y && one_in_(3)) {
			if (one_in_(2)) {
				offset.x = 0;
			} else {
				offset.y = 0;
			}
		}

		/* Move toward the target */
		grid = loc_sum(offset, grid);
		square_set_feat(c, grid, feat);
	}
}

/**
 * Make sure that the level is sufficiently connected.
 *
 * Currently a failure here results in a new level being generated, which is OK
 * as long as it's not happening too often.  Failure can now only happen from
 * the player not reaching the stairs due to rubble.
 */
static bool ensure_connectivity(struct chunk *c)
{
	struct loc grid;
	bool result = false;

	/* Set the array used for checking connectivity */
	bool **access = mem_zalloc(c->height * sizeof(bool*));
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		access[grid.y] = mem_zalloc(c->width  * sizeof(bool));
	}

	/* Make sure entire dungeon is connected (ignoring rubble) */
	while (true) {
		bool fail = false;
		int dist = 2;
		struct loc target;
		flood_access(c, player->grid, access, true);
		for (grid.y = 0; grid.y < c->height; grid.y++) {
			for (grid.x = 0; grid.x < c->width; grid.x++) {
				if (player_pass(c, grid, true) && !access[grid.y][grid.x]) {
					fail = true;
					break;
				}
			}
			if (fail) break;
		}
		if (!fail) break;

		/* Find a room to connect the fail grid to */
		while (true) {
			struct loc tl, br;
			tl.x = MAX(grid.x - dist, 1);
			br.x = MIN(grid.x + dist, c->width - 1);
			tl.y = MAX(grid.y - dist, 1);
			br.y = MIN(grid.y + dist, c->height - 1);
			if (cave_find_in_range(c, &target, tl, br, square_isroom)) break;
			dist++;
		}
		build_thread(c, FEAT_FLOOR, grid, target);
	}

	/* Reset the array used for checking connectivity */
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			access[grid.y][grid.x] = false;
		}
	}
	
	/* Make sure player can reach stairs without going through rubble */
	flood_access(c, player->grid, access, false);
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		for (grid.x = 0; grid.x < c->width; grid.x++) {
			if (access[grid.y][grid.x] && square_isstairs(c, grid)) {
				result = true;
				goto CLEANUP;
			}
		}
	}

CLEANUP:
	for (grid.y = 0; grid.y < c->height; grid.y++) {
		mem_free(access[grid.y]);
	}
	mem_free(access);

	return result;
}

/**
 * Places a streamer of rock through dungeon.
 *
 * \param c is the current chunk
 * \param feat is the base feature (currently only FEAT_QUARTZ)
 */
static void build_streamer(struct chunk *c, int feat)
{
	/* Hack -- Choose starting point */
	struct loc grid = rand_loc(loc(c->width / 2, c->height / 2), 15, 10);

	/* Choose a random direction */
	int dir = ddd[randint0(8)];

	/* Place streamer into dungeon */
	while (true) {
		int i;
		struct loc change;

		/* One grid per density */
		for (i = 0; i < dun->profile->str.den; i++) {
			int d = dun->profile->str.rng;

			/* Pick a nearby grid */
			find_nearby_grid(c, &change, grid, d, d);

			/* Only convert walls */
			if (square_isgranitewall(c, change)) {
				/* Turn the rock into the vein type */
				square_set_feat(c, change, feat);
			}
		}

		/* Advance the streamer */
		grid = loc_sum(grid, ddgrid[dir]);

		/* Stop at dungeon edge */
		if (!square_in_bounds(c, grid)) break;
	}
}

/**
 * Places a single chasm.
 *
 * \param c is the current chunk
 */
static void build_chasm(struct chunk *c)
{
	bool chasm_ok = false;
	struct loc grid1;

	/* Try to mark squares to be a chasm */
	while (!chasm_ok) {
		int i;

        /* Choose starting point */
        struct loc grid = loc(rand_range(10, c->width - 10),
							  rand_range(10, c->height - 10));

		/* Choose a random cardinal direction for it to run in */
		int main_dir = ddd[randint0(4)];

		/* Choose a random length for it */
        int length = damroll(4, 8);

		/* Count floor squares that will be turned to chasm */
        int floor_to_chasm = 0;

		/* Determine its shape */
        for (i = 0; i < length; i++) {
            /* Go in a random direction half the time */
            if (one_in_(2)) {
                /* Choose the random cardinal direction */
                grid = loc_sum(grid, ddgrid_ddd[randint0(4)]);
            } else {
				/* Go straight ahead the other half */
				grid = loc_sum(grid, ddgrid[main_dir]);
			}

			/* Stop near dungeon edge */
            if ((grid.y < 3) || (grid.y > c->height - 3) ||
				(grid.x < 3) || (grid.x > c->width - 3))
				break;

            /* Mark that we want to put a chasm here */
            sqinfo_on(square(c, grid)->info, SQUARE_CHASM);
        }

		/* Start by assuming it will be OK */
        chasm_ok = true;

		/* Check it doesn't wreck the dungeon */
        for (grid.y = 1; grid.y < c->height - 1; grid.y++) {
            for (grid.x = 1; grid.x < c->width - 1; grid.x++) {
				/* Adjacent grids in the cardinal directions */
				struct loc gride = loc_sum(grid, loc(1, 0));
				struct loc grids = loc_sum(grid, loc(0, 1));
				struct loc gridw = loc_sum(grid, loc(-1, 0));
				struct loc gridn = loc_sum(grid, loc(0, -1));

                /* Only inspect squares currently destined to be chasms */
                if (square_tobechasm(c, grid)) {
                    /* Avoid chasms in interesting rooms / vaults */
                    if (square_isvault(c, grid)) {
                        chasm_ok = false;
                    }

                    /* Avoid two chasm square in a row in corridors */
                    if (square_tobechasm(c, grids) && !square_isroom(c, grid) &&
						!square_isroom(c, grids) && square_isfloor(c, grid) &&
						square_isfloor(c, grids)) {
                        chasm_ok = false;
                    }
                    if (square_tobechasm(c, gride) && !square_isroom(c, grid) &&
						!square_isroom(c, gride) && square_isfloor(c, grid) &&
						square_isfloor(c, gride)) {
                        chasm_ok = false;
                    }
                    
                    /* Avoid a chasm taking out the rock next to a door */
                    if (square_iscloseddoor(c, gride) ||
						square_iscloseddoor(c, grids) ||
						square_iscloseddoor(c, gridw) ||
						square_iscloseddoor(c, gridn)) {
                        chasm_ok = false;
                    }
                    
                    /* Avoid a chasm just hitting the wall of a lit room (would
					 * look odd that the light doesn't hit the wall behind) */
                    if (square_isrock(c, grid) && square_isglow(c, grid)) {
                        if ((square_isrock(c, gride) && !square_isglow(c, gride)
							 && !square_tobechasm(c, gride)) ||
							(square_isrock(c, grids) && !square_isglow(c, grids)
							 && !square_tobechasm(c, grids)) ||
							(square_isrock(c, gridw) && !square_isglow(c, gridw)
							 && !square_tobechasm(c, gridw)) ||
							(square_isrock(c, gridn) && !square_isglow(c, gridn)
							 && !square_tobechasm(c, gridn))) {
                            chasm_ok = false;
                        }
                    }
                        
                    /* Avoid a chasm having no squares in a room/corridor */
                    if (square_ispassable(c, grid)) {
                        floor_to_chasm++;
                    }
                }
            }
        }

		/* The chasm must affect at least one floor square */
        if (floor_to_chasm < 1) {
			chasm_ok = false;
		}

		/* Clear the flag for failed chasm placement */
        if (!chasm_ok) {
			for (grid.y = 0; grid.y < c->height; grid.y++) {
				for (grid.x = 0; grid.x < c->width; grid.x++) {
					sqinfo_off(square(c, grid)->info, SQUARE_CHASM);
                }
            }
        }
	}

	/* Actually place the chasm and clear the flag */
	for (grid1.y = 0; grid1.y < c->height; grid1.y++) {
		for (grid1.x = 0; grid1.x < c->width; grid1.x++) {
			if (sqinfo_has(square(c, grid1)->info, SQUARE_CHASM)) {
				square_set_feat(c, grid1, FEAT_CHASM);
				sqinfo_off(square(c, grid1)->info, SQUARE_CHASM);
			}
		}
	}
}

/**
 * Places chasms through dungeon
 */
static void build_chasms(struct chunk *c)
{
    int i;
    int chasms = 0;
    //int blocks = (c->height / z_info->block_hgt) * (c->width / z_info->block_wid);
    int blocks = 15; //Another guess - NRM

	/* If the level below is already built, no chasms */
	int lower, upper;
	bool below = gen_loc_find(chunk_list[player->place].x_pos,
							  chunk_list[player->place].y_pos,
							  chunk_list[player->place].z_pos + 1,
							  &lower, &upper);
	if (below) return;

    /* Determine whether to add chasms, and how many */
    if ((c->depth > 2) && (c->depth < z_info->dun_depth - 1) &&
		percent_chance(c->depth + 30)) {
        /* Add some chasms */
        chasms += damroll(1, blocks / 3);

        /* Flip a coin, and if it is heads... */
        while (one_in_(2)) {
            /* Add some more chasms and flip again... */
            chasms += damroll(1, blocks / 3);
        }
    }

    /* Build them */
    for (i = 0; i < chasms; i++) {
        build_chasm(c);
    }

    if (OPT(player, cheat_room) && (chasms > 0)) {
		msg("%d chasms.", chasms);
	}
}

/**
 * Reset entrance data for rooms in global dun.
 * \param c Is the chunk holding the rooms.
 */
static void reset_entrance_data(const struct chunk *c)
{
	int i;

	for (i = 0; i < z_info->level_room_max; ++i) {
		dun->ent_n[i] = 0;
	}
	if (dun->ent2room) {
		for (i = 0; dun->ent2room[i]; ++i) {
			mem_free(dun->ent2room[i]);
		}
		mem_free(dun->ent2room);
	}
	/* Add a trailing NULL so the deallocation knows when to stop. */
	dun->ent2room = mem_alloc((c->height + 1) * sizeof(*dun->ent2room));
	for (i = 0; i < c->height; ++i) {
		int j;

		dun->ent2room[i] =
			mem_alloc(c->width * sizeof(*dun->ent2room[i]));
		for (j = 0; j < c->width; ++j) {
			dun->ent2room[i][j] = -1;
		}
	}
	dun->ent2room[c->height] = NULL;
}


/**
 * Randomly choose a room entrance and return its coordinates.
 * \param c Is the chunk to use.
 * \param ridx Is the 0-based index for the room.
 * \param tgt If not NULL, the choice of entrance will either be *tgt if *tgt
 * is an entrance for the room, ridx, or can be biased to be closer to *tgt
 * when *tgt is not an entrance for the room, ridx.
 * \param bias Sets the amount of bias if tgt is not NULL and *tgt is not an
 * entrance for the room, ridx.  A larger value increases the amount of bias.
 * A value of zero will give no bias.  Must be non-negative.
 * \param exc Is an array of grids whose adjacent neighbors (but not the grid
 * itself) should be excluded from selection.  May be NULL if nexc is not
 * positive.
 * \param nexc Is the number of grids to use from exc.
 * \return The returned value is an entrance for the room or (0, 0) if
 * no entrance is available.  An entrance, x, satisfies these requirements:
 * 1) x is the same as dun->ent[ridx][k] for some k between 0 and
 * dun->ent_n[ridx - 1].
 * 2) square_is_marked_granite(c, x, SQUARE_WALL_OUTER) is true.
 * 3) For all m between zero and nexc - 1, ABS(x.x - exc[m].x) > 1 or
 * ABS(x.y - exc[m].y) > 1 or (x.x == exc[m].x and x.y == exc[m].y).
 */
static struct loc choose_random_entrance(struct chunk *c, int ridx,
	const struct loc *tgt, int bias, const struct loc *exc, int nexc)
{
	assert(ridx >= 0 && ridx < dun->cent_n);
	if (dun->ent_n[ridx] > 0) {
		int nchoice = 0;
		int *accum = mem_alloc((dun->ent_n[ridx] + 1) *
			sizeof(*accum));
		int i;

		accum[0] = 0;
		for (i = 0; i < dun->ent_n[ridx]; ++i) {
			bool included = square_is_granite_with_flag(c,
				dun->ent[ridx][i], SQUARE_WALL_OUTER);

			if (included) {
				int j = 0;

				while (1) {
					struct loc diff;

					if (j >= nexc) {
						break;
					}
					diff = loc_diff(dun->ent[ridx][i],
						exc[j]);
					if (ABS(diff.x) <= 1 &&
							ABS(diff.y) <= 1 &&
							(diff.x != 0 ||
							diff.y != 0)) {
						included = false;
						break;
					}
					++j;
				}
			}
			if (included) {
				if (tgt) {
					int d, biased;

					assert(bias >= 0);
					d = distance(dun->ent[ridx][i], *tgt);
					if (d == 0) {
						/*
						 * There's an exact match.  Use
						 * it.
						 */
						mem_free(accum);
						return dun->ent[ridx][i];
					}

					biased = MAX(1, bias - d);
					/*
					 * Squaring here is just a guess without
					 * any specific reason to back it.
					 */
					accum[i + 1] = accum[i] +
						biased * biased;
				} else {
					accum[i + 1] = accum[i] + 1;
				}
				++nchoice;
			} else {
				accum[i + 1] = accum[i];
			}
		}
		if (nchoice > 0) {
			int chosen = randint0(accum[dun->ent_n[ridx]]);
			int low = 0, high = dun->ent_n[ridx];

			/* Locate the selection by binary search. */
			while (1) {
				int mid;

				if (low == high - 1) {
					assert(accum[low] <= chosen &&
						accum[high] > chosen);
					mem_free(accum);
					return dun->ent[ridx][low];
				}
				mid = (low + high) / 2;
				if (accum[mid] <= chosen) {
					low = mid;
				} else {
					high = mid;
				}
			}
		}
		mem_free(accum);
	}

	/* There's no satisfactory marked entrances. */
	return loc(0, 0);
}


/**
 * Help build_tunnel():  pierce an outer wall and prevent nearby piercings.
 * \param c Is the chunk to use.
 * \param grid Is the location to pierce.
 */
static void pierce_outer_wall(struct chunk *c, struct loc grid)
{
	struct loc adj;

	/* Save the wall location */
	if (dun->wall_n < z_info->wall_pierce_max) {
		dun->wall[dun->wall_n] = grid;
		dun->wall_n++;
	}

	/* Forbid re-entry near this piercing */
	for (adj.y = grid.y - 1; adj.y <= grid.y + 1; adj.y++) {
		for (adj.x = grid.x - 1; adj.x <= grid.x + 1; adj.x++) {
			if (adj.x != 0 && adj.y != 0 &&
					square_in_bounds(c, adj) &&
					square_is_granite_with_flag(c, adj,
					SQUARE_WALL_OUTER)) {
				set_marked_granite(c, adj, SQUARE_WALL_SOLID);
			}
		}
	}
}


/**
 * Help build_tunnel():  handle bookkeeping, mainly if there's a diagonal step,
 * for the first step after piercing a wall.
 * \param c Is the chunk to use.
 * \param grid At entry, *grid is the location at which the wall was pierced.
 * At exit, *grid is the starting point for the next iteration of tunnel
 * building.
 * \param dir At entry, *dir is the chosen direction for the first step after
 * the wall piercing.  At exit, *dir is the direction for the next iteration of
 * tunnel building.
 * \param door_flag At entry, *door_flag is the current setting for whether a
 * door can be added.  At exit, *door_flag is the setting for whether a door
 * can be added in the next iteration of tunnel building.
 * \param bend_invl At entry, *bend_intvl is the current setting for the number
 * of tunnel iterations to wait before applying a bend.  At exit, *bend_intvl
 * is what that intverval should be for the next iteration of tunnel building.
 */
static void handle_post_wall_step(struct chunk *c, struct loc *grid,
	struct loc *dir, bool *door_flag, int *bend_intvl)
{
	if (dir->x != 0 && dir->y != 0) {
		/*
		 * Take a diagonal step upon leaving the wall.  Proceed to that.
		 */
		*grid = loc_sum(*grid, *dir);
		assert(!square_is_granite_with_flag(c, *grid, SQUARE_WALL_OUTER) &&
			!square_is_granite_with_flag(c, *grid, SQUARE_WALL_SOLID) &&
			!square_is_granite_with_flag(c, *grid, SQUARE_WALL_INNER) &&
			!square_isperm(c, *grid));

		if (!square_isroom(c, *grid) && square_isgranite(c, *grid)) {
			/* Save the tunnel location */
			if (dun->tunn_n < z_info->tunn_grid_max) {
				dun->tunn[dun->tunn_n] = *grid;
				dun->tunn_n++;
			}

			/* Allow door in next grid */
			*door_flag = false;
		}

		/*
		 * Having pierced the wall and taken a step, can forget about
		 * what was set to suppress bends in the past.
		 */
		*bend_intvl = 0;

		/*
		 * Now choose a cardinal direction, one that is +/-45 degrees
		 * from what was used for the diagonal step, for the next step
		 * since the tunnel iterations want a cardinal direction.
		 */
		if (randint0(32768) < 16384) {
			dir->x = 0;
		} else {
			dir->y = 0;
		}
	} else {
		/*
		 * Take a cardinal step upon leaving the wall.  Most of the
		 * passed in state is fine, but temporarily suppress bends so
		 * the step will be handled as is by the next iteration of
		 * tunnel building.
		 */
		*bend_intvl = 1;
	}
}


/**
 * Help build_tunnel():  choose a direction that is approximately normal to a
 * room's wall.
 * \param c Is the chunk to use.
 * \param grid Is a location on the wall.
 * \param inner If true, return a direction that points to the interior of the
 * room.  Otherwise, return a direction pointing to the exterior.
 * \return The returned value is the chosen direction.  It may be loc(0, 0)
 * if no feasible direction could be found.
 */
static struct loc find_normal_to_wall(struct chunk *c, struct loc grid,
	bool inner)
{
	int n = 0, ncardinal = 0, i;
	struct loc choices[8];

	assert(square_is_granite_with_flag(c, grid, SQUARE_WALL_OUTER) ||
		square_is_granite_with_flag(c, grid, SQUARE_WALL_SOLID));
	/* Relies on the cardinal directions being first in ddgrid_ddd. */
	for (i = 0; i < 8; ++i) {
		struct loc chk = loc_sum(grid, ddgrid_ddd[i]);

		if (square_in_bounds(c, chk) &&
			!square_isperm(c, chk) &&
			(square_isroom(c, chk) == inner) &&
			!square_is_granite_with_flag(c, chk, SQUARE_WALL_OUTER) &&
			!square_is_granite_with_flag(c, chk, SQUARE_WALL_SOLID) &&
			!square_is_granite_with_flag(c, chk, SQUARE_WALL_INNER)) {
			choices[n] = ddgrid_ddd[i];
			++n;
			if (i < 4) {
				++ncardinal;
			}
		}
	}
	/* Prefer a cardinal direction if available. */
	if (n > 1 && ncardinal > 0) {
		n = ncardinal;
	}
	return (n == 0) ? loc(0, 0) : choices[randint0(n)];
}


/**
 * Help build_tunnel():  test if a wall-piercing location can have a door.
 * Don't want a door that's only adjacent to terrain that is either
 * 1) not passable and not rubble
 * 2) a door
 * on either the side facing outside the room or the side facing the room.
 * \param c Is the chunk to use.
 * \param grid Is the location of the wall piercing.
 */
static bool allows_wall_piercing_door(struct chunk *c, struct loc grid)
{
	struct loc chk;
	int n_outside_good = 0;
	int n_inside_good = 0;

	for (chk.y = grid.y - 1; chk.y <= grid.y + 1; ++chk.y) {
		for (chk.x = grid.x - 1; chk.x <= grid.x + 1; ++chk.x) {
			if ((chk.y == 0 && chk.x == 0) ||
					!square_in_bounds(c, chk)) continue;
			if ((square_ispassable(c, chk) ||
					square_isrubble(c, chk)) &&
					!square_isdoor(c, chk)) {
				if (square_isroom(c, chk)) {
					++n_inside_good;
				} else {
					++n_outside_good;
				}
			}
		}
	}
	return n_outside_good > 0 && n_inside_good > 0;
}


/**
 * Constructs a tunnel between two points
 *
 * \param c is the current chunk
 * \param grid1 is the location of the first point
 * \param grid2 is the location of the second point
 *
 * This function must be called BEFORE any streamers are created, since we use
 * granite with the special SQUARE_WALL flags to keep track of legal places for
 * corridors to pierce rooms.
 *
 * Locations to excavate are queued and applied afterward.  The wall piercings
 * are also queued but the outer wall grids adjacent to the piercing are marked
 * right away to prevent adjacent piercings.  That makes testing where to
 * pierce easier (look at grid flags rather than search through the queued
 * piercings).
 *
 * The solid wall check prevents silly door placement and excessively wide
 * room entrances.
 */
static void build_tunnel(struct chunk *c, struct loc grid1, struct loc grid2)
{
	int i;
	int dstart = ABS(grid1.x - grid2.x) + ABS(grid1.y - grid2.y);
	int main_loop_count = 0;
	struct loc start = grid1, tmp_grid, offset;
	/* Used to prevent random bends for a while. */
	int bend_intvl = 0;
	/*
	 * Used to prevent excessive door creation along overlapping corridors.
	 */
	bool door_flag = false;
	bool preemptive = false;

	/* Reset the arrays */
	dun->tunn_n = 0;
	dun->wall_n = 0;

	/* Start out in the correct direction */
	correct_dir(&offset, grid1, grid2);

	/* Keep going until done (or bored) */
	while (!loc_eq(grid1, grid2)) {
		/* Mega-Hack -- Paranoia -- prevent infinite loops */
		if (main_loop_count++ > 2000) break;

		/* Allow bends in the tunnel */
		if (bend_intvl == 0) {
			if (randint0(100) < dun->profile->tun.chg) {
				/* Get the correct direction */
				correct_dir(&offset, grid1, grid2);

				/* Random direction */
				if (randint0(100) < dun->profile->tun.rnd)
					rand_dir(&offset);
			}
		} else {
			assert(bend_intvl > 0);
			--bend_intvl;
		}

		/* Get the next location */
		tmp_grid = loc_sum(grid1, offset);

		while (!square_in_bounds(c, tmp_grid)) {
			/* Get the correct direction */
			correct_dir(&offset, grid1, grid2);

			/* Random direction */
			if (randint0(100) < dun->profile->tun.rnd)
				rand_dir(&offset);

			/* Get the next location */
			tmp_grid = loc_sum(grid1, offset);
		}

		/* Avoid obstacles */
		if ((square_isperm(c, tmp_grid) && !sqinfo_has(square(c,
				tmp_grid)->info, SQUARE_WALL_INNER)) ||
				square_is_granite_with_flag(c, tmp_grid,
				SQUARE_WALL_SOLID)) {
			continue;
		}

		/* Pierce "outer" walls of rooms */
		if (square_is_granite_with_flag(c, tmp_grid, SQUARE_WALL_OUTER)) {
			int iroom;
			struct loc nxtdir = loc_diff(grid2, tmp_grid);

			/* If it's the goal, accept and pierce the wall. */
			if (nxtdir.x == 0 && nxtdir.y == 0) {
				grid1 = tmp_grid;
				pierce_outer_wall(c, grid1);
				continue;
			}
			/*
			 * If it's adjacent to the goal and that is also an
			 * outer wall, then can't pierce without making the
			 * goal unreachable.
			 */
			if (ABS(nxtdir.x) <= 1 && ABS(nxtdir.y) <= 1 &&
					square_is_granite_with_flag(c, grid2,
					SQUARE_WALL_OUTER)) {
				continue;
			}
			/* See if it is a marked entrance. */
			iroom = dun->ent2room[tmp_grid.y][tmp_grid.x];
			if (iroom != -1) {
				/* It is. */
				assert(iroom >= 0 && iroom < dun->cent_n);
				if (square_isroom(c, grid1)) {
					/*
					 * The tunnel is coming from inside the
					 * room.  See if there's somewhere on
					 * the outside to go.
					 */
					nxtdir = find_normal_to_wall(c,
						tmp_grid, false);
					if (nxtdir.x == 0 && nxtdir.y == 0) {
						/* There isn't. */
						continue;
					}
					/*
					 * There is.  Accept the grid and pierce
					 * the wall.
					 */
					grid1 = tmp_grid;
					pierce_outer_wall(c, grid1);
				} else {
					/*
					 * The tunnel is coming from outside the
					 * room.  Choose an entrance (perhaps
					 * the same as the one just entered) to
					 * use as the exit.  Crudely adjust how
					 * biased the entrance selection is
					 * based on how often random steps are
					 * taken while tunneling.  The rationale
					 * for a maximum bias of 80 is similar
					 * to that in
					 * do_traditional_tunneling().
					 */
					int bias = 80 - ((80 *
						MIN(MAX(0, dun->profile->tun.chg), 100) *
						MIN(MAX(0, dun->profile->tun.rnd), 100)) /
						10000);
					int ntry = 0, mtry = 20;
					struct loc exc[2] = { tmp_grid, grid2 };
					struct loc chk = loc(0, 0);

					while (1) {
						if (ntry >= mtry) {
							/*
							 * Didn't find a usable
							 * exit.
							 */
							break;
						}
						chk = choose_random_entrance(
							c, iroom, &grid2, bias,
							exc, 2);
						if (chk.x == 0 && chk.y == 0) {
							/* No exits at all. */
							ntry = mtry;
							break;
						}
						nxtdir = find_normal_to_wall(
							c, chk, false);
						if (nxtdir.x != 0 ||
								nxtdir.y != 0) {
							/*
							 * Found a usable exit.
							 */
							break;
						}
						++ntry;
						/* Also make it less biased. */
						bias = (bias * 8) / 10;
					}
					if (ntry >= mtry) {
						/* No usable exit was found. */
						continue;
					}
					/*
					 * Pierce the wall at the original
					 * entrance.
					 */
					pierce_outer_wall(c, tmp_grid);
					/*
					 * And at the exit which is also the
					 * continuation point for the rest of
					 * the tunnel.
					 */
					pierce_outer_wall(c, chk);
					grid1 = chk;
				}
				offset = nxtdir;
				handle_post_wall_step(c, &grid1, &offset,
					&door_flag, &bend_intvl);
				continue;
			}

			/* Is there a feasible location after the wall? */
			nxtdir = find_normal_to_wall(c, tmp_grid,
				!square_isroom(c, grid1));

			if (nxtdir.x == 0 && nxtdir.y == 0) {
				/* There's no feasible location. */
				continue;
			}

			/* Accept the location and pierce the wall. */
			grid1 = tmp_grid;
			pierce_outer_wall(c, grid1);
			offset = nxtdir;
			handle_post_wall_step(c, &grid1, &offset, &door_flag,
				&bend_intvl);
		} else if (square_isroom(c, tmp_grid)) {
			/* Travel quickly through rooms */

			/* Accept the location */
			grid1 = tmp_grid;
		} else if (square_isgranite(c, tmp_grid)) {
			/* Tunnel through all other walls */

			/* Accept this location */
			grid1 = tmp_grid;

			/* Save the tunnel location */
			if (dun->tunn_n < z_info->tunn_grid_max) {
				dun->tunn[dun->tunn_n] = grid1;
				dun->tunn_n++;
			}

			/* Allow door in next grid */
			door_flag = false;
		} else {
			/* Handle corridor intersections or overlaps */

			assert(square_in_bounds_fully(c, tmp_grid));

			/* Accept the location */
			grid1 = tmp_grid;

			/* Collect legal door locations */
			if (!door_flag) {
				/* Save the door location */
				if (dun->door_n < z_info->level_door_max) {
					dun->door[dun->door_n] = grid1;
					dun->door_n++;
				}

				/* No door in next grid */
				door_flag = true;
			}

			/* Hack -- allow pre-emptive tunnel termination */
			if (randint0(100) >= dun->profile->tun.con) {
				/* Offset between grid1 and start */
				tmp_grid = loc_diff(grid1, start);

				/* Terminate the tunnel if too far vertically or horizontally */
				if ((ABS(tmp_grid.x) > 10) ||
						(ABS(tmp_grid.y) > 10)) {
					preemptive = true;
					break;
				}
			}
		}
	}

	/* Turn the tunnel into corridor */
	for (i = 0; i < dun->tunn_n; i++) {
		/* Clear previous contents, add a floor */
		square_set_feat(c, dun->tunn[i], FEAT_FLOOR);
	}

	/* Apply the piercings that we found */
	for (i = 0; i < dun->wall_n; i++) {
		/* Convert to floor grid */
		square_set_feat(c, dun->wall[i], FEAT_FLOOR);

		/* Place a random door */
		if (randint0(100) < dun->profile->tun.pen &&
				allows_wall_piercing_door(c, dun->wall[i]))
			place_random_door(c, dun->wall[i]);
	}

	event_signal_tunnel(EVENT_GEN_TUNNEL_FINISHED,
		main_loop_count, dun->wall_n, dun->tunn_n, dstart,
		ABS(grid1.x - grid2.x) + ABS(grid1.y - grid2.y), preemptive);
}

/**
 * Count the number of corridor grids adjacent to the given grid.
 *
 * This routine currently only counts actual "empty floor" grids which are not
 * in rooms.
 * \param c is the current chunk
 * \param y1 are the co-ordinates
 * \param x1 are the co-ordinates
 *
 * TODO: count stairs, open doors, closed doors?
 */
static int next_to_corr(struct chunk *c, struct loc grid)
{
	int i, k = 0;
	assert(square_in_bounds(c, grid));

	/* Scan adjacent grids */
	for (i = 0; i < 4; i++) {
		/* Extract the location */
		struct loc grid1 = loc_sum(grid, ddgrid_ddd[i]);

		/* Count only floors which aren't part of rooms */
		if (square_isfloor(c, grid1) && !square_isroom(c, grid1)) k++;
	}

	/* Return the number of corridors */
	return k;
}

/**
 * Returns whether a doorway can be built in a space.
 * \param c is the current chunk
 * \param y are the co-ordinates
 * \param x are the co-ordinates
 *
 * To have a doorway, a space must be adjacent to at least two corridors and be
 * between two walls.
 */
static bool possible_doorway(struct chunk *c, struct loc grid)
{
	assert(square_in_bounds(c, grid));
	if (next_to_corr(c, grid) < 2)
		return false;
	else if (square_isstrongwall(c, next_grid(grid, DIR_N)) &&
			 square_isstrongwall(c, next_grid(grid, DIR_S)))
		return true;
	else if (square_isstrongwall(c, next_grid(grid, DIR_W)) &&
			 square_isstrongwall(c, next_grid(grid, DIR_E)))
		return true;
	else
		return false;
}


/**
 * Places door or trap at y, x position if at least 2 walls found
 * \param c is the current chunk
 * \param y are the co-ordinates
 * \param x are the co-ordinates
 */
static void try_door(struct chunk *c, struct loc grid)
{
	assert(square_in_bounds(c, grid));

	if (square_isstrongwall(c, grid)) return;
	if (square_isroom(c, grid)) return;
	if (square_isplayertrap(c, grid)) return;
	if (square_isdoor(c, grid)) return;

	if (randint0(100) < dun->profile->tun.jct && possible_doorway(c, grid))
		place_random_door(c, grid);
	else if (randint0(500) < dun->profile->tun.jct && possible_doorway(c, grid))
		place_trap(c, grid, -1, player->depth);
}


/**
 * Connect the rooms with tunnels in the traditional fashion.
 * \param c Is the chunk to use.
 */
static void do_traditional_tunneling(struct chunk *c)
{
	int *scrambled = mem_alloc(dun->cent_n * sizeof(*scrambled));
	int i;
	struct loc grid;

	/*
	 * Scramble the order in which the rooms will be connected.  Use
	 * indirect indexing so dun->ent2room can be left as it is.
	 */
	for (i = 0; i < dun->cent_n; ++i) {
		scrambled[i] = i;
	}
	for (i = 0; i < dun->cent_n; ++i) {
		int pick1 = randint0(dun->cent_n);
		int pick2 = randint0(dun->cent_n);
		int tmp = scrambled[pick1];

		scrambled[pick1] = scrambled[pick2];
		scrambled[pick2] = tmp;
	}

	/* Start with no tunnel doors. */
	dun->door_n = 0;

	/*
	 * Link the rooms in the scrambled order with the first connecting to
	 * the last.  The bias argument for choose_random_entrance() was
	 * somewhat arbitrarily chosen:  i.e. if the room is more than a
	 * typical screen width away, don't particularly care which entrance is
	 * selected.
	 */
	grid = choose_random_entrance(c, scrambled[dun->cent_n - 1], NULL, 80,
		NULL, 0);
	if (grid.x == 0 && grid.y == 0) {
		/* Use the room's center. */
		grid = dun->cent[scrambled[dun->cent_n - 1]];
	}
	for (i = 0; i < dun->cent_n; ++i) {
		struct loc next_grid = choose_random_entrance(c, scrambled[i],
			&grid, 80, NULL, 0);

		if (next_grid.x == 0 && next_grid.y == 0) {
			next_grid = dun->cent[scrambled[i]];
		}
		build_tunnel(c, next_grid, grid);

		/* Remember the "previous" room. */
		grid = next_grid;
	}

	mem_free(scrambled);

	/* Place intersection doors. */
	for (i = 0; i < dun->door_n; ++i) {
		/* Try placing doors. */
		try_door(c, next_grid(dun->door[i], DIR_W));
		try_door(c, next_grid(dun->door[i], DIR_E));
		try_door(c, next_grid(dun->door[i], DIR_N));
		try_door(c, next_grid(dun->door[i], DIR_S));
	}
}


/**
 * Build the staircase rooms.
 */
static void build_staircase_rooms(struct chunk *c, const char *label)
{
	int num_rooms = dun->profile->n_room_profiles;
	struct room_profile profile;
	struct connector *join;
	int i;

	for (i = 0; i < num_rooms; i++) {
		profile = dun->profile->room_profiles[i];
		if (streq(profile.name, "staircase room")) {
			break;
		}
	}
	assert(i < num_rooms);
	for (join = dun->join; join; join = join->next) {
		if (!(feat_is_stair(join->feat) || feat_is_shaft(join->feat))) continue;
		dun->curr_join = join;
		if (!room_build(c, profile)) {
			dump_level_simple(NULL, format("%s:  Failed to Build "
				"Staircase Room at Row=%d Column=%d in a "
				"Cave with %d Rows and %d Columns", label,
				join->grid.y, join->grid.x, c->height,
				c->width), c);
			quit("Failed to place stairs");
		}
		++dun->nstair_room;
	}
}


/**
 * Add stairs to a level, taking into account joins to other levels.
 */
static void handle_level_stairs(struct chunk *c, struct player *p, int count)
{
	/*
	 * Require that the stairs be at least four grids apart (two for
	 * surrounding walls; two for a buffer between the walls; the buffer
	 * space could be one - shared by the staircases - but the reservations
	 * in the room map don't allow for that) so the staircase rooms in the
	 * connecting level won't overlap.
	 */
	int minsep = 4;
	int lower, upper;
	bool one_above = gen_loc_find(chunk_list[p->place].x_pos,
								  chunk_list[p->place].y_pos,
								  chunk_list[p->place].z_pos - 1,
								  &lower, &upper);
	bool one_below = gen_loc_find(chunk_list[p->place].x_pos,
								  chunk_list[p->place].y_pos,
								  chunk_list[p->place].z_pos + 1,
								  &lower, &upper);
	bool two_above = gen_loc_find(chunk_list[p->place].x_pos,
								  chunk_list[p->place].y_pos,
								  chunk_list[p->place].z_pos - 1,
								  &lower, &upper);
	bool two_below = gen_loc_find(chunk_list[p->place].x_pos,
								  chunk_list[p->place].y_pos,
								  chunk_list[p->place].z_pos + 1,
								  &lower, &upper);

	if (!one_below) {
		assert(!two_below);
		alloc_stairs(c, FEAT_MORE, count, minsep, true);
	} else if (!two_below) {
		alloc_stairs(c, FEAT_MORE_SHAFT, count / 2, minsep, false);
	}

	if (!one_above) {
		assert(two_above);
		alloc_stairs(c, FEAT_MORE, count / 2 + 1, minsep, false);
	} else if (!two_above) {
		assert(one_above);
		alloc_stairs(c, FEAT_MORE_SHAFT, count / 2, minsep, false);
	}
}

/* ------------------ ANGBAND ---------------- */
/**
 * The main angband generation algorithm
 * \param p is the player, in case generation fails and the partially created
 * level needs to be cleaned up
 * \param depth is the chunk's native depth
 * \param height are the chunk's dimensions
 * \param width are the chunk's dimensions
 * \param forge if true forces a forge on this level
 * \return a pointer to the generated chunk
 */
static struct chunk *angband_chunk(struct player *p, int depth, int height,
								   int width, bool forge)
{
	int i;
	int key, rarity;
	int num_floors;
	int num_rooms = dun->profile->n_room_profiles;
	int dun_unusual = dun->profile->dun_unusual;
	int n_attempt;

	/* Make the cave */
	struct chunk *c = chunk_new(height, width);
	c->depth = p->depth;

	/* Set the intended number of floor grids based on cave floor area */
	num_floors = c->height * c->width / 7;
	ROOM_LOG("height=%d  width=%d  nfloors=%d", c->height, c->width,num_floors);

	/* Fill cave area with basic granite */
	fill_rectangle(c, 0, 0, c->height - 1, c->width - 1, 
		FEAT_GRANITE, SQUARE_NONE);

	/* Generate permanent walls around the generated area (temporarily!) */
	draw_rectangle(c, 0, 0, c->height - 1, c->width - 1, 
		FEAT_PERM, SQUARE_NONE, true);

	/* Actual maximum number of blocks on this level */
	dun->row_blocks = c->height / dun->block_hgt;
	dun->col_blocks = c->width / dun->block_wid;

	/* Initialize the room table */
	dun->room_map = mem_zalloc(dun->row_blocks * sizeof(bool*));
	for (i = 0; i < dun->row_blocks; i++)
		dun->room_map[i] = mem_zalloc(dun->col_blocks * sizeof(bool));

	/* No rooms yet, pits or otherwise. */
	dun->cent_n = 0;
	reset_entrance_data(c);

	/* Build the special staircase rooms */
	build_staircase_rooms(c, "Standard Generation");

	/* Guarantee a forge if one hasn't been generated in a while */
	if (forge) {
		struct room_profile profile = lookup_room_profile("Interesting room");
		if (OPT(p, cheat_room)) msg("Trying to force a forge:");
		p->upkeep->force_forge = true;

		/* Failure (not clear why this would happen) */
		if (!room_build(c, profile)) {
			p->upkeep->force_forge = false;
			if (OPT(p, cheat_room)) msg("failed.");
			uncreate_artifacts(c);
			uncreate_greater_vaults(c, p);
			delete_temp_monsters();
			chunk_wipe(c);
			return NULL;
		}

		if (OPT(p, cheat_room)) msg("succeeded.");
		p->upkeep->force_forge = false;
	}

	/*
	 * Build rooms until we have enough floor grids and at least two rooms
	 * or we appear to be stuck and can't match those criteria.
	 */
	n_attempt = 0;
	while (1) {
		if (c->feat_count[FEAT_FLOOR] >= num_floors
				&& dun->cent_n >= 2) {
			break;
		}
		/*
		 * At an average of roughly 22 successful rooms per level
		 * (and a standard deviation of 4.5 or so for that) and a
		 * room failure rate that's less than .5 failures per success
		 * (4.2.x profile doesn't use full allocation for rarity two
		 * rooms - only up to 60; and the last type tried in that
		 * rarity has a failure rate per successful rooms of all types
		 * of around .024).  500 attempts is a generous cutoff for
		 * saying no further progress is likely.
		 */
		if (n_attempt > 500) {
			uncreate_artifacts(c);
			uncreate_greater_vaults(c, p);
			delete_temp_monsters();
			chunk_wipe(c);
			return NULL;
		}
		++n_attempt;

		/* Roll for random key (to be compared against a profile's cutoff) */
		key = randint0(100);

		/* We generate a rarity number to figure out how exotic to make
		 * the room. This number has a (50+depth/2)/DUN_UNUSUAL chance
		 * of being > 0, a (50+depth/2)^2/DUN_UNUSUAL^2 chance of
		 * being > 1, up to MAX_RARITY. */
		i = 0;
		rarity = 0;
		while (i == rarity && i < dun->profile->max_rarity) {
			if (randint0(dun_unusual) < 50 + depth / 2) rarity++;
			i++;
		}

		/* Once we have a key and a rarity, we iterate through out list of
		 * room profiles looking for a match (whose cutoff > key and whose
		 * rarity > this rarity). We try building the room, and if it works
		 * then we are done with this iteration. We keep going until we find
		 * a room that we can build successfully or we exhaust the profiles. */
		for (i = 0; i < num_rooms; i++) {
			struct room_profile profile = dun->profile->room_profiles[i];
			if (profile.rarity > rarity) continue;
			if (profile.cutoff <= key) continue;
			if (room_build(c, profile)) break;
		}
	}

	for (i = 0; i < dun->row_blocks; i++)
		mem_free(dun->room_map[i]);
	mem_free(dun->room_map);

	/* Connect all the rooms together */
	do_traditional_tunneling(c);

	/* Turn the outer permanent walls back to granite */
	draw_rectangle(c, 0, 0, c->height - 1, c->width - 1, 
		FEAT_GRANITE, SQUARE_NONE, true);

	return c;
}

/**
 * Generate a new dungeon level.
 * \param p is the player
 * \return a pointer to the generated chunk
 *
 * This is sample code to illustrate some of the new dungeon generation
 * methods; I think it actually produces quite nice levels.  New stuff:
 *
 * - different sized levels
 * - independence from block size: the block size can be set to any number
 *   from 1 (no blocks) to about 15; beyond that it struggles to generate
 *   enough floor space
 * - the find_space function, called from the room builder functions, allows
 *   the room to find space for itself rather than the generation algorithm
 *   allocating it; this helps because the room knows better what size it is
 * - a count is now kept of grids of the various terrains, allowing dungeon
 *   generation to terminate when enough floor is generated
 * - there are three new room types - huge rooms, rooms of chambers
 *   and interesting rooms - as well as many new vaults
 * - there is the ability to place specific monsters and objects in vaults and
 *   interesting rooms, as well as to make general monster restrictions in
 *   areas or the whole dungeon
 */
struct chunk *angband_gen(struct player *p) {
	int i;
	int y_size = ARENA_SIDE, x_size = ARENA_SIDE;
	struct chunk *c;
	bool forge = false;
	struct connector *join;

	/* Hack - variables for allocations */
	int rubble_gen, mon_gen, obj_room_gen;


	/* Guarantee a forge if one hasn't been generated in a while */
	if (p->forge_drought >= rand_range(2000, 5000)) forge = true;

	/* Set the block height and width */
	dun->block_hgt = dun->profile->block_size;
	dun->block_wid = dun->profile->block_size;

	c = angband_chunk(p, p->depth, MIN(z_info->dungeon_hgt, y_size),
					  MIN(z_info->dungeon_wid, x_size), forge);
	if (!c) return NULL;

	/* Generate permanent walls around the edge of the generated area */
	draw_rectangle(c, 0, 0, c->height - 1, c->width - 1,
		FEAT_PERM, SQUARE_NONE, true);

	/* Add some quartz streamers */
	for (i = 0; i < dun->profile->str.qua; i++)
		build_streamer(c, FEAT_QUARTZ);

	/* Place stairs near some walls as allowed by levels above and below */
	handle_level_stairs(c, p, rand_range(3, 4));

    /* Add any chasms if needed */
    build_chasms(c);

	/* Place some rubble, occasionally much more on deep levels */
	rubble_gen = randint1(5);
	if ((c->depth >= 10) && one_in_(10)) {
		rubble_gen += 30;
	}
	alloc_object(c, SET_BOTH, TYP_RUBBLE, rubble_gen, p->depth, ORIGIN_FLOOR);

	/* Add join floors (the bottoms of chasms) */
	for (join = dun->join; join; join = join->next) {
		if (feat_is_floor(join->feat)) {
			/* Allow any passable terrain, but replace impassable with floor */
			if (!square_ispassable(c, join->grid)) {
				square_set_feat(c, join->grid, FEAT_FLOOR);
			}
		}
	}

	/* Check dungeon connectivity */
	if (!ensure_connectivity(c)) {
		if (OPT(p, cheat_room)) msg("Failed connectivity.");
		uncreate_artifacts(c);
		uncreate_greater_vaults(c, p);
		delete_temp_monsters();
		chunk_wipe(c);
		return NULL;
	}

	/* Remove all monster restrictions. */
	//mon_restrict(NULL, p->depth, p->depth, true);

	/* Place the player */
	player_place(c, p, p->grid);

	/* Place Morgoth if on the run */
	if (p->on_the_run && !p->morgoth_slain) {
		/* Place Morgoth */
		struct loc grid;
		int count = 100;

		/* Find a suitable place */
		while (cave_find(c, &grid, square_suits_start) && count--) {
			/* Out of sight of the player */
			if (!los(c, p->grid, grid)) {
				struct monster_group_info info = {0, 0};
				place_new_monster_one(c, grid, lookup_monster("Morgoth"), true,
									  true, info, ORIGIN_DROP);
				break;
			}
		}
	}

	/* If we've generated this level before, we're done now */
	if (!dun->first_time) return c;

	/* Put some monsters in the dungeon */
	if (p->depth == 1) {
		/* Smaller number of monsters at 50ft */
		mon_gen = dun->cent_n / 2;
	} else {
		/* Pick some number of monsters (between 0.5 per room and 1 per room) */
		mon_gen = (dun->cent_n + randint1(dun->cent_n)) / 2;
	}
	for (i = mon_gen; i > 0; i--)
		pick_and_place_distant_monster(c, p, '!', true, p->depth);

	/* Put some objects in rooms */
	obj_room_gen = 3 * mon_gen / 4;
	if (obj_room_gen > 0) {
		alloc_object(c, SET_ROOM, TYP_OBJECT, obj_room_gen, p->depth,
			ORIGIN_FLOOR);
	}

    /* Place the traps */
    place_traps(c);

	/* Add a curved sword near the player if this is the start of the game */
	if (p->turn == 0) {
		place_item_near_player(c, p, TV_SWORD, "Curved Sword");
	}

	return c;
}

/* ------------------ THRONE ---------------- */

/**
 * Create the level containing Morgoth's throne room
 */
struct chunk *throne_gen(struct player *p)
{
	int y, x;
	struct chunk *c;
	struct room_profile profile = lookup_room_profile("Throne room");
	struct loc pgrid = loc(0, 0);

	/* Display the throne poetry */
	event_signal_poem(EVENT_POEM, "throne_poetry", 5, 15);

	/* Set the 'truce' in action */
	p->truce = true;

	/* Restrict to single-screen size */
	c = chunk_new(3 * 11, 3 * 33);
	c->depth = p->depth;

	/* Fill cave area with basic granite */
	fill_rectangle(c, 0, 0, c->height - 1, c->width - 1, FEAT_GRANITE,
				   SQUARE_WALL_SOLID);

	/* Generate permanent walls around the edge of the generated area */
	draw_rectangle(c, 0, 0, c->height - 1, c->width - 1, FEAT_PERM, SQUARE_NONE,
				   true);

	/* Build it */
	room_build(c, profile);

	/* Find an up staircase */
	for (y = 0; y < c->height; y++) {
		for (x = 0; x < c->width; x++) {
			struct loc grid = loc(x, y);
			/* Assumes the important staircase is at the centre of the level */
			if (square_isupstairs(c, grid) && (x >= 40) && (x <= 55)) {
				pgrid = grid;
				break;
			}
		}
		if (!loc_eq(pgrid, loc(0, 0))) break;
	}

	if (loc_eq(pgrid, loc(0, 0))) {
		msg("Failed to find an up staircase in the throne-room");
	}

	/* Delete any monster on the starting square */
	delete_monster(pgrid);
	
	/* Place the player */
	player_place(c, p, pgrid);

	return c;
}


/* ------------------ LANDMARK ---------------- */

/**
 * Load the appropriate bit of a landmark from the text file
 */
bool build_landmark(struct chunk *c, int index, int map_y, int map_x,
					int y_coord, int x_coord)
{
	/* Where in the arena the chunk is going */
	struct loc target = loc(x_coord * CHUNK_SIDE, y_coord * CHUNK_SIDE);

	/* Set all the chunk reading data */
	struct landmark *landmark = &landmark_info[index];
	struct loc top_left = loc((map_x - landmark->map_x) * CHUNK_SIDE,
							  (map_y - landmark->map_y) * CHUNK_SIDE);
	struct loc bottom_right = loc_sum(top_left, loc(CHUNK_SIDE, CHUNK_SIDE));
	int y_total = landmark->height * CHUNK_SIDE;
	int x_total = landmark->width * CHUNK_SIDE;

	/* Check bounds */
	if ((top_left.y < 0) || (top_left.y > y_total) ||
		(top_left.x < 0) || (top_left.x > x_total)) {
		/* Oops.  We're /not/ on a landmark */
		return false;
	}

	/* Place terrain features */
	get_terrain(c, top_left, bottom_right, target, y_total, x_total,
				0, false, NULL, true, landmark->text, true);

	/* Success. */
	return true;
}
