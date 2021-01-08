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

#include "dhdb.h"
#include "dhdb_dump.h"
#include "dhdb_private.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdarg.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#ifndef __USE_XOPEN2K8
#define __USE_XOPEN2K8
#endif
#include <string.h>
#include <inttypes.h>

#define MAX_VA_STR_LEN 1024 * 8
#define VA_START va_list args; va_start(args, fmt)

static dhdb_t* _add_to_array(dhdb_t *, dhdb_t *, dhdb_t *);
static dhdb_t* _add_to_object(dhdb_t *, const char *, bool, dhdb_t *);
static void _free(dhdb_t *, int);
static const char* _va_str(const char *, va_list);
static bool _set_type(dhdb_t *, uint8_t);
static void _remove_item(dhdb_t *, dhdb_t *);
static dhdb_t* _find_object(dhdb_t *, const char *);

uint8_t
dhdb_type(dhdb_t *s)
{
	if (s == NULL)
		return DHDB_VALUE_UNDEFINED;

	return s->type;
}

int
dhdb_len(dhdb_t *s)
{
	if (s == NULL)
		return 0;

	return s->array_len;
}

bool
dhdb_is_container(dhdb_t *s)
{
	if (s == NULL)
		return false;
	if (s->type == DHDB_VALUE_OBJECT)
		return true;
	if (s->type == DHDB_VALUE_ARRAY)
		return true;
	return false;
}

uint32_t
dhdb_size(dhdb_t *s)
{
	int bytes;
	dhdb_t *n;
	int i;

	bytes = sizeof(dhdb_t);
	if (s->name)
		bytes += strlen(s->name) + 1;
	if (s->str)
		bytes += strlen(s->str) + 1;

	n = s->first_child;
	i = 0;
	while (n) {
		bytes += dhdb_size(n);
		n = n->next;
	}
	return bytes;
}

void
dhdb_free(dhdb_t *s)
{
	dhdb_t *n, *next;

	if (s == NULL)
		return;

	if (s->str)
		free(s->str);
	if (s->name)
		free(s->name);
	if (s->parent)
		_remove_item(s->parent, s);

	n = s->first_child;
	while (n) {
		next = n->next;
		dhdb_free(n);
		n = next;
	}

	free(s);
}

dhdb_t*
dhdb_detach(dhdb_t *s)
{
	if (s->parent)
		_remove_item(s->parent, s);
	s->parent = NULL;
	if (s->name) {
		free(s->name);
		s->name = NULL;
	}
	s->next = NULL;
	s->prev = NULL;
	return s;
}

void
dhdb_set_str_len(dhdb_t *s, int len, const char *str)
{
	if (!_set_type(s, DHDB_VALUE_STRING))
		return;

	s->str = strndup(str, len);
}

void
dhdb_set_str(dhdb_t *s, const char *str)
{
	assert(s);
	assert(str);

	if (!_set_type(s, DHDB_VALUE_STRING))
		return;

	s->str = strdup(str);
}

void
dhdb_set_str_va(dhdb_t *s, const char *fmt, ...)
{
	VA_START;
	return dhdb_set_str(s, _va_str(fmt, args));
}

void
dhdb_set_str_add(dhdb_t *s, const char *str)
{
	if (!s->str)
		return dhdb_set_str(s, str);

	s->str = realloc(s->str, strlen(s->str) + strlen(str) + 1);
	strcat(s->str, str);
}

void
dhdb_set_str_add_va(dhdb_t *s, const char *fmt, ...)
{
	VA_START;
	return dhdb_set_str_add(s, _va_str(fmt, args));
}

dhdb_t*
dhdb_first(dhdb_t *s)
{
	if (!s)
		return NULL;
	else
		return s->first_child;
}

dhdb_t*
dhdb_last(dhdb_t *s)
{
	if (!s)
		return NULL;
	else
		return s->last_child;
}

dhdb_t*
dhdb_next(dhdb_t *s)
{
	if (!s)
		return NULL;
	else
		return s->next;
}

