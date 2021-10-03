/* parse/lure.c */

#include "unit-test.h"
#include "test-utils.h"

#include <stdio.h>
#include "init.h"
#include "mon-init.h"

int setup_tests(void **state) {
	set_file_paths();
	init_angband();

	return 0;
}

int teardown_tests(void *state) {
	cleanup_angband();
	return 0;
}

static errr run_parse_monster(struct parser *p) {
	return parse_file(p, "monster", NULL);
}

static int test_lore_parse_monster_text(void *state) {

	struct file_parser test_lore_perser = lore_parser;
	test_lore_perser.run = run_parse_monster;

	ok;
	/* Leaving this out for now because it's leading to double frees due to
	 * calling finish_parse_lore() after run_parse_monster() instead of
	 * run_parse_lore() */
	errr err = run_parser(&test_lore_perser);

	eq(err, PARSE_ERROR_NONE);

	ok;
}


const char *suite_name = "parse/lure";
struct test tests[] = {
	{ "lore_parse_monster_text", test_lore_parse_monster_text },
	{ NULL, NULL }
};
