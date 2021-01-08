#include "dhdb_ini.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

const char *_progName;
static dhdb_t* _test(const char *str) { printf("\033[1m%s: %s\033[0m\n", _progName, str); return dhdb_create(); }

int main(int argc, char **argv)
{
	_progName = argv[0];
	dhdb_t *s = _test("Ini export import loop");

	// Add some values that don't belong to category
	dhdb_set_obj_str(s, "noncategoryfield", "valfoo");

	dhdb_t *section_a = dhdb_create();
	dhdb_set_obj_str(section_a, "field", "value");
	dhdb_set_obj_str(section_a, "field2", "value2");

	dhdb_t *section_b = dhdb_create();
	dhdb_set_obj_str(section_b, "field3", "value3");
	dhdb_set_obj_str(section_b, "field4", "value4");

	dhdb_set_obj(s, "section_a", section_a);
	dhdb_set_obj(s, "section_b", section_b);

	const char *ini = dhdb_to_ini(s);
	//printf("\033[36m%s\033[0m", ini);
	dhdb_free(s);
	s = dhdb_create_from_ini(ini);
	assert(dhdb_type(s) == DHDB_VALUE_OBJECT);
	assert(!strcmp(dhdb_str_by(s, "noncategoryfield"), "valfoo"));
	assert(!strcmp(dhdb_str_by(dhdb_by(s, "section_a"), "field"), "value"));
	assert(!strcmp(dhdb_str_by(dhdb_by(s, "section_a"), "field2"), "value2"));
	assert(!strcmp(dhdb_str_by(dhdb_by(s, "section_b"), "field3"), "value3"));
	assert(!strcmp(dhdb_str_by(dhdb_by(s, "section_b"), "field4"), "value4"));
	dhdb_free(s);

	return 0;
}
