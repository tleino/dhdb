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

#include "dhdb_ini.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_BSD
#include <string.h>

static void _parse_section(dhdb_t *, const char *, dhdb_t **);
static void _parse_line(dhdb_t *, const char *, dhdb_t **);
static void _print_value(dhdb_t *, char *);
static const char* _serialize(dhdb_t *, char *, int);

dhdb_t*
dhdb_create_from_ini(const char *str)
{
	dhdb_t *s, *current_section = 0;
	int i, begin = 0;
	char *line;

	s = dhdb_create();

	for (i = 0; i < strlen(str); i++) {
		if (str[i] == '\n') {
			line = strdup(&str[begin]);
			line[i - begin] = 0;
			_parse_line(s, line, &current_section);
			free(line);
			begin = i + 1;
		}
	}

	return s;
}

const char*
dhdb_to_ini(dhdb_t *s)
{
	static char out[8192];

	assert(s);
	out[0] = 0;
	_serialize(s, out, 0);
	return out;	
}

static void
_parse_section(dhdb_t *s, const char *line, dhdb_t **current_section)
{
	char *section;

	section = strdup(&line[1]);
	section[strlen(section)-1] = 0;

	if (dhdb_by(s, section) == NULL) {
		dhdb_set_obj(s, section, dhdb_create());
	}
	*current_section = dhdb_by(s, section);
	free(section);
}

static void
_parse_line(dhdb_t *s, const char *line, dhdb_t **current_section)
{
	int i;
	char *p, *field, *value;

	if (line[0] == '[')
		return _parse_section(s, line, current_section);
	if (line[0] == ';' || line[0] == '#')
		return;

	p = strchr(line, '=');
	if (!p)
		return;

	field = strdup(line);
	field[p - line] = 0;
	value = strdup(&line[p - line + 1]);
	
	if (*current_section)
		dhdb_set_obj_str(*current_section, field, value);
	else
		dhdb_set_obj_str(s, field, value);

	free(field);
	free(value);
}

static void
_print_value(dhdb_t *s, char *out)
{
	char val[8192];
	val[0] = 0;

	if (dhdb_type(s) == DHDB_VALUE_NUMBER) {
		if ((int) dhdb_num(s) == dhdb_num(s))
			snprintf(val, sizeof(val), "%ld",
			    (long int) dhdb_num(s));
		else
			snprintf(val, sizeof(val), "%lf", dhdb_num(s));
	}
	else if (dhdb_type(s) == DHDB_VALUE_STRING)
		snprintf(val, sizeof(val), "%s", dhdb_str(s));
	else if (dhdb_type(s) == DHDB_VALUE_BOOL)
		snprintf(val, sizeof(val), "%s",
		    dhdb_num(s) ? "true" : "false");
	else if (dhdb_type(s) == DHDB_VALUE_NULL)
		snprintf(val, sizeof(val), "null");

	strcat(out, val);
}

static const char*
_serialize(dhdb_t *s, char *out, int level)
{
	char val[8192];
	const char *name;
	val[0] = 0;
	dhdb_t *n;

	name = dhdb_name(s);

	if (level == 1 && name && dhdb_type(s) == DHDB_VALUE_OBJECT) {
		strcat(out, "[");
		strcat(out, dhdb_name(s));
		strcat(out, "]\n");
	} else if (name && dhdb_type(s) != DHDB_VALUE_OBJECT) {
		strcat(out, dhdb_name(s));
		strcat(out, "=");
		_print_value(s, out);
		strcat(out, "\n");
	}

	n = dhdb_first(s);
	while (n) {
		_serialize(n, out, level + 1);
		n = dhdb_next(n);
	}

	return out;
}
