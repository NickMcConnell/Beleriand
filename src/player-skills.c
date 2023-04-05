/**
 * \file player-skills.c
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

#include "angband.h"
#include "cmd-core.h"
#include "player-calcs.h"
#include "player-skills.h"


static int skills[SKILL_MAX];
static int old_skills[SKILL_MAX];
static int exp_spent[SKILL_MAX];
static int exp_inc[SKILL_MAX];
static int exp_left;
static int old_exp_left;

/**
 * Skill point costs.
 *
 * The nth skill point costs (100 * n) experience points
 */
static int skill_cost(int base, int points)
{
	int total_cost = (points + base) * (points + base + 1) / 2;
	int prev_cost = (base) * (base + 1) / 2;
	return ((total_cost - prev_cost) * 100);
}

static void recalculate_skills(void)
{
	int i;

	/* Variable skill maxes */
	for (i = 0; i < SKILL_MAX; i++) {
		player->skill_base[i] = skills[i];
	}
	player->new_exp = exp_left;

	/* Update bonuses etc. */
	player->upkeep->update |= (PU_BONUS);
	update_stuff(player);


	/* Tell the UI about all this stuff that's changed. */
	event_signal(EVENT_SKILLS);
	event_signal(EVENT_EXP_CHANGE);
}

/**
 * Set allocated skill points to zero.
 */
void init_skills(bool start, bool reset)
{
	int i;

	/* Calculate and signal initial skills, points and experience totals. */
	old_exp_left = exp_left = player->new_exp;

	for (i = 0; i < SKILL_MAX; i++) {
		/* Initial skills are the current values and costs are zero */
		old_skills[i] = start ? 0 : (reset ? player->skill_base[i] :
									 skills[i] - player->skill_base[i]);
		skills[i] = start ? 0 : player->skill_base[i];
		exp_spent[i] = reset ? 0 : skill_cost(old_skills[i], skills[i] - old_skills[i]);
		exp_inc[i] = skill_cost(skills[i], 1);
	}

	/* Use the new base skill values to work out the skill values after
	 * modifiers) and tell the UI things have changed if necessary. */
	if (!start || reset) {
		recalculate_skills();
		event_signal_skillpoints(exp_spent, exp_inc, exp_left);
	}
}

/**
 * Set remembered skill points to what we've chosen.
 */
void finalise_skills(void)
{
	int i;
	for (i = 0; i < SKILL_MAX; i++) {
		old_skills[i] = skills[i];
	}
	old_exp_left = exp_left;
}

/**
 * Reset the allocated skill points for this buy to zero.
 */
static void reset_skills(void)
{
	int i;

	/* Calculate and signal initial skills, points and experience totals. */
	exp_left = old_exp_left;

	for (i = 0; i < SKILL_MAX; i++) {
		/* Initial skills are the current values and costs are zero */
		skills[i] = old_skills[i];
		exp_spent[i] = 0;
		exp_inc[i] = skill_cost(skills[i], 1);
	}

	/* Use the new base skill values to work out the skill values after
	 * modifiers) and tell the UI things have changed. */
	recalculate_skills();
	event_signal_skillpoints(exp_spent, exp_inc, exp_left);
}

static bool buy_skill(int choice)
{
	/* Must be a valid skill to be adjusted */
	if (!(choice >= SKILL_MAX || choice < 0)) {
		/* Get the cost of buying the extra point (beyond what
		   it has already cost to get this far). */
		int cost = skill_cost(skills[choice], 1);

		assert(cost == exp_inc[choice]);
		if (cost <= exp_left) {
			skills[choice]++;
			exp_spent[choice] += cost;
			exp_inc[choice] = skill_cost(skills[choice], 1);
			exp_left -= cost;

			/* Tell the UI the new points situation. */
			event_signal_skillpoints(exp_spent, exp_inc, exp_left);

			/* Recalculate everything that's changed because
			   the skill has changed, and inform the UI. */
			recalculate_skills();

			return true;
		}
	}

	/* Didn't adjust skill. */
	return false;
}


static bool sell_skill(int choice)
{
	/* Must be a valid skill, and we can't "sell" skills below current value. */
	if (!(choice >= SKILL_MAX || choice < 0) &&	(exp_spent[choice] > 0)) {
		int cost = skill_cost(skills[choice] - 1, 1);

		skills[choice]--;
		exp_spent[choice] -= cost;
		exp_inc[choice] = skill_cost(skills[choice], 1);
		exp_left += cost;

		/* Tell the UI the new points situation. */
		event_signal_skillpoints(exp_spent,	exp_inc, exp_left);

		/* Recalculate everything that's changed because
		   the skill has changed, and inform the UI. */
		recalculate_skills();

		return true;
	}

	/* Didn't adjust skill. */
	return false;
}

void do_cmd_buy_skill(struct command *cmd)
{
	/* .choice is the skill to sell */
	int choice;
	cmd_get_arg_choice(cmd, "choice", &choice);
	buy_skill(choice);
}

void do_cmd_sell_skill(struct command *cmd)
{
	/* .choice is the skill to sell */
	int choice;
	cmd_get_arg_choice(cmd, "choice", &choice);
	sell_skill(choice);
}

void do_cmd_reset_skills(struct command *cmd)
{
	reset_skills();
}

void do_cmd_refresh_skills(struct command *cmd)
{
	event_signal_skillpoints(exp_spent, exp_inc, exp_left);
}
