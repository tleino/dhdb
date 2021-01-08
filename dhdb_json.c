/* 
 * [ISC LICENSE]
 * Copyright (c) 2015, Tommi Leino <tleino@me.com>
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
#include <stdlib.h>
#include <assert.h>
#define __USE_BSD
#include <string.h>
#include <stdbool.h>

#include <ctype.h>

static const char *jsonValueTxt[] = {
	"undefined", "object", "array", "number", "string", "bool", "null" 
};

static int _parse(const char *str, dhdb_t *json, uint8_t type)
{
	int i;
	int valBegin = 0;
	int haveMemberBegin = 0;
	int haveObjectName = 0;
	dhdb_t *currentObject = 0;
	bool bool_val = false;

	for (i = 0; i < strlen(str); i++) {
		if (type == DHDB_VALUE_UNDEFINED && isspace(str[i])) {
			continue;
		}

		// Determine type based on first non-space byte
		if (type == DHDB_VALUE_UNDEFINED) {
			valBegin = i;

			if (str[i] == '\"') { // Assume string
				type = DHDB_VALUE_STRING;
				valBegin++;
				continue;
			}
			else if (isdigit(str[i]) || str[i] == '-') { // Assume number
				type = DHDB_VALUE_NUMBER;
				if (str[i] == '-') continue;
			}
			else if (str[i] == '[') { // Assume array
				type = DHDB_VALUE_ARRAY;
				continue;
			}
			else if (str[i] == '{') { // Assume object
				type = DHDB_VALUE_OBJECT;
				continue;
			}
			else if (!strncmp(&str[i], "false", 5)) {
				type = DHDB_VALUE_BOOL;
				bool_val = false;
				i += 5 - 1;
			}
			else if (!strncmp(&str[i], "true", 4)) {
				type = DHDB_VALUE_BOOL;
				bool_val = true; 
				i += 4 - 1;
			}
			else if (!strncmp(&str[i], "null", 4)) {
				type = DHDB_VALUE_NULL;
				i += 4 - 1;
			}
			else {
				printf("\033[1;31mUnknown json type at %s\033[0m\n", &str[i]);
				type = DHDB_VALUE_UNDEFINED;
				return i;
			}
		}
		// Ensure type doesn't change midflight & look for end, for each type
		if (type == DHDB_VALUE_STRING) {
			if (str[i] == '\"') {
				char *str2 = strdup(&str[valBegin]);
				str2[i - valBegin] = 0;
				dhdb_set_str(json, str2);
				free(str2);
				return i;
			}
		}
		if (type == DHDB_VALUE_BOOL || type == DHDB_VALUE_NULL) {
			if (isspace(str[i]) || i == strlen(str) - 1 || str[i] == ',') {
				if (type == DHDB_VALUE_NULL) dhdb_set_null(json);
				else if (type == DHDB_VALUE_BOOL) dhdb_set_bool(json, bool_val);
				return i;
			}
		}
		if (type == DHDB_VALUE_NUMBER) {
			if (isspace(str[i]) || i == strlen(str) - 1 || str[i] == ',') {
				char *s = strdup(&str[valBegin]);
				if (i != strlen(str) - 1) s[i - valBegin] = 0;
				dhdb_set_num(json, atof(s));
				free(s);
				return i;
			}
			if (!isdigit(str[i]) && str[i] != '.') {
				printf("\033[1;31mType error at %s\033[0m\n", &str[i]);
				type = DHDB_VALUE_UNDEFINED;
				return i;
			}
		}
		if (type == DHDB_VALUE_ARRAY) {
			if (isspace(str[i])) continue;
			if (str[i] == ']') return i;
			if (str[i] == ',') continue;

			dhdb_t *val = dhdb_create(NULL);
			dhdb_add(json, val);

			i += _parse(&str[i], val, DHDB_VALUE_UNDEFINED);
		}

		if (type == DHDB_VALUE_OBJECT && !haveObjectName) {
			if (isspace(str[i])) continue;
			if (!haveMemberBegin && str[i] == '\"') {
				valBegin = i + 1;
				haveMemberBegin = 1;
				continue;
			}
			if (str[i] == '\"') {
				char *field = strdup(&str[valBegin]);
				field[i - valBegin] = 0;
				haveObjectName = 1;
				
				dhdb_t *val = dhdb_create(NULL);
				dhdb_set_obj(json, field, val);
				currentObject = val;
				free(field);
			}
			if (str[i] == '}') return i;
		}
		else if (type == DHDB_VALUE_OBJECT && currentObject && dhdb_type(currentObject) == DHDB_VALUE_UNDEFINED) {
			if (isspace(str[i])) continue;
			if (str[i] == ':') {
				i ++;

				i += _parse(&str[i], currentObject, DHDB_VALUE_UNDEFINED);

				currentObject = 0;
				haveObjectName = 0;
				haveMemberBegin = 0;
			} else {
				printf("ERROR, EXPECTED ':' GOT SOMETHING ELSE!\n");
			}
		}
	}
	return 0;
}

dhdb_t* dhdb_create_from_json(const char *str)
{
	dhdb_t *s = dhdb_create(NULL);
	assert(s);

	_parse(str, s, DHDB_VALUE_UNDEFINED);

	return s;
}

static const char* _serialize(dhdb_t *json, char *out, int level, bool pretty)
{
	char val[8192];
	val[0] = 0;

	const char *name = dhdb_name(json);
	if (name) {
		strcat(out, "\"");
		strcat(out, name);
		strcat(out, "\" : ");
	}

	if (dhdb_type(json) == DHDB_VALUE_NUMBER) {
		if ((int) dhdb_num(json) == dhdb_num(json)) snprintf(val, sizeof(val), "%ld", (long int) dhdb_num(json));
		else snprintf(val, sizeof(val), "%lf", dhdb_num(json));
	}
	else if (dhdb_type(json) == DHDB_VALUE_STRING) snprintf(val, sizeof(val), "\"%s\"", dhdb_str(json));
	else if (dhdb_type(json) == DHDB_VALUE_BOOL) snprintf(val, sizeof(val), "%s", dhdb_num(json) ? "true" : "false");
	else if (dhdb_type(json) == DHDB_VALUE_NULL) snprintf(val, sizeof(val), "null");

	strcat(out, val);

	dhdb_t *n = dhdb_first(json);

	if (name && n && pretty) {
		strcat(out, "\n");
		if (pretty) for (int i = 0; i < level; i++) { strcat(out, "  "); }
	}
	if (dhdb_type(json) == DHDB_VALUE_ARRAY) {  strcat(out, "[ ");  }
	else if (dhdb_type(json) == DHDB_VALUE_OBJECT) {  strcat(out, "{ ");  }

	while (n) {
		_serialize(n, out, level + 1, pretty);
		if (dhdb_next(n)) {
			strcat(out, ",");
			if (pretty) { strcat(out, "\n"); for (int i = 0; i < level + 1; i++) strcat(out, "  "); }
		}
		else {
			strcat(out, " ");
		}
	
		n = dhdb_next(n);
	}

	if (pretty && (dhdb_type(json) == DHDB_VALUE_ARRAY || dhdb_type(json) == DHDB_VALUE_OBJECT)) {
		strcat(out, "\n");
		for (int i = 0; i < level; i++) { strcat(out, "  "); }
	}
	if (dhdb_type(json) == DHDB_VALUE_ARRAY) { strcat(out, "]"); }
	else if (dhdb_type(json) == DHDB_VALUE_OBJECT) { strcat(out, "}"); }

	return out;
}

const char* dhdb_to_json(dhdb_t *s)
{
	assert(s);
	static char out[64000];
	out[0] = 0;
	_serialize(s, out, 0, false);
	strcat(out, "\n");
	return out;	
}

const char* dhdb_to_json_pretty(dhdb_t *s)
{
	assert(s);
	static char out[64000];
	out[0] = 0;
	_serialize(s, out, 0, true);
	return out;	
}

