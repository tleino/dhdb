#include "dhdb.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <stdbool.h>

const char *_progName;

static dhdb_t* _test(const char *str) { printf("\033[1m%s: %s\033[0m\n", _progName, str); return dhdb_create(); }

static void test_str()
{
	dhdb_t *s = _test("String set and change");
	dhdb_set_str(s, "hello world");
	assert(!strcmp(dhdb_str(s), "hello world"));
	dhdb_set_str(s, "world hello");
	assert(!strcmp(dhdb_str(s), "world hello"));
	dhdb_free(s);
}

static void test_num()
{
	dhdb_t *s = _test("Set number and change");
	dhdb_set_num(s, 34);
	assert(dhdb_num(s) == 34);
	dhdb_set_num(s, 34.5);
	assert(dhdb_num(s) == 34.5);
	dhdb_set_num(s, -34.5);
	assert(dhdb_num(s) == -34.5);
	dhdb_free(s);
}

static void test_literals()
{
	dhdb_t *s = _test("Booleans and null and change");
	dhdb_set_bool(s, true);
	assert(dhdb_type(s) == DHDB_VALUE_BOOL);
	assert(dhdb_num(s) == 1);
	dhdb_set_bool(s, false);
	assert(dhdb_num(s) == 0);
	dhdb_set_null(s);
	assert(dhdb_type(s) == DHDB_VALUE_NULL);
	assert(dhdb_num(s) == 0);
	dhdb_set_bool(s, true);
	assert(dhdb_type(s) == DHDB_VALUE_BOOL);
	assert(dhdb_num(s) == 1);
	dhdb_free(s);
}

static void test_mixed_array()
{
	dhdb_t *s = _test("Mixed array");
	dhdb_add_str(s, "hello");
	dhdb_add_num(s, 31);
	dhdb_add_num(s, 31.33);
	assert(!strcmp(dhdb_str_at(s, 0), "hello"));
	assert(dhdb_num_at(s, 1) == 31);
	assert(dhdb_num_at(s, 2) == 31.33);
	assert(dhdb_len(s) == 3);
	assert(dhdb_type(s) == DHDB_VALUE_ARRAY);

	dhdb_free(s);
}

static void test_array_within_array()
{
	// Begin first level array
	dhdb_t *s = _test("Array within array");
	dhdb_add_str(s, "hello");

	// Create second level array
	dhdb_t *o = dhdb_create(NULL);
	dhdb_add_str(o, "world");
	dhdb_add_num(o, 2015);

	// Add second level array to the first
	dhdb_add(s, o);

	// Add still one item to first level array
	dhdb_add_num(s, 2014.3);

	assert(!strcmp(dhdb_str_at(s, 0), "hello"));
	assert(!strcmp(dhdb_str_at(dhdb_at(s, 1), 0), "world"));
	assert(dhdb_num_at(dhdb_at(s, 1), 1) == 2015);
	assert(dhdb_num_at(s, 2) == 2014.3);
	assert(dhdb_len(s) == 3);
	assert(dhdb_len(dhdb_at(s, 1)) == 2);
	
	dhdb_free(s);	
}

static void test_object_within_array()
{
	dhdb_t *s = _test("Object within array");
	dhdb_add_str(s, "hello");
	dhdb_add_num(s, 31);
	dhdb_add_num(s, 31.33);
	dhdb_t *o = dhdb_create(NULL);
	dhdb_set_obj_str(o, "mem", "ber");
	dhdb_set_obj_str(o, "f2", "v2");
	dhdb_add(s, o);
	dhdb_free(s);
}

static void test_mixed_object()
{
	dhdb_t *s = _test("Mixed object and changing values");
	dhdb_set_obj_num(s, "hello", 31);
	dhdb_set_obj_num(s, "halla", 31.33);
	dhdb_set_obj_str(s, "foops", "world");
	assert(dhdb_num_by(s, "hello") == 31);
	assert(dhdb_num_by(s, "halla") == 31.33);
	assert(!strcmp(dhdb_str_by(s, "foops"), "world"));
	assert(dhdb_len(s) == 3);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);
	dhdb_set_obj_str(s, "foops", "faaps");
	assert(!strcmp(dhdb_str_by(s, "foops"), "faaps"));

	dhdb_free(s);
}

void test_array_within_object()
{
	dhdb_t *s = _test("Array within object");

	dhdb_t *a = dhdb_create(NULL);
	dhdb_add_num(a, 34);
	dhdb_add_num(a, 21.1);

	dhdb_t *o = dhdb_create(NULL);
	dhdb_set_obj_str(o, "mem", "ber");

	dhdb_add(a, o);

	dhdb_set_obj(s, "hello", a);
	dhdb_set_obj_num(s, "halla", 31.33);

	assert(dhdb_num_at(dhdb_by(s, "hello"), 0) == 34);
	assert(dhdb_num_at(dhdb_by(s, "hello"), 1) == 21.1);
	assert(!strcmp(dhdb_str_by(dhdb_at(dhdb_by(s, "hello"), 2), "mem"), "ber"));
	assert(dhdb_num_by(s, "halla") == 31.33);
	assert(dhdb_len(s) == 2);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);

	dhdb_free(s);
}

void test_type_changes()
{
	dhdb_t *s = _test("Type changes");

	dhdb_add_num(s, 34);
	dhdb_add_num(s, 21.1);
	assert(dhdb_type(s) == DHDB_VALUE_ARRAY);

	dhdb_set_str(s, "hello");
	assert(dhdb_type(s) == DHDB_VALUE_STRING);
	assert(!strcmp(dhdb_str(s), "hello"));

	dhdb_set_num(s, 34);
	assert(dhdb_type(s) == DHDB_VALUE_NUMBER);
	assert(dhdb_num(s) == 34);

	dhdb_set_obj_num(s, "hello", 34);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);
	assert(dhdb_num_by(s, "hello") == 34); 

	dhdb_free(s);
}

void test_empty_array()
{
	dhdb_t *s = _test("Test empty array");
	dhdb_add_num(s, 34);
	dhdb_add_num(s, 21.1);
	dhdb_set_array(s);
	assert(dhdb_len(s) == 0);
	assert(dhdb_type(s) == DHDB_VALUE_ARRAY);
	dhdb_free(s);
}

int main(int argc, char **argv)
{
	_progName = argv[0];

	test_str();
	test_num();
	test_literals();
	test_mixed_array();
	test_mixed_object();
	test_array_within_array();
	test_object_within_array();
	test_array_within_object();
	test_type_changes();
	test_empty_array();
	
	return 0;
}
