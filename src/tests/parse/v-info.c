/* parse/v-info */

#include "unit-test.h"

#include "init.h"
#include "cave.h"
#include "generate.h"


int setup_tests(void **state) {
	z_info = mem_zalloc(sizeof(struct angband_constants));
	*state = init_parse_vault();
	return !*state;
}

int teardown_tests(void *state) {
	struct vault *v = parser_priv(state);
	string_free(v->name);
	string_free(v->text);
	string_free(v->typ);
	mem_free(v);
	mem_free(z_info);
	parser_destroy(state);
	return 0;
}

static int test_name0(void *state) {
	enum parser_error r = parser_parse(state, "name:round");
	struct vault *v;

	eq(r, PARSE_ERROR_NONE);
	v = parser_priv(state);
	require(v);
	require(streq(v->name, "round"));
	ok;
}

static int test_typ0(void *state) {
	enum parser_error r = parser_parse(state, "type:Lesser vault");
	struct vault *v;

	eq(r, PARSE_ERROR_NONE);
	v = parser_priv(state);
	require(v);
	require(streq(v->typ, "Lesser vault"));
	ok;
}

static int test_depth0(void *state) {
	enum parser_error r = parser_parse(state, "depth:15");
	struct vault *v;

	eq(r, PARSE_ERROR_NONE);
	v = parser_priv(state);
	eq(v->depth, 15);
	require(v);
	ok;
}

static int test_rarity0(void *state) {
	enum parser_error r = parser_parse(state, "rarity:25");
	struct vault *v;

	eq(r, PARSE_ERROR_NONE);
	v = parser_priv(state);
	eq(v->rarity, 25);
	require(v);
	ok;
}

static int test_flags0(void *state) {
	enum parser_error r = parser_parse(state, "flags:NO_ROTATION | LIGHT");
	struct vault *v;

	eq(r, PARSE_ERROR_NONE);
	v = parser_priv(state);
	eq(v->rarity, 25);
	require(v);
	require(v->flags);
	ok;
}

static int test_d0(void *state) {
	enum parser_error r0 = parser_parse(state, "D:  %%  ");
	enum parser_error r1 = parser_parse(state, "D: %  % ");
	struct vault *v;

	eq(r0, PARSE_ERROR_NONE);
	eq(r1, PARSE_ERROR_NONE);
	v = parser_priv(state);
	require(v);
	require(streq(v->text, "  %%   %  % "));
	ok;
}

const char *suite_name = "parse/v-info";
struct test tests[] = {
	{ "name0", test_name0 },
	{ "typ0", test_typ0 },
	{ "depth0", test_depth0 },
	{ "rarity0", test_rarity0 },
	{ "flags0", test_flags0 },
	{ "d0", test_d0 },
	{ NULL, NULL }
};
