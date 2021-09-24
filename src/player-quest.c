/**
 * \file player-quest.c
 * \brief All quest-related code
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
#include "angband.h"
#include "datafile.h"
#include "game-world.h"
#include "init.h"
#include "mon-util.h"
#include "monster.h"
#include "obj-pile.h"
#include "obj-util.h"
#include "player-calcs.h"
#include "player-quest.h"

/**
 * Array of quests
 */
struct quest *quests;

/**
 * Parsing functions for quest.txt
 */
static enum parser_error parse_quest_name(struct parser *p) {
	const char *name = parser_getstr(p, "name");
	struct quest *h = parser_priv(p);

	struct quest *q = mem_zalloc(sizeof(*q));
	q->next = h;
	parser_setpriv(p, q);
	q->name = string_make(name);

	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_type(struct parser *p) {
	struct quest *q = parser_priv(p);
	const char *name = parser_getstr(p, "type");

	if (streq(name, "monster")) {
		q->type = QUEST_MONSTER;
	} else if (streq(name, "unique")) {
		q->type = QUEST_UNIQUE;
		q->max_num = 1;
	} else if (streq(name, "place")) {
		q->type = QUEST_PLACE;
	} else if (streq(name, "final")) {
		q->type = QUEST_FINAL;
	} else {
		return PARSE_ERROR_INVALID_QUEST_TYPE;
	}
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_race(struct parser *p) {
	struct quest *q = parser_priv(p);
	const char *name = parser_getstr(p, "race");
	assert(q);

	q->race = lookup_monster(name);
	if (!q->race)
		return PARSE_ERROR_INVALID_MONSTER;

	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_artifact(struct parser *p) {
	struct quest *q = parser_priv(p);
	int chance = parser_getuint(p, "chance");
	struct quest_artifact *arts;
	const struct artifact *art;

	if (!q)
		return PARSE_ERROR_MISSING_RECORD_HEADER;
	if ((q->type != QUEST_UNIQUE) && (q->type != QUEST_FINAL)) {
		return PARSE_ERROR_ARTIFACT_IN_WRONG_QUEST;
	}
	art = lookup_artifact_name(parser_getstr(p, "name"));
	if (!art)
		return PARSE_ERROR_NO_ARTIFACT_NAME;
	arts = mem_zalloc(sizeof(*arts));
	arts->index = art->aidx;
	arts->chance = chance;
	arts->next = q->arts;
	q->arts = arts;
	return PARSE_ERROR_NONE;
}

static enum parser_error parse_quest_number(struct parser *p) {
	struct quest *q = parser_priv(p);
	assert(q);

	q->max_num = parser_getuint(p, "number");
	return PARSE_ERROR_NONE;
}

struct parser *init_parse_quest(void) {
	struct parser *p = parser_new();
	parser_setpriv(p, NULL);
	parser_reg(p, "name str name", parse_quest_name);
	parser_reg(p, "type str type", parse_quest_type);
	parser_reg(p, "race str race", parse_quest_race);
	parser_reg(p, "artifact uint chance str name", parse_quest_artifact);
	parser_reg(p, "number uint number", parse_quest_number);
	return p;
}

static errr run_parse_quest(struct parser *p) {
	return parse_file_quit_not_found(p, "quest");
}

static errr finish_parse_quest(struct parser *p) {
	quests = parser_priv(p);
	parser_destroy(p);
	return 0;
}

static void cleanup_quest(void)
{
	struct quest *quest = quests, *next;

	while (quest) {
		struct quest_artifact *arts = quest->arts, *next_art;
		next = quest->next;
		string_free(quest->name);
		while (arts) {
			next_art = arts->next;
			mem_free(arts);
			arts = next_art;
		}
		mem_free(quest);
		quest = next;
	}
}

struct file_parser quests_parser = {
	"quest",
	init_parse_quest,
	run_parse_quest,
	finish_parse_quest,
	cleanup_quest
};

/**
 * Set all the quests to incomplete.
 */
void quests_reset(void)
{
	struct quest *quest = quests;
	while (quest) {
		quest->complete = false;
		quest = quest->next;
	}
}

/**
 * Count the number of complete quests.
 */
int quests_count(void)
{
	struct quest *quest = quests;
	int num = 0;
	while (quest) {
		if (quest->complete) {
			num++;
		}
		quest = quest->next;
	}
	return num;
}

/**
 * Creates magical stairs or paths after finishing a quest.
 *
 * This assumes that any exit from the quest level except upstairs is
 * blocked until the quest is complete.
 */
static void build_quest_stairs(struct player *p, struct loc grid)
{
	struct loc new_grid = grid;

	/* Stagger around */
	while (!square_changeable(cave, grid) &&
		   !square_iswall(cave, grid) &&
		   !square_isdoor(cave, grid)) {
		/* Pick a location */
		scatter(cave, &new_grid, grid, 1, false);

		/* Stagger */
		grid = new_grid;
	}

	/* Push any objects */
	push_object(grid);

	/* Explain the staircase */
	msg("A magical staircase appears...");

	/* Create stairs down */
	square_set_feat(cave, grid, FEAT_MORE);

	/* Update the visuals */
	p->upkeep->update |= (PU_UPDATE_VIEW | PU_MONSTERS);

}

/**
 * Check if a monster is a unique quest monster
 */
bool quest_unique_monster_check(const struct monster_race *race)
{
	struct quest *quest = quests;

	while (quest) {
		if (((quest->type == QUEST_UNIQUE) || (quest->type == QUEST_FINAL))
			&& (quest->race == race)) {
			return true;
		}
		quest = quest->next;
	}
	return false;
}

/**
 * Check if this (now dead) monster is a quest monster, and act appropriately
 */
bool quest_monster_death_check(struct player *p, const struct monster *mon)
{
	struct quest *quest = NULL; //B

	/* Simple checks */
	if (!quest) return false;
	if ((mon->race != quest->race) || quest->complete) return false;

	/* Increment count, check for completion */
	quest->cur_num++;
	if (quest->cur_num >= quest->max_num) {
		quest->complete = true;

		/* Build magical stairs if needed */
		build_quest_stairs(p, mon->grid);

		/* Check specialties */
		p->upkeep->update |= (PU_SPECIALTY);
	}

	/* Game over... */
	if ((quest->type == QUEST_FINAL) && quest->complete) {
		p->total_winner = true;
		p->upkeep->redraw |= (PR_TITLE);
		msg("*** CONGRATULATIONS ***");
		msg("You have won the game!");
		msg("You may retire (commit suicide) when you are ready.");
	}

	return true;
}
