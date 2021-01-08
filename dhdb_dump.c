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
#include <string.h>

static int _dump(dhdb_t *, int, int);

void
dhdb_dump(dhdb_t *s)
{
	int bytes;

	bytes = _dump(s, 0, 0);
	printf("Bytes allocated: %d\n", bytes);
}

static int
_dump(dhdb_t *s, int level, int index)
{
	int i, bytes;
	dhdb_t *n;
	static const char *dhdbValueTxt[] = {
		"undefined", "object", "array", "number", "string",
		"bool", "null" 
	};

	bytes = sizeof(dhdb_t);
	if (s->name) bytes += strlen(s->name) + 1;
	if (s->str) bytes += strlen(s->str) + 1;

	for (int i = 0; i < level; i++)
		printf("\t");

	if (index)
		printf("%2d ", index);
	else
		printf("** ");

	printf("%s ", dhdbValueTxt[s->type]);

	if (s->name)
		printf("%s ", s->name);
	
	if (s->type == DHDB_VALUE_NUMBER)
		printf("%lf ", s->num);
	else if (s->type == DHDB_VALUE_STRING)
		printf("\"%s\" ", s->str);
	else if (s->type == DHDB_VALUE_BOOL)
		printf("%s ", s->num ? "true" : "false");
	else if (s->type == DHDB_VALUE_NULL)
		printf("null ");

	if (s->type == DHDB_VALUE_ARRAY || s->type == DHDB_VALUE_OBJECT)
		printf("[len=%d] [address=%lx] ", s->array_len, (uintptr_t) s);

	if (s->parent)
		printf("[parent=%lx] ", (uintptr_t) s->parent);

	printf("\n");

	n = s->first_child;
	i = 0;
	while (n) {
		bytes += _dump(n, level + 1, ++i);
		n = n->next;
	}

	return bytes;
}
