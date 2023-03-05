/**
 * \file player-quest.h
 * \brief Quest-related variables and functions
 *
 * Copyright (c) 2013 Angband developers
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

#ifndef QUEST_H
#define QUEST_H

/* Functions */
void drop_iron_crown(struct monster *mon, const char *message);
void shatter_weapon(struct player *p, int silnum);
void break_truce(struct player *p, bool obvious);
void check_truce(struct player *p);
void wake_all_monsters(struct player *p);
void prise_silmaril(struct player *p);
int silmarils_possessed(struct player *p);


#endif /* QUEST_H */
