#include "dhdb_path.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <fnmatch.h>

static void _test_pick(dhdb_t *s, const char *path, int expected_elems)
{
	printf("TEST PICK %s\n", path);
	dhdb_path_iter_t *pi;
	dhdb_t *e = dhdb_path_pick_first(s, &pi, path);
	assert(e);
	int elems = 0;
	for (; e; e = dhdb_path_pick_next(pi)) {
		printf("\t#%d %s ... vs ... %s\n", elems, dhdb_path_name(e), path);
		assert(!fnmatch(path, dhdb_path_name(e), 0));
		elems++;
	}
	assert(elems == expected_elems);
	dhdb_path_pick_free(&pi);
	printf("TEST PICK %s WAS SUCCESSFUL\n", path);
}

static void _test_path_set(dhdb_t *s, const char *path, const char *value)
{
	printf("TEST PATH SET %s\n", path);

	dhdb_path_set_str(s, value, path);

	assert(!strcmp(dhdb_path_str(s, path), value));
	assert(!strcmp(dhdb_path_name(dhdb_path(s, path)), path));
}

static void test_multi()
{
	dhdb_path_internal_set_separator('.');

	dhdb_t *s = dhdb_create();
	// TODO:
	_test_path_set(s, "foobar.laalaa.fuubah", "String to change");

	_test_path_set(s, "foobar.laalaa.fuubah", "Hello world");
	_test_path_set(s, "foobar.laalaa.fuu2", "Hello again");
	_test_path_set(s, "foobar.laaloo.fuu2", "Hello again 2");
	_test_path_set(s, "foobar.foo.bar.baz", "4 Element path");

	assert(dhdb_len(s) == 1);

	dhdb_t *n = dhdb_path_first(s, "foobar.laalaa");
	for (; n; n = dhdb_next(n)) {
		assert(!strcmp(dhdb_path_name(n), "foobar.laalaa.fuubah") ||
			!strcmp(dhdb_path_name(n), "foobar.laalaa.fuu2"));
	}

	n = dhdb_path_first(s, "foobar.laaloo");
	for (; n; n = dhdb_next(n)) {
		assert(!strcmp(dhdb_path_name(n), "foobar.laaloo.fuu2"));
	}

	_test_pick(s, "foobar.laalaa.fuubah", 1);
	_test_pick(s, "foobar.laalaa.fuu2", 1);
	_test_pick(s, "foobar.laalaa.*", 2);
	_test_pick(s, "foobar.*.fuu2", 2);
	_test_pick(s, "foobar.foo.*", 1);
	_test_pick(s, "*.*.*.baz", 1);
	_test_pick(s, "*", 4);
	_test_pick(s, "foobar.*", 4);
	
	// TODO: expansion from left side and handling non-wildcard in middle
	//_test_pick(s, "*.laalaa.*", 3);
	//_test_pick(s, "*.fuu2", 2);
	
	dhdb_free(s);
}

int main(int argc, char **argv)
{
	test_multi();

	return 0;
}
