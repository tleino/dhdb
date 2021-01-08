#include "dhdb_path.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(int argc, char **argv)
{
	dhdb_t *s = dhdb_create(NULL);
	dhdb_add_num(s, 34);
	dhdb_add_num(s, 21.1);
	dhdb_t *a = dhdb_create(NULL);
	dhdb_add_num(a, 10);
	dhdb_add_num(a, 20);
	dhdb_add(s, a);
	dhdb_t *o = dhdb_create(NULL);
	dhdb_set_obj_str(o, "field", "value");
	dhdb_set_obj_str(o, "field2", "value2");
	dhdb_add(s, o);

	dhdb_t *p = dhdb_path(s, "0", NULL);
	assert(dhdb_num(p) == 34);

	p = dhdb_path(s, "2/1", NULL);
	assert(dhdb_num(p) == 20);

	p = dhdb_path(s, "3/field", NULL);
	assert(!strcmp(dhdb_str(p), "value"));

	p = dhdb_path(s, "3/field2", NULL);
	assert(!strcmp(dhdb_str(p), "value2"));

	p = dhdb_path(s, "3/field2", NULL);
	assert(!strcmp(dhdb_str(p), "value2"));

	p = dhdb_path(s, "2/1", NULL);
	assert(dhdb_num(p) == 20);

	dhdb_free(s);
	return 0;
}
