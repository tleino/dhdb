/*
 * dhdb - Multi-format dynamic and hierarchical database for C
 * Copyright (c) 2015 Tommi M. Leino <tleino@me.com>
 * 
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "dhdb_json.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

dhdb_t*
dhdb_create_from_xml(const char *str)
{
	dhdb_t *s;

	s = dhdb_create(NULL);
	assert(s);

	return s;
}

static const char*
_serialize(dhdb_t *json, char *out, int level)
{
	char val[8192];
	const char *name;
	int i;
	dhdb_t *n;

	val[0] = 0;

	for (i = 3; i < level; i++)
		strcat(out, "\t");

	if ((name = dhdb_name(json))) {
		strcat(out, "<");
		strcat(out, name);
		strcat(out, ">");
		strcat(out, "\n");
	}

	if (dhdb_type(json) == DHDB_VALUE_NUMBER)
		snprintf(val, sizeof(val), "%lf", dhdb_num(json));
	else if (dhdb_type(json) == DHDB_VALUE_STRING)
		snprintf(val, sizeof(val), "%s", dhdb_str(json));
	else if (dhdb_type(json) == DHDB_VALUE_BOOL)
		snprintf(val, sizeof(val), "%s",
		    dhdb_num(json) ? "true" : "false");
	else if (dhdb_type(json) == DHDB_VALUE_NULL)
		snprintf(val, sizeof(val), "null");

	for (i = 1; i < level; i++)
		strcat(out, "\t");

	strcat(out, val);
	if (dhdb_type(json) == DHDB_VALUE_STRING)
		strcat(out, "\n");

	n = dhdb_first(json);
	while (n) {
		_serialize(n, out, level + 1);
		n = dhdb_next(n);
	}

	for (i = 2; i < level; i++)
		strcat(out, "\t");

	if (name) {
		strcat(out, "</");
		strcat(out, name);
		strcat(out, ">");
		strcat(out, "\n");
	}

	return out;
}

const char*
dhdb_to_xml(dhdb_t *s)
{
	static char out[8192];

	assert(s);

	out[0] = '\0';
	strcat(out, "\n");
	_serialize(s, out, 0);

	return out;	
}
