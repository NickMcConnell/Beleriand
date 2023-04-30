/**
 * \file mon-spell.h
 * \brief structures and functions for monster spells
 *
 * Copyright (c) 2011 Chris Carr
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

#ifndef MONSTER_SPELL_H
#define MONSTER_SPELL_H

#include "monster.h"

/** Variables **/
/* none so far */

/** Constants **/

/* Spell type bitflags */
enum mon_spell_type {
	RST_NONE		= 0x0000,
	RST_INNATE		= 0x0001,
	RST_ARCHERY		= 0x0002,
	RST_BREATH		= 0x0004,
	RST_SPELL		= 0x0008,
	RST_DISTANT		= 0x0010,
	RST_SONG		= 0x0020
};


/** Macros **/
#define rsf_has(f, flag)       flag_has_dbg(f, RSF_SIZE, flag, #f, #flag)
#define rsf_next(f, flag)      flag_next(f, RSF_SIZE, flag)
#define rsf_count(f)           flag_count(f, RSF_SIZE)
#define rsf_is_empty(f)        flag_is_empty(f, RSF_SIZE)
#define rsf_is_full(f)         flag_is_full(f, RSF_SIZE)
#define rsf_is_inter(f1, f2)   flag_is_inter(f1, f2, RSF_SIZE)
#define rsf_is_subset(f1, f2)  flag_is_subset(f1, f2, RSF_SIZE)
#define rsf_is_equal(f1, f2)   flag_is_equal(f1, f2, RSF_SIZE)
#define rsf_on(f, flag)        flag_on_dbg(f, RSF_SIZE, flag, #f, #flag)
#define rsf_off(f, flag)       flag_off(f, RSF_SIZE, flag)
#define rsf_wipe(f)            flag_wipe(f, RSF_SIZE)
#define rsf_setall(f)          flag_setall(f, RSF_SIZE)
#define rsf_negate(f)          flag_negate(f, RSF_SIZE)
#define rsf_copy(f1, f2)       flag_copy(f1, f2, RSF_SIZE)
#define rsf_union(f1, f2)      flag_union(f1, f2, RSF_SIZE)
#define rsf_inter(f1, f2)      flag_inter(f1, f2, RSF_SIZE)
#define rsf_diff(f1, f2)       flag_diff(f1, f2, RSF_SIZE)


/** Functions **/
const struct monster_spell *monster_spell_by_index(int index);
int monster_cast_chance(struct monster *mon);
void do_mon_spell(int index, struct monster *mon, bool seen);
void remove_bad_spells(struct monster *mon, bitflag f[RSF_SIZE]);
void create_mon_spell_mask(bitflag *f, ...);
const char *mon_spell_lore_description(int index,
									   const struct monster_race *race);
random_value mon_spell_lore_damage(int index);

#endif /* MONSTER_SPELL_H */
