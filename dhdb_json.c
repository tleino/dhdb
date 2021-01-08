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
#include "dhdb_dump.h"

#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#ifndef __USE_XOPEN2K8
#define __USE_XOPEN2K8
#endif
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>

#include <ctype.h>

static const char *jsonValueTxt[] = {
	"undefined", "object", "array", "number", "string", "bool", "null" 
};

static const char *_parse_error[] = {
	"Successful", "Expected ':'", "Unknown type",
	"Expected digit or '.'", "Closing quote not found", "Expected string"
};

static int
_parse(const char *str, dhdb_t *json, uint8_t type, int col,
    int *err_code, int *err_col)
{
	int i, add;
	int valBegin = 0;
	int haveMemberBegin = 0;
	int haveObjectName = 0;
	dhdb_t *currentObject = 0, *val;
	bool bool_val = false;
	int sz = strlen(str);
	char *field;
	
	for (i = 0; i < sz; i++) {
		if (type == DHDB_VALUE_UNDEFINED && isspace(str[i]))
			continue;

		/* Determine type based on first non-space byte */
		if (type == DHDB_VALUE_UNDEFINED) {
			valBegin = i;

			if (str[i] == '\"') {
				type = DHDB_VALUE_STRING;
				valBegin++;
				continue;
			}
			else if (isdigit(str[i]) || str[i] == '-') {
				type = DHDB_VALUE_NUMBER;
				if (str[i] == '-') continue;
			}
			else if (str[i] == '[') {
				type = DHDB_VALUE_ARRAY;
				continue;
			}
			else if (str[i] == '{') {
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
				*err_code = 2;
				*err_col = col + i;
				type = DHDB_VALUE_UNDEFINED;
				return 0;
			}
		}
		/*
		 * Ensure type doesn't change midflight & look for end,
		 * for each type
		 */
		if (type == DHDB_VALUE_STRING) {
			if (str[i] == '\"') {
				dhdb_set_str_len(json, i - valBegin,
				    &str[valBegin]);
				return i;
			}
		}
		if (type == DHDB_VALUE_BOOL || type == DHDB_VALUE_NULL) {
			if (isspace(str[i]) || i == sz - 1 || str[i] == ',') {
				if (type == DHDB_VALUE_NULL)
					dhdb_set_null(json);
				else if (type == DHDB_VALUE_BOOL)
					dhdb_set_bool(json, bool_val);
				return i;
			}
		}
		if (type == DHDB_VALUE_NUMBER) {
			if (isspace(str[i]) || i == sz - 1 || str[i] == ',') {
				char *s;
				if (i != sz - 1)
					s = strndup(&str[valBegin],
					    i - valBegin);
				else
					s = strdup(&str[valBegin]);
				dhdb_set_num(json, atof(s));
				free(s);
				return i;
			}
			if (!isdigit(str[i]) && str[i] != '.' &&
			    str[i] != 'e' && str[i] != 'E' &&
			    str[i] != '+' && str[i] != '-') {
				type = DHDB_VALUE_UNDEFINED;
				*err_code = 3;
				*err_col = col + i;
				return 0;
			}
		}
		if (type == DHDB_VALUE_ARRAY) {
			if (str[i] == ' ' || str[i] == '\t')
				continue;
			if (str[i] == ']')
				return i;
			if (str[i] == ',')
				continue;

			val = dhdb_create(NULL);
			dhdb_add(json, val);

			add = _parse(&str[i], val, DHDB_VALUE_UNDEFINED,
			    col + i, err_code, err_col);
			if (add == 0)
				break;
			i += add;
		}

		if (type == DHDB_VALUE_OBJECT && !haveObjectName) {
			if (isspace(str[i]))
				continue;
			if (!haveMemberBegin && str[i] == '\"') {
				valBegin = i + 1;
				haveMemberBegin = 1;
				continue;
			}
			if (str[i] == '\"') {
				field = strndup(&str[valBegin],
				    i - valBegin);
				haveObjectName = 1;
				
				val = dhdb_create(NULL);
				dhdb_set_obj(json, field, val);
				currentObject = val;
				free(field);
			}
			else if (!haveMemberBegin && str[i] == '}')
				return i;
			else if (haveMemberBegin && i == sz - 1) {
				*err_code = 4;
				*err_col = col + (valBegin - 1);
				return 0;
			} else if (!haveMemberBegin && str[i] != ',') {
				*err_code = 5;
				*err_col = col + i;
				return 0;
			}
			
		}
		else if (type == DHDB_VALUE_OBJECT && currentObject &&
		    dhdb_type(currentObject) == DHDB_VALUE_UNDEFINED) {
			if (isspace(str[i]))
				continue;
			if (str[i] == ':') {
				i++;

				add = _parse(&str[i], currentObject,
				    DHDB_VALUE_UNDEFINED, col + i, err_code,
				    err_col);
				if (add == 0)
					break;
				i += add;

				currentObject = 0;
				haveObjectName = 0;
				haveMemberBegin = 0;
			} else {
				*err_code = 1;
				*err_col = col + i;
				return 0;
				//printf("ERROR, EXPECTED ':' GOT SOMETHING ELSE!\n");
			}
		}
	}
	return 0;
}