dhdb_t*
dhdb_prev(dhdb_t *s)
{
	if (!s)
		return NULL;
	else
		return s->prev;
}

dhdb_t*
dhdb_parent(dhdb_t *s)
{
	if (!s)
		return NULL;
	else
		return s->parent;
}

const char*
dhdb_name(dhdb_t *s)
{
	if (!s)
		return NULL;
	else
		return s->name;
}

void
dhdb_set_obj(dhdb_t *s, const char *field, dhdb_t *v)
{
	(void) _add_to_object(s, field, true, v);
}

void
dhdb_set_obj_str(dhdb_t *s, const char *field, const char *str)
{
	dhdb_t *o;

	o = _add_to_object(s, field, true, NULL);
	dhdb_set_str(o, str);
}

void
dhdb_set_obj_num(dhdb_t *s, const char *field, double num)
{
	dhdb_t *o;

	o = _add_to_object(s, field, true, NULL);
	dhdb_set_num(o, num);
}

dhdb_t*
dhdb_set_array(dhdb_t *s)
{
	assert(s);

	if (!_set_type(s, DHDB_VALUE_UNDEFINED))
		return NULL;
	if (!_set_type(s, DHDB_VALUE_ARRAY))
		return NULL;

	return s;
}

static dhdb_t*
_add_to_array(dhdb_t *s, dhdb_t *val, dhdb_t *after)
{
	dhdb_t *next;

	if (s->type != DHDB_VALUE_OBJECT && !_set_type(s, DHDB_VALUE_ARRAY))
		return NULL;

	if (val == NULL)
		val = dhdb_create();

	assert(val->parent == NULL);
	assert(val->next == NULL);
	assert(val->prev == NULL);

	val->parent = s;

	/* Add to end of the list, or after 'after' item */
	if (after == 0)
		after = s->last_child;
	if (!s->first_child)
		s->first_child = val;
	if (!s->last_child)
		s->last_child = val;
	else {
		next = after->next;
		after->next = val;
		val->prev = after;
		if (after == s->last_child)
			s->last_child = val;
		val->next = next;
		val->index = after->index + 1;
	}
	
	s->array_len++;
	return val;
}

static dhdb_t*
_find_object(dhdb_t *s, const char *field)
{
	dhdb_t *n;

	assert(s);
	assert(s->type == DHDB_VALUE_OBJECT);

	n = s->next;
	while (n) {
		if (!strcasecmp(n->name, field))
			return n;

		n = n->next;
	}
	return NULL;
}

static dhdb_t*
_add_to_object(dhdb_t *s, const char *field, bool prevent_duplicates,
    dhdb_t *val)
{
	if (!_set_type(s, DHDB_VALUE_OBJECT))
		return NULL;

	if (prevent_duplicates && dhdb_by(s, field))
		return dhdb_by(s, field);

	if (val == NULL)
		val = dhdb_create();

	if (val->name)
		free(val->name);
	val->name = strdup(field);

	val = _add_to_array(s, val, NULL);

	return val;
}

void
dhdb_add_str(dhdb_t *s, const char *str)
{
	dhdb_t *n;

	assert(s);
	assert(str);

	n = _add_to_array(s, NULL, NULL);
	if (!n)
		return;

	dhdb_set_str(n, str);
}

void
dhdb_add_num(dhdb_t *s, double num)
{
	dhdb_t *n;

	assert(s);

	n = _add_to_array(s, NULL, NULL);
	if (!n)
		return;

	dhdb_set_num(n, num);
}

void
dhdb_add(dhdb_t *s, dhdb_t *v)
{
	(void) _add_to_array(s, v, NULL);
}

void
dhdb_insert(dhdb_t *s, dhdb_t *after, dhdb_t *v)
{
	(void) _add_to_array(s, v, after);
}

void
dhdb_set_from(dhdb_t *s, dhdb_t *v)
{
	uint8_t type;

	type = dhdb_type(v);
	if (!_set_type(s, type))
		return;

	switch (type) {
	case DHDB_VALUE_BOOL:
	case DHDB_VALUE_NUMBER:
		s->num = v->num;
		break;
	case DHDB_VALUE_STRING:
		s->str = strdup(v->str);
		break;
	}
}

