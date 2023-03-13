/* parse/e-info */

#include "unit-test.h"
#include "unit-test-data.h"
#include "obj-tval.h"
#include "object.h"
#include "init.h"


int setup_tests(void **state) {
	*state = init_parse_ego();
	return !*state;
}

int teardown_tests(void *state) {
	struct ego_item *e = parser_priv(state);
	string_free(e->name);
	string_free(e->text);
	mem_free(e);
	parser_destroy(state);
	return 0;
}

static int test_name0(void *state) {
	enum parser_error r = parser_parse(state, "name:of Resist Lightning");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	require(streq(e->name, "of Resist Lightning"));
	ok;
}

static int test_attack0(void *state) {
	enum parser_error r = parser_parse(state, "max-attack:6");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	eq(e->att, 6);
	ok;
}

static int test_dam_dice0(void *state) {
	enum parser_error r = parser_parse(state, "dam-dice:0");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	eq(e->dd, 0);
	ok;
}

static int test_dam_sides0(void *state) {
	enum parser_error r = parser_parse(state, "dam-sides:0");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	eq(e->ds, 0);
	ok;
}

static int test_evasion0(void *state) {
	enum parser_error r = parser_parse(state, "max-evasion:3");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	eq(e->evn, 3);
	ok;
}

static int test_prot_dice0(void *state) {
	enum parser_error r = parser_parse(state, "prot-dice:1");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	eq(e->pd, 1);
	ok;
}

static int test_prot_sides0(void *state) {
	enum parser_error r = parser_parse(state, "prot-sides:4");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	eq(e->ps, 4);
	ok;
}

static int test_flags0(void *state) {
	enum parser_error r = parser_parse(state, "flags:SEE_INVIS");
	struct ego_item *e;

	eq(r, PARSE_ERROR_NONE);
	e = parser_priv(state);
	require(e);
	require(e->flags);
	ok;
}

const char *suite_name = "parse/e-info";
struct test tests[] = {
	{ "name0", test_name0 },
	{ "attack0", test_attack0 },
	{ "dam_dice0", test_dam_dice0 },
	{ "dam_sides0", test_dam_sides0 },
	{ "evasion0", test_evasion0 },
	{ "prot_dice0", test_prot_dice0 },
	{ "prot_sides0", test_prot_sides0 },
	{ "flags0", test_flags0 },
	{ NULL, NULL }
};
