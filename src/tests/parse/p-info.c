/* parse/p-info */

#include "unit-test.h"
#include "init.h"
#include "player.h"


int setup_tests(void **state) {
	*state = init_parse_race();
	return !*state;
}

int teardown_tests(void *state) {
	struct player_race *pr = parser_priv(state);
	string_free((char *)pr->name);
	mem_free(pr);
	parser_destroy(state);
	return 0;
}

static int test_name0(void *state) {
	enum parser_error r = parser_parse(state, "name:Half-Elf");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	require(streq(pr->name, "Half-Elf"));
	ok;
}

static int test_stats0(void *state) {
	enum parser_error r = parser_parse(state, "stats:1:-1:2:-2");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	eq(pr->stat_adj[STAT_STR], 1);
	eq(pr->stat_adj[STAT_DEX], -1);
	eq(pr->stat_adj[STAT_CON], 2);
	eq(pr->stat_adj[STAT_GRA], -2);
	ok;
}

static int test_skills0(void *state) {
	enum parser_error r = parser_parse(state, "skills:1:2:-1:0:1:0:-1:0");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	eq(pr->skill_adj[SKILL_MELEE], 1);
	eq(pr->skill_adj[SKILL_ARCHERY], 2);
	eq(pr->skill_adj[SKILL_EVASION], -1);
	eq(pr->skill_adj[SKILL_STEALTH], 0);
	eq(pr->skill_adj[SKILL_PERCEPTION], 1);
	eq(pr->skill_adj[SKILL_WILL], 0);
	eq(pr->skill_adj[SKILL_SMITHING], -1);
	eq(pr->skill_adj[SKILL_SONG], 0);
	ok;
}

static int test_history0(void *state) {
	enum parser_error r = parser_parse(state, "history:0");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	null(pr->history);
	ok;
}

static int test_age0(void *state) {
	enum parser_error r = parser_parse(state, "age:10:3");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	eq(pr->b_age, 10);
	eq(pr->m_age, 3);
	ok;
}

static int test_height0(void *state) {
	enum parser_error r = parser_parse(state, "height:10:2");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	eq(pr->base_hgt, 10);
	eq(pr->mod_hgt, 2);
	ok;
}

static int test_weight0(void *state) {
	enum parser_error r = parser_parse(state, "weight:80:10");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	eq(pr->base_wgt, 80);
	eq(pr->mod_wgt, 10);
	ok;
}

static int test_play_flags0(void *state) {
	enum parser_error r = parser_parse(state, "player-flags:BLADE_PROFICIENCY");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	require(pr->pflags);
	ok;
}

static int test_equip0(void *state) {
	enum parser_error r = parser_parse(state, "equip:sword:1:2:5");
	struct player_race *pr;

	eq(r, PARSE_ERROR_NONE);
	pr = parser_priv(state);
	require(pr);
	eq(pr->start_items[0].tval, TV_SWORD);
	eq(pr->start_items[0].sval, 1);
	eq(pr->start_items[0].min, 2);
	eq(pr->start_items[0].max, 5);
	ok;
}

const char *suite_name = "parse/p-info";
struct test tests[] = {
	{ "name0", test_name0 },
	{ "stats0", test_stats0 },
	{ "skills0", test_skills0 },
	{ "history0", test_history0 },
	{ "age0", test_age0 },
	{ "height0", test_height0 },
	{ "weight0", test_weight0 },
	{ "player_flags0", test_play_flags0 },
	{ "equip0", test_equip0 },
	{ NULL, NULL }
};