void
dhdb_set_bool(dhdb_t *s, bool val)
{
	assert(s);
	if (!_set_type(s, DHDB_VALUE_BOOL))
		return;
	s->num = (double) val;
}

void
dhdb_set_bool_toggle(dhdb_t *s)
{
	return dhdb_set_bool(s, s->num ? false : true);
}

void
dhdb_set_null (dhdb_t *s)
{
	assert(s);
	if (!_set_type(s, DHDB_VALUE_NULL))
		return;
	s->num = 0;
}

void
dhdb_set_num(dhdb_t *s, double num)
{
	assert(s);
	if (!_set_type(s, DHDB_VALUE_NUMBER))
		return;
	s->num = num;
}

void
dhdb_set_num_dec(dhdb_t *s)
{
	return dhdb_set_num(s, (s->num) - 1);
}

void
dhdb_set_num_inc(dhdb_t *s)
{
	return dhdb_set_num(s, (s->num) + 1);
}

void
dhdb_set_num_add(dhdb_t *s, double add_num)
{
	return dhdb_set_num(s, (s->num) + add_num);
}

void
dhdb_set_num_sub(dhdb_t *s, double add_num)
{
	return dhdb_set_num(s, (s->num) - add_num);
}

void
dhdb_set_num_div(dhdb_t *s, double div_num)
{
	return dhdb_set_num(s, (s->num) / div_num);
}

void
dhdb_set_num_mul(dhdb_t *s, double mul_num)
{
	return dhdb_set_num(s, (s->num) * mul_num);
}

void
dhdb_set_num_from (dhdb_t *s, dhdb_t *v)
{
	if (v->type == DHDB_VALUE_STRING)
		return dhdb_set_num(s, atof(s->str));

	return dhdb_set_num(s, v->num);
}

void
dhdb_set_str_from(dhdb_t *s, dhdb_t *v)
{
	static char buf[MAX_VA_STR_LEN];

	if (v->type == DHDB_VALUE_BOOL)
		return dhdb_set_str(s, v->num ? "true" : "false");
	if (v->type == DHDB_VALUE_NULL)
		return dhdb_set_str(s, "null");
	if (v->type == DHDB_VALUE_STRING)
		return dhdb_set_str(s, v->str);
	if (v->type == DHDB_VALUE_NUMBER) {
		snprintf(buf, sizeof(buf), "%f", v->num);
		return dhdb_set_str(s, buf);
	}
	return dhdb_set_str(s, "");
}

void
dhdb_set_bool_from(dhdb_t *s, dhdb_t *v)
{
	return dhdb_set_bool(s, v->num);
}

dhdb_t*
dhdb_by(dhdb_t *s, const char *name)
{
	dhdb_t *n;

	assert(s);
	assert(name);

	if (s->type != DHDB_VALUE_OBJECT)
		return NULL;

	n = s->first_child;
	while (n) {
		if (!strcasecmp(n->name, name))
			return n;

		n = n->next;
	}

	return NULL;
}

dhdb_t*
dhdb_at(dhdb_t *s, int idx)
{
	dhdb_t *n;
	int i;

	assert(s);
	assert(idx >= 0);

	n = s->first_child;
	i = 0;
	while (n) {
		if (i == idx) return n;
		n = n->next; i++;
	}
	dhdb_dump(s);
	fprintf(stderr, "Index out of bounds idx=%d\n", idx);

	return NULL;
}

uint32_t
dhdb_index(dhdb_t *s)
{
	assert(s);
	return s->index;
}

double
dhdb_num_by(dhdb_t *s, const char *name)
{
	dhdb_t *v;

	v = dhdb_by(s, name);
	if (v)
		return v->num;

	return 0;
}

double
dhdb_num_at(dhdb_t *s, int idx)
{
	dhdb_t *v;

	v = dhdb_at(s, idx);
	if (v)
		return v->num;

	return 0;
}

