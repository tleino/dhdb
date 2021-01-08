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
#include "dhdb_private.h"

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdint.h>
#include <ctype.h>
#include <stdbool.h>
#define __USE_BSD
#include <string.h>

static dhdb_t* _add_to_array(dhdb_t *, dhdb_t *);
static dhdb_t* _add_to_object(dhdb_t *, const char *, bool, dhdb_t *);
static void _free(dhdb_t *, int);

uint8_t
dhdb_type(dhdb_t *s)
{
	if (s == NULL) return DHDB_VALUE_UNDEFINED;
	return s->type;
}

int
dhdb_len(dhdb_t *s)
{
	if (s == NULL) return 0;
	return s->array_len;
}

bool
dhdb_is_container(dhdb_t *s)
{
	if (s == NULL) return false;
	if (s->type == DHDB_VALUE_OBJECT) return true;
	if (s->type == DHDB_VALUE_ARRAY) return true;
	return false;
}

uint32_t
dhdb_size(dhdb_t *s)
{
	int i, bytes;
	dhdb_t *n;

	bytes = sizeof(dhdb_t);
	if (s->name) bytes += strlen(s->name) + 1;
	if (s->str) bytes += strlen(s->str) + 1;

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

	n = s->first_child;
	while (n) {
		next = n->next;
		dhdb_free(n);
		n = next;
	}

	free(s);
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

void
dhdb_set_str(dhdb_t *s, const char *str)
{
	assert(s);
	assert(str);

	if (!_set_type(s, DHDB_VALUE_STRING))
		return;
	s->str = strdup(str);
}

static dhdb_t*
_find_object(dhdb_t *s, const char *field)
{
	dhdb_t *n;

	assert(s);
	assert(s->type == DHDB_VALUE_OBJECT);

	n = s->next;
	while (n) {
		if (strcmp(n->name, field) == 0)
			return n;
		n = n->next;
	}
	return NULL;
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
	else return s->next;
}

dhdb_t*
dhdb_prev(dhdb_t *s)
{
	if (!s)
		return NULL;
	else return s->prev;
}

dhdb_t*
dhdb_parent(dhdb_t *s)
{
	if (!s)
		return NULL;
	else return s->parent;
}

const char*
dhdb_name(dhdb_t *s)
{
	if (!s)
		return NULL;
	else return s->name;
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
_add_to_array(dhdb_t *s, dhdb_t *val)
{
	if (s->type != DHDB_VALUE_OBJECT && !_set_type(s, DHDB_VALUE_ARRAY))
		return NULL;

	if (val == NULL)
		val = dhdb_create();

	val->parent = s;
	s->array_len++;

	/* Add to end of the list */
	val->prev = s->last_child;
	s->last_child = val;
	if (s->first_child == NULL)
		s->first_child = val;
	if (val->prev)
		val->prev->next = val;

	return val;
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

	val = _add_to_array(s, val);

	return val;
}

void
dhdb_add_str(dhdb_t *s, const char *str)
{
	dhdb_t *n;

	assert(s);
	assert(str);

	n = _add_to_array(s, NULL);
	if (!n)
		return;

	dhdb_set_str(n, str);
}

void
dhdb_add_num(dhdb_t *s, double num)
{
	dhdb_t *n;

	assert(s);

	n = _add_to_array(s, NULL);
	if (!n)
		return;

	dhdb_set_num(n, num);
}

void
dhdb_add (dhdb_t *s, dhdb_t *v)
{
	dhdb_t *n;

	n = _add_to_array(s, v);
	if (!n)
		return;
}

void
dhdb_set_bool (dhdb_t *s, bool val)
{
	assert(s);

	if (!_set_type(s, DHDB_VALUE_BOOL))
		return;
	s->num = (double) val;
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
		if (strcmp(n->name, name) == 0)
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
		if (i == idx)
			return n;
		n = n->next;
		i++;
	}

	return NULL;
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

const char*
dhdb_str(dhdb_t *s)
{
	assert(s);

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
dhdb_create() {
	return calloc(1, sizeof(dhdb_t));
}
