/* object/attack */

#include "unit-test.h"
#include "unit-test-data.h"

#include "object.h"
#include "obj-make.h"
#include "player-attack.h"

int setup_tests(void **state) {
	z_info = mem_zalloc(sizeof(struct angband_constants));
	z_info->k_max = 1;
	k_info = mem_zalloc(z_info->k_max * sizeof(*k_info));
	return !*state;
}

int teardown_tests(void *state) {
	int k;

	for (k = 1; k < z_info->k_max; ++k) {
		struct object_kind *kind = &k_info[k];

		string_free(kind->name);
		string_free(kind->text);
		string_free(kind->effect_msg);
		mem_free(kind->brands);
		mem_free(kind->slays);
	}
	mem_free(k_info);
	mem_free(z_info);
	return 0;
}

static int test_breakage_chance(void *state) {
	struct object obj;
	int c;

	object_prep(&obj, &test_longsword, 1, AVERAGE);
	c = breakage_chance(&obj, true);
	eq(c, 100);
	c = breakage_chance(&obj, false);
	eq(c, 50);
	obj.artifact = &test_artifact_sword;
	c = breakage_chance(&obj, true);
	eq(c, 0);
	c = breakage_chance(&obj, false);
	eq(c, 0);
	ok;
}

const char *suite_name = "object/attack";
struct test tests[] = {
	//{ "breakage-chance", test_breakage_chance },
	{ NULL, NULL }
};