bool
dhdb_bool_by(dhdb_t *s, const char *name)
{
	return (bool) dhdb_num_by(s, name);
}

bool
dhdb_bool_at(dhdb_t *s, int idx)
{
	return (bool) dhdb_num_at(s, idx);
}

bool
dhdb_bool(dhdb_t *s)
{
	return (bool) s->num;
}

const char*
dhdb_str(dhdb_t *s)
{
	return s->str;
}

const char*
dhdb_str_at(dhdb_t *s, int idx)
{
	dhdb_t *v;

	v = dhdb_at(s, idx);
	if (v)
		return v->str;

	return 0;
}

double
dhdb_num(dhdb_t *s)
{
	return s->num;
}

const char*
dhdb_str_by(dhdb_t *s, const char *name)
{
	dhdb_t *v;

	v = dhdb_by(s, name);
	if (v)
		return v->str;

	return NULL;
}

dhdb_t*
dhdb_create()
{
	return calloc(1, sizeof(dhdb_t));
}

dhdb_t*
dhdb_create_str(const char *str)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_str(s, str);
	return s;
}

dhdb_t*
dhdb_create_str_va(const char *fmt, ...)
{
	VA_START;
	return dhdb_create_str(_va_str(fmt, args));
}

dhdb_t*
dhdb_create_str_len(int len, const char *str)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_str_len(s, len, str);
	return s;
}

dhdb_t*
dhdb_create_str_from(dhdb_t *v)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_str_from(s, v);
	return s;
}

dhdb_t*
dhdb_create_num(double num)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_num(s, num);
	return s;
}

dhdb_t*
dhdb_create_num_from(dhdb_t *v)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_num_from(s, v);
	return s;
}

dhdb_t*
dhdb_create_bool(bool flag)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_bool(s, flag);
	return s;
}

dhdb_t*
dhdb_create_bool_from(dhdb_t *v)
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_bool_from(s, v);
	return s;
}

dhdb_t*
dhdb_create_null()
{
	dhdb_t *s;

	s = dhdb_create();
	dhdb_set_null(s);
	return s;
}

static void
_remove_item(dhdb_t *s, dhdb_t *item)
{
	dhdb_t *next, *prev;

	next = item->next;
	prev = item->prev;

	if (prev)
		prev->next = next;
	if (next)
		next->prev = prev;
	if (item == s->first_child)
		s->first_child = next;
	if (item == s->last_child)
		s->last_child = prev;

	s->array_len--;
}

static bool
_set_type(dhdb_t *s, uint8_t type)
{
	dhdb_t *n, *next;
	int i;
	char buf[64];

	if (s->type == DHDB_VALUE_STRING && s->str) {
		free(s->str);
		s->str = 0;
	}
	if (s->type == DHDB_VALUE_OBJECT && type == DHDB_VALUE_ARRAY) {
		n = dhdb_first(s);
		while (n) {
			if (n->name) {
				free(n->name);
				n->name = 0;
			}
			n = dhdb_next(n);
		}
	}
	if (s->type == DHDB_VALUE_ARRAY && type == DHDB_VALUE_OBJECT) {
		n = dhdb_first(s);
		i = 0;
		while (n) {
			snprintf(buf, sizeof(buf), "%d", i);
			n->name = strdup(buf);
			i++;
			n = dhdb_next(n);
		}
	}
	if ((s->type == DHDB_VALUE_ARRAY || s->type == DHDB_VALUE_OBJECT) &&
	    (type != DHDB_VALUE_ARRAY && type != DHDB_VALUE_OBJECT)) {
		n = dhdb_first(s);
		while (n) {
			next = dhdb_next(n);
			dhdb_free(n);
			n = next;
		}
		s->first_child = NULL;
		s->last_child = NULL;
		s->array_len = 0;
	}

	s->type = type;
	return true;
}

static const char*
_va_str(const char *fmt, va_list args)
{
	static char str[MAX_VA_STR_LEN];

	vsnprintf(str, sizeof(str), fmt, args);
	va_end(args);
	return str;
}
