/**
 * \file mon-predicate.c
 * \brief Monster predicates
 *
 * Copyright (c) 1997-2007 Ben Harrison, James E. Wilson, Robert A. Koeneke
 * Copyright (c) 2017 Nick McConnell
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
#include "game-world.h"
#include "generate.h"
#include "mon-group.h"
#include "mon-spell.h"
#include "mon-util.h"

/**
 * ------------------------------------------------------------------------
 * Permanent monster properties
 * ------------------------------------------------------------------------ */
/**
 * Undead monsters
 */
bool monster_is_undead(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_UNDEAD);
}

/**
 * Nonliving monsters are immune to life drain
 */
bool monster_is_nonliving(const struct monster *mon)
{
	return (monster_is_undead(mon) || rf_has(mon->race->flags, RF_RAUKO) ||
			rf_has(mon->race->flags, RF_STONE) ||
			(mon->race->base == lookup_monster_base("deathblade")));
}

/**
 * Living monsters
 */
bool monster_is_living(const struct monster *mon)
{
	return !monster_is_nonliving(mon);
}

/**
 * Monster is invisible
 */
bool monster_is_invisible(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_INVISIBLE);
}

/**
 * Monster is unique
 */
bool monster_is_unique(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_UNIQUE);
}

/**
 * Monster is (or was) smart
 */
bool monster_is_smart(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_SMART);
}

/**
 * Monster is free (ie not bound to Morgoth)
 */
bool monster_is_free(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_FREE);
}

/**
 * Monster can be ridden
 */
bool monster_is_rideable(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_RIDEABLE);
}

/**
 * Monster has friends
 */
bool monster_has_friends(const struct monster *mon)
{
	return rf_has(mon->race->flags, RF_FRIEND) ||
		rf_has(mon->race->flags, RF_FRIENDS) ||
		rf_has(mon->race->flags, RF_ESCORT) ||
		rf_has(mon->race->flags, RF_ESCORTS);
}

/**
 * Monster has damaging breath
 */
bool monster_breathes(const struct monster *mon)
{
	bitflag breaths[RSF_SIZE];
	create_mon_spell_mask(breaths, RST_BREATH, RST_NONE);
	rsf_inter(breaths, mon->race->spell_flags);
	return rsf_is_empty(breaths) ? false : true;
}

/**
 * ------------------------------------------------------------------------
 * Temporary monster properties
 * ------------------------------------------------------------------------ */
/**
 * Monster is in the player's field of view
 */
bool monster_is_in_view(const struct monster *mon)
{
	return mflag_has(mon->mflag, MFLAG_VIEW);
}

/**
 * Monster is visible to the player
 */
bool monster_is_visible(const struct monster *mon)
{
	return mflag_has(mon->mflag, MFLAG_VISIBLE);
}

/**
 * Monster is currently heard by the listen ability
 */
bool monster_is_listened(const struct monster *mon)
{
	return mflag_has(mon->mflag, MFLAG_LISTENED);
}

/**
 * Monster is not in the current playing arena
 */
bool monster_is_stored(const struct monster *mon)
{
	return mon->place != CHUNK_CUR;
}

/**
 * Monster is currently tame
 */
bool monster_is_tame(const struct monster *mon)
{
	return mflag_has(mon->mflag, MFLAG_TAME);
}

/**
 * Monster is currently hostile
 */
bool monster_is_hostile(const struct monster *mon)
{
	if (!monster_is_free(mon)) return true;
	return mflag_has(mon->mflag, MFLAG_HOSTILE);
}

/**
 * Monster is currently friendly
 */
bool monster_is_friendly(const struct monster *mon)
{
	return mflag_has(mon->mflag, MFLAG_FRIENDLY);
}

/**
 * Monster is currently neutral
 */
bool monster_is_neutral(const struct monster *mon)
{
	if (!monster_is_free(mon)) return false;
	return !(mflag_has(mon->mflag, MFLAG_FRIENDLY) ||
			 mflag_has(mon->mflag, MFLAG_TAME) ||
			 mflag_has(mon->mflag, MFLAG_HOSTILE));
}
