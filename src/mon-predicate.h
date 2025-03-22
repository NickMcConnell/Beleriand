/**
 * \file mon-predicate.h
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

#ifndef MON_PREDICATE_H
#define MON_PREDICATE_H

/**
 * monster_predicate is a function pointer which tests a given monster to
 * see if the predicate in question is true.
 */
typedef bool (*monster_predicate)(const struct monster *mon);

bool monster_is_undead(const struct monster *mon);
bool monster_is_nonliving(const struct monster *mon);
bool monster_is_living(const struct monster *mon);
bool monster_is_invisible(const struct monster *mon);
bool monster_is_unique(const struct monster *mon);
bool monster_is_smart(const struct monster *mon);
bool monster_is_free(const struct monster *mon);
bool monster_is_rideable(const struct monster *mon);
bool monster_has_friends(const struct monster *mon);
bool monster_breathes(const struct monster *mon);

bool monster_is_in_view(const struct monster *mon);
bool monster_is_visible(const struct monster *mon);
bool monster_is_listened(const struct monster *mon);
bool monster_is_stored(const struct monster *mon);
bool monster_is_tame(const struct monster *mon);
bool monster_is_hostile(const struct monster *mon);
bool monster_is_friendly(const struct monster *mon);
bool monster_is_neutral(const struct monster *mon);

#endif /* !MON_PREDICATE_H */
