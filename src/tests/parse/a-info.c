/* parse/a-info */

#include "unit-test.h"
#include "unit-test-data.h"
#include "effects.h"
#include "obj-tval.h"
#include "obj-util.h"
#include "object.h"
#include "init.h"
	
int setup_tests(void **state) {
	*state = init_parse_artifact();
	/* Do the bare minimum so kind lookups work. */
	z_info = mem_zalloc(sizeof(*z_info));
	z_info->k_max = 1;
	z_info->ordinary_kind_max = 1;
	k_info = mem_zalloc(z_info->k_max * sizeof(*k_info));
	kb_info = mem_zalloc(TV_MAX * sizeof(*kb_info));
	kb_info[TV_LIGHT].tval = TV_LIGHT;
	return !*state;
}

int teardown_tests(void *state) {
	struct artifact *a = parser_priv(state);
	int k;

	string_free(a->name);
	string_free(a->text);
	mem_free(a);
	for (k = 1; k < z_info->k_max; ++k) {
		struct object_kind *kind = &k_info[k];

		string_free(kind->name);
		string_free(kind->text);
		string_free(kind->effect_msg);
		mem_free(kind->brands);
		mem_free(kind->slays);
		free_effect(kind->effect);
	}
	mem_free(k_info);
	mem_free(kb_info);
	mem_free(z_info);
	parser_destroy(state);
	return 0;
}

static int test_name0(void *state) {
	enum parser_error r = parser_parse(state, "name:of Thrain");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	require(streq(a->name, "of Thrain"));
	ok;
}

static int test_badtval0(void *state) {
	enum parser_error r = parser_parse(state, "base-object:badtval:Junk");
	eq(r, PARSE_ERROR_UNRECOGNISED_TVAL);
	ok;
}

static int test_badtval1(void *state) {
	enum parser_error r = parser_parse(state, "base-object:-1:Junk");
	eq(r, PARSE_ERROR_UNRECOGNISED_TVAL);
	ok;
}

static int test_base_object0(void *state) {
	enum parser_error r = parser_parse(state, "base-object:light:Arkenstone");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	eq(a->tval, TV_LIGHT);
	eq(a->sval, z_info->ordinary_kind_max);
	ok;
}

static int test_level0(void *state) {
	enum parser_error r = parser_parse(state, "depth:3");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	eq(a->level, 3);
	ok;
}

static int test_weight0(void *state) {
	enum parser_error r = parser_parse(state, "weight:8");
	struct artifact *a;
	struct object_kind *k;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	eq(a->weight, 8);
	k = lookup_kind(a->tval, a->sval);
	noteq(k, NULL);
	if (k->kidx >= z_info->ordinary_kind_max) {
		eq(k->weight, 8);
	}
	ok;
}

static int test_cost0(void *state) {
	enum parser_error r = parser_parse(state, "cost:200");
	struct artifact *a;
	struct object_kind *k;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	eq(a->cost, 200);
	k = lookup_kind(a->tval, a->sval);
	noteq(k, NULL);
	if (k->kidx >= z_info->ordinary_kind_max) {
		eq(k->cost, 200);
	}
	ok;
}

static int test_attack0(void *state) {
	enum parser_error r = parser_parse(state, "attack:2:4d5");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	eq(a->att, 2);
	eq(a->dd, 4);
	eq(a->ds, 5);
	ok;
}

static int test_defence0(void *state) {
	enum parser_error r = parser_parse(state, "defence:-3:1d7");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	eq(a->evn, -3);
	eq(a->pd, 1);
	eq(a->ps, 7);
	ok;
}

static int test_flags0(void *state) {
	enum parser_error r = parser_parse(state, "flags:SEE_INVIS | FREE_ACT");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	require(a);
	require(a->flags);
	ok;
}

static int test_desc0(void *state) {
	enum parser_error r = parser_parse(state, "desc:baz");
	struct artifact *a;

	eq(r, 0);
	r = parser_parse(state, "desc: quxx");
	eq(r, 0);
	a = parser_priv(state);
	require(a);
	require(streq(a->text, "baz quxx"));
	ok;
}

static int test_values0(void *state) {
	enum parser_error r = parser_parse(state, "values:STR[1] | CON[1]");
	struct artifact *a;

	eq(r, PARSE_ERROR_NONE);
	a = parser_priv(state);
	eq(a->modifiers[0], 1);
	eq(a->modifiers[2], 1);
	ok;
}

const char *suite_name = "parse/a-info";
struct test tests[] = {
	{ "name0", test_name0 },
	{ "badtval0", test_badtval0 },
	{ "badtval1", test_badtval1 },
	{ "base-object0", test_base_object0 },
	{ "level0", test_level0 },
	{ "weight0", test_weight0 },
	{ "cost0", test_cost0 },
	{ "attack0", test_attack0 },
	{ "defence0", test_defence0 },
	{ "flags0", test_flags0 },
	{ "desc0", test_desc0 },
	{ "values0", test_values0 },
	{ NULL, NULL }
};