dhdb_t*
dhdb_create_from_json(const char *str)
{
	dhdb_t *s;
	int err_code = 0, err_col = 0;
	int beginI, endI;
	char *err_line;

	s = dhdb_create(NULL);
	assert(s);

	_parse(str, s, DHDB_VALUE_UNDEFINED, 0, &err_code, &err_col);
	if (err_code) {
		dhdb_dump(s);
		fprintf(stderr, "%s: Error '%s' at byte %d of %zu:\n",
		    __FUNCTION__,
		    _parse_error[err_code], err_col, strlen(str));
		beginI = err_col - 30;
		endI = err_col + 30;
		if (beginI < 0) beginI = 0;
		if (endI > strlen(str) - 1) endI = strlen(str) - 1;
		err_line = strndup(&str[beginI], beginI - endI);
		for (int i = 0; i < 10; i++)
			fprintf(stderr, " ");
		for (int i = 0; i < strlen(err_line); i++)
			fprintf(stderr, "%c", err_line[i]);
		fprintf(stderr, "\n");
		free(err_line);
		
		dhdb_free(s);
		return NULL;
	}

	return s;
}

dhdb_t*
dhdb_create_from_json_file(const char *fmt, ...)
{
	char file[1024], *buf;
	va_list args;
	FILE *fp;
	struct stat statbuf;
	dhdb_t *s;

	va_start(args, fmt);
	if (vsnprintf(file, sizeof(file), fmt, args) >= sizeof(file)) {
		fprintf(stderr, "File name '%s' was too long\n", file);
		return 0;
	}
	va_end(args);

	fp = fopen(file, "r");
	if (!fp) {
		fprintf(stderr, "Couldn't open %s\n", file);
		return 0;
	}

	if (fstat(fileno(fp), &statbuf) == -1) {
		fprintf(stderr, "Couldn't stat %s\n", file);
		return 0;
	}

	buf = malloc(statbuf.st_size + 1);
	if (buf == 0) {
		fprintf(stderr, "Couldn't malloc %lld bytes space for %s\n", statbuf.st_size, file);
		return 0;
	}
	int n = fread(buf, sizeof(char), statbuf.st_size, fp);
	if (n < statbuf.st_size) {
		fprintf(stderr, "Error while freading %s\n", file);
		return 0;
	}
	buf[statbuf.st_size] = 0;
	
	s = dhdb_create_from_json(buf);

	fclose(fp);
	free(buf);

	return s;
}

static const char*
_serialize(dhdb_t *json, char *out, int level, bool pretty)
{
	char val[8192];
	const char *name;
	dhdb_t *n;

	val[0] = 0;
	name = dhdb_name(json);
	if (name) {
		strcat(out, "\"");
		strcat(out, name);
		strcat(out, "\" : ");
	}

	if (dhdb_type(json) == DHDB_VALUE_NUMBER) {
		if ((int) dhdb_num(json) == dhdb_num(json))
			snprintf(val, sizeof(val), "%ld",
			    (long int) dhdb_num(json));
		else
			snprintf(val, sizeof(val), "%.8f", dhdb_num(json));
	}
	else if (dhdb_type(json) == DHDB_VALUE_STRING)
		snprintf(val, sizeof(val), "\"%s\"", dhdb_str(json));
	else if (dhdb_type(json) == DHDB_VALUE_BOOL)
		snprintf(val, sizeof(val), "%s",
		    dhdb_num(json) ? "true" : "false");
	else if (dhdb_type(json) == DHDB_VALUE_NULL)
		snprintf(val, sizeof(val), "null");

	strcat(out, val);

	n = dhdb_first(json);

	if (name && n && pretty) {
		strcat(out, "\n");
		if (pretty)
			for (int i = 0; i < level; i++)
				strcat(out, "  ");
	}
	if (dhdb_type(json) == DHDB_VALUE_ARRAY)
		strcat(out, "[ ");
	else if (dhdb_type(json) == DHDB_VALUE_OBJECT)
		strcat(out, "{ ");

	while (n) {
		_serialize(n, out, level + 1, pretty);
		if (dhdb_next(n)) {
			strcat(out, ",");
			if (pretty) {
				strcat(out, "\n");
				for (int i = 0; i < level + 1; i++)
					strcat(out, "  ");
			}
		}
		else
			strcat(out, " ");
	
		n = dhdb_next(n);
	}

	if (pretty && (dhdb_type(json) == DHDB_VALUE_ARRAY ||
	    dhdb_type(json) == DHDB_VALUE_OBJECT)) {
		strcat(out, "\n");
		for (int i = 0; i < level; i++)
			strcat(out, "  ");
	}
	if (dhdb_type(json) == DHDB_VALUE_ARRAY)
		strcat(out, "]");
	else if (dhdb_type(json) == DHDB_VALUE_OBJECT)
		strcat(out, "}");

	return out;
}

const char*
dhdb_to_json(dhdb_t *s)
{
	static char out[64000];

	assert(s);
	out[0] = 0;
	_serialize(s, out, 0, false);
	strcat(out, "\n");
	return out;	
}

const char*
dhdb_to_json_pretty(dhdb_t *s)
{
	static char out[64000];

	assert(s);
	out[0] = 0;
	_serialize(s, out, 0, true);
	return out;	
}
