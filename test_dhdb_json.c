#include "dhdb_json.h"
#include "dhdb_dump.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

char *_progName;

static dhdb_t* _export_import(dhdb_t *s)
{
	const char *str = dhdb_to_json(s);
	dhdb_free(s);
	//printf("..export: \033[36m%s\033[0m\n", str);
	s = dhdb_create_from_json(str);
	return s;
}

static dhdb_t* _test(const char *name, const char *json, bool want_export_import)
{
	dhdb_t *s;
	printf("\033[1m%s: %s\033[0m\n", _progName, name);
	//printf("..import: \033[35m%s\033[0m\n", json);
	s = dhdb_create_from_json(json);
	if (want_export_import) s = _export_import(s);
	return s;
}

static void _test_parse(bool want_export_import)
{
	dhdb_t *s;
	const char *str;

	// String
	s = _test("String", "\"hello world\"", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_STRING);
	assert(!strcmp(dhdb_str(s), "hello world"));
	dhdb_free(s);

	// Positive number
	s = _test("Positive number", "3", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_NUMBER);
	assert(dhdb_num(s) == 3.0);
	dhdb_free(s);

	// Negative number
	s = _test("Negative number", "-3", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_NUMBER);
	assert(dhdb_num(s) == -3);
	dhdb_free(s);

	// Positive float
	s = _test("Positive float", "3.5", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_NUMBER);
	assert(dhdb_num(s) == 3.5);
	dhdb_free(s);

	// Negative float
	s = _test("Negative float", "-3.5", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_NUMBER);
	assert(dhdb_num(s) == -3.5);
	dhdb_free(s);

	// True
	s = _test("Boolean true", "true", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_BOOL);
	assert(dhdb_num(s) == 1);
	dhdb_free(s);

	// False
	s = _test("Boolean false", "false", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_BOOL);
	assert(dhdb_num(s) == 0);
	dhdb_free(s);

	// Null
	s = _test("Literal null", "null", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_NULL);
	assert(dhdb_num(s) == 0);
	dhdb_free(s);

	// Mixed array
	s = _test("Mixed array", "[ 1, -5.1, \"hello world\", false, true, null ]", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_ARRAY);
	assert(dhdb_num_at(s, 0) == 1);
	assert(dhdb_num_at(s, 1) == -5.1);
	assert(!strcmp(dhdb_str_at(s, 2), "hello world"));
	assert(dhdb_num_at(s, 3) == 0);
	assert(dhdb_num_at(s, 4) == 1);
	assert(dhdb_num_at(s, 5) == 0);
	dhdb_free(s);

	// Array within array
	s = _test("Array within array", "[ 1, -5.1, \"hello world\", [ 2, -3 ] ]", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_ARRAY);
	assert(dhdb_num_at(s, 0) == 1);
	assert(dhdb_num_at(s, 1) == -5.1);
	assert(!strcmp(dhdb_str_at(s, 2), "hello world"));
	assert(dhdb_type(dhdb_at(s, 3)) == DHDB_VALUE_ARRAY);
	assert(dhdb_num_at(dhdb_at(s, 3), 0) == 2);
	assert(dhdb_num_at(dhdb_at(s, 3), 1) == -3);
	dhdb_free(s);

	// Mixed object
	s = _test("Mixed object", "{ \"f1\" : \"val1\", \"f2\" : 3, \"f3\" : -3.5 }", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);
	assert(!strcmp(dhdb_str_by(s, "f1"), "val1"));
	assert(dhdb_num_by(s, "f2") == 3);
	assert(dhdb_num_by(s, "f3") == -3.5);
	dhdb_free(s);

	// Object within object
	s = _test("Object within object", "{ \"f1\" : \"val1\", \"f2\" : { \"foo\" : false, \"bar\" : 3 }, \"f3\" : -3 }", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);
	assert(!strcmp(dhdb_str_by(s, "f1"), "val1"));
	assert(dhdb_type(dhdb_by(s, "f2")) == DHDB_VALUE_OBJECT);
	assert(dhdb_num_by(dhdb_by(s, "f2"), "foo") == 0);
	assert(dhdb_num_by(dhdb_by(s, "f2"), "bar") == 3);
	assert(dhdb_num_by(s, "f3") == -3);
	dhdb_free(s);

	// Object within array
	s = _test("Object within array", "[ 1, { \"f1\" : true, \"f2\" : -3.5 }, 2 ]", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_ARRAY);
	assert(dhdb_type(dhdb_at(s, 1)) == DHDB_VALUE_OBJECT);
	assert(dhdb_num_at(s, 0) == 1);
	assert(dhdb_num_by(dhdb_at(s, 1), "f1") == 1);
	assert(dhdb_num_by(dhdb_at(s, 1), "f2") == -3.5);
	assert(dhdb_num_at(s, 2) == 2);
	dhdb_free(s);

	// Array within object
	s = _test("Array within object", "{ \"f1\" : [ 2, 1, 3 ], \"f2\" : 3, \"f3\" : -3.5 }", want_export_import);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);
	assert(dhdb_type(dhdb_by(s, "f1")) == DHDB_VALUE_ARRAY);
	assert(dhdb_num_at(dhdb_by(s, "f1"), 0) == 2);
	assert(dhdb_num_at(dhdb_by(s, "f1"), 1) == 1);
	assert(dhdb_num_at(dhdb_by(s, "f1"), 2) == 3);
	assert(dhdb_num_by(s, "f2") == 3);
	assert(dhdb_num_by(s, "f3") == -3.5);
	dhdb_free(s);

	// Error
	s = _test("Error handling 1", "{ \"f1\" : [ af, foop ] }", false);
	assert(s == NULL);

	s = _test("Error handling 2", "{ \"f1\" : [ 1.5, 1.a, 1 ] }", false);
	assert(s == NULL);

	s = _test("Error handling 3", "{ \"f1\" : [ 1, 2 }", false);
	assert(s == NULL);

	s = _test("Error handling 4", "{ \"f1 : [ 1, 2 ]", false);
	assert(s == NULL);

	s = _test("Error handling 5", "{ f1 : [ 1, 2 ] }", false);
	assert(s == NULL);

	s = _test("Error handling 6", "{ \"f1\" : { 1, 2 ] }", false);
	assert(s == NULL);

	// Test file
	s = dhdb_create_from_json_file("test.json");
	dhdb_dump(s);
	assert(s);
	dhdb_free(s);
}

int main(int argc, char **argv)
{
	_progName = argv[0];
	_test_parse(true);
	return 0;
}
