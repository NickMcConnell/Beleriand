/**
 * \file player-skills.h
 * \brief Player skill allocation
 *
 * Copyright (c) 1997 - 2023 Ben Harrison, James E. Wilson, Robert A. Koeneke
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

#ifndef INCLUDED_PLAYER_SKILLS_H
#define INCLUDED_PLAYER_SKILLS_H

void init_skills(bool start, bool reset);
void finalise_skills(void);
void do_cmd_buy_skill(struct command *cmd);
void do_cmd_sell_skill(struct command *cmd);
void do_cmd_reset_skills(struct command *cmd);
void do_cmd_refresh_skills(struct command *cmd);

#endif
