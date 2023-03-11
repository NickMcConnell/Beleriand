/*
 * effects/info
 * Test functions from effects-info.c for single effects; tests for chains are
 * in chain.c.
 */

#include "unit-test.h"
#include "test-utils.h"
#include "effects.h"
#include "effects-info.h"
#include "init.h"
#include "z-dice.h"

struct test_effects {
	struct effect *acid_bolt;
	struct effect *cold_sphere;
	struct effect *heal;
	struct effect *food;
	struct effect *cure_stun;
	struct effect *inc_fear;
	struct effect *inc_nores_blind;
	struct effect *set_fast;
	int avgd_acid_bolt;
	int avgd_cold_sphere;
};

static struct effect *build_effect(int index, const char *st_str,
		const char *d_str, int radius, int other) {
	struct effect *e = mem_zalloc(sizeof(*e));

	e->index = index;
	if (d_str) {
		e->dice = dice_new();
		if (!dice_parse_string(e->dice, d_str)) {
			free_effect(e);
			return NULL;
		}
	}
	e->subtype = effect_subtype(e->index, st_str);
	if (e->subtype == -1) {
		free_effect(e);
		return NULL;
	}
	e->radius = radius;
	e->other = other;
	return e;
}

int teardown_tests(void *state) {
	struct test_effects *te = state;

	if (te) {
		free_effect(te->set_fast);
		free_effect(te->inc_nores_blind);
		free_effect(te->inc_fear);
		free_effect(te->cure_stun);
		free_effect(te->food);
		free_effect(te->heal);
		free_effect(te->cold_sphere);
		free_effect(te->acid_bolt);
		mem_free(te);
	}
	cleanup_angband();
	return 0;
}

int setup_tests(void **state) {
	struct test_effects *te;
	bool failed;

	set_file_paths();
	init_angband();

	/* Set up some effects.  Remember expected average damage. */
	failed = false;
	te = mem_zalloc(sizeof(*te));
	te->acid_bolt = build_effect(EF_BOLT, "ACID", "2d8", 0, 0);
	te->avgd_acid_bolt = 9;
	if (!te->acid_bolt) failed = true;
	te->cold_sphere = build_effect(EF_SPHERE, "COLD", "2+3d1", 5, 0);
	te->avgd_cold_sphere = 5;
	if (!te->cold_sphere) failed = true;
	te->heal = build_effect(EF_HEAL_HP, "NONE", "13", 0, 0);
	if (!te->heal) failed = true;
	te->food = build_effect(EF_NOURISH, "INC_BY", "5", 0, 0);
	if (!te->food) failed = true;
	te->cure_stun = build_effect(EF_CURE, "STUN", NULL, 0, 0);
	if (!te->cure_stun) failed = true;
	te->inc_fear = build_effect(EF_TIMED_INC, "AFRAID", "30+1d10", 0, 0);
	if (!te->inc_fear) failed = true;
	te->inc_nores_blind = build_effect(EF_TIMED_INC_NO_RES, "BLIND",
		"40", 0, 0);
	if (!te->inc_nores_blind) failed = true;
	te->set_fast = build_effect(EF_TIMED_SET, "FAST", "15", 0, 0);
	if (!te->set_fast) failed = true;

	if (failed) {
		teardown_tests(te);
		return 1;
	}

	*state = te;
	return 0;
}

static int test_damages(void *state)
{
	struct test_effects *te = state;

	require(effect_damages(te->acid_bolt));
	require(effect_damages(te->cold_sphere));
	require(!effect_damages(te->heal));
	require(!effect_damages(te->food));
	require(!effect_damages(te->cure_stun));
	require(!effect_damages(te->inc_fear));
	require(!effect_damages(te->inc_nores_blind));
	require(!effect_damages(te->set_fast));
	ok;
}

static int test_avg_damage(void *state) {
	struct test_effects *te = state;

	eq(effect_avg_damage(te->acid_bolt), te->avgd_acid_bolt);
	eq(effect_avg_damage(te->cold_sphere), te->avgd_cold_sphere);
	//eq(effect_avg_damage(te->heal), 0);
	//eq(effect_avg_damage(te->food), 0);
	//eq(effect_avg_damage(te->cure_stun), 0);
	//eq(effect_avg_damage(te->inc_fear), 0);
	//eq(effect_avg_damage(te->inc_nores_blind), 0);
	//eq(effect_avg_damage(te->set_fast), 0);
	ok;
}

static int test_projection(void *state) {
	struct test_effects *te = state;

	require(streq(effect_projection(te->acid_bolt), "acid"));
	require(streq(effect_projection(te->cold_sphere), "frost"));
	require(streq(effect_projection(te->heal), ""));
	require(streq(effect_projection(te->food), ""));
	require(streq(effect_projection(te->cure_stun), ""));
	require(streq(effect_projection(te->inc_fear), ""));
	require(streq(effect_projection(te->inc_nores_blind), ""));
	require(streq(effect_projection(te->set_fast), ""));
	ok;
}

static int test_menu_name(void *state) {
	struct test_effects *te = state;
	char buf[80];
	size_t n;

	n = effect_get_menu_name(buf, sizeof(buf), te->acid_bolt);
	eq(n, strlen(buf));
	require(streq(buf, "cast a bolt of acid"));
	n = effect_get_menu_name(buf, sizeof(buf), te->cold_sphere);
	eq(n, strlen(buf));
	require(streq(buf, "project frost"));
	n = effect_get_menu_name(buf, sizeof(buf), te->heal);
	eq(n, strlen(buf));
	require(streq(buf, "heal self"));
	n = effect_get_menu_name(buf, sizeof(buf), te->food);
	eq(n, strlen(buf));
	require(streq(buf, "feed yourself"));
	n = effect_get_menu_name(buf, sizeof(buf), te->cure_stun);
	eq(n, strlen(buf));
	require(streq(buf, "cure stunning"));
	n = effect_get_menu_name(buf, sizeof(buf), te->inc_fear);
	eq(n, strlen(buf));
	require(streq(buf, "extend fear"));
	n = effect_get_menu_name(buf, sizeof(buf), te->inc_nores_blind);
	eq(n, strlen(buf));
	require(streq(buf, "extend blindness"));
	n = effect_get_menu_name(buf, sizeof(buf), te->set_fast);
	eq(n, strlen(buf));
	require(streq(buf, "administer haste"));
	ok;
}

const char *suite_name = "effects/info";
struct test tests[] = {
	{ "damages", test_damages },
	{ "average damage", test_avg_damage },
	{ "projection", test_projection },
	{ "menu name", test_menu_name },
	{ NULL, NULL }
};
