/* parse/house-info */

#include "unit-test.h"

#include "init.h"
#include "obj-properties.h"
#include "object.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "player.h"

int setup_tests(void **state) {
	*state = init_parse_house();
	return !*state;
}

int teardown_tests(void *state) {
	struct player_house *h = parser_priv(state);
	string_free((char *)h->name);
	mem_free(h);
	parser_destroy(state);
	return 0;
}

static int test_name0(void *state) {
	enum parser_error r = parser_parse(state, "name:House of Fingolfin");
	struct player_house *h;

	eq(r, PARSE_ERROR_NONE);
	h = parser_priv(state);
	require(h);
	require(streq(h->name, "House of Fingolfin"));
	ok;
}

static int test_name1(void *state) {
	enum parser_error r = parser_parse(state, "alt-name:Fingolfin's house");
	struct player_house *h;

	eq(r, PARSE_ERROR_NONE);
	h = parser_priv(state);
	require(h);
	require(streq(h->alt_name, "Fingolfin's house"));
	ok;
}

static int test_name2(void *state) {
	enum parser_error r = parser_parse(state, "short-name:Fingolfin");
	struct player_house *h;

	eq(r, PARSE_ERROR_NONE);
	h = parser_priv(state);
	require(h);
	require(streq(h->short_name, "Fingolfin"));
	ok;
}

static int test_stats0(void *state) {
	enum parser_error r = parser_parse(state, "stats:3:-3:2:-2");
	struct player_house *h;

	eq(r, PARSE_ERROR_NONE);
	h = parser_priv(state);
	require(h);
	eq(h->stat_adj[STAT_STR], 3);
	eq(h->stat_adj[STAT_DEX], -3);
	eq(h->stat_adj[STAT_CON], 2);
	eq(h->stat_adj[STAT_GRA], -2);
	ok;
}

static int test_skills0(void *state) {
	enum parser_error r = parser_parse(state, "skills:1:2:-1:0:1:1:0:0");
	struct player_house *h;

	eq(r, PARSE_ERROR_NONE);
	h = parser_priv(state);
	require(h);
	eq(h->skill_adj[SKILL_MELEE], 1);
	eq(h->skill_adj[SKILL_ARCHERY], 2);
	eq(h->skill_adj[SKILL_EVASION], -1);
	eq(h->skill_adj[SKILL_STEALTH], 0);
	eq(h->skill_adj[SKILL_PERCEPTION], 1);
	eq(h->skill_adj[SKILL_WILL], 1);
	eq(h->skill_adj[SKILL_SMITHING], 0);
	eq(h->skill_adj[SKILL_SONG], 0);
	ok;
}

static int test_flags0(void *state) {
	enum parser_error r = parser_parse(state, "player-flags:AXE_PROFICIENCY | BLADE_PROFICIENCY");
	struct player_house *h;

	eq(r, PARSE_ERROR_NONE);
	h = parser_priv(state);
	require(h);
	require(h->pflags);
	ok;
}

const char *suite_name = "parse/house-info";
struct test tests[] = {
	{ "name0", test_name0 },
	{ "name1", test_name1 },
	{ "name2", test_name2 },
	{ "stats0", test_stats0 },
	{ "skills0", test_skills0 },
	{ "flags0", test_flags0 },
	{ NULL, NULL }
};
