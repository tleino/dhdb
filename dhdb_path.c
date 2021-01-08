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

#include "dhdb_path.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#define __USE_BSD
#define __USE_POSIX
#include <string.h>


#define MAX_HISTORY_LEVEL 9

static dhdb_t*		prev_search_root;
static int		prev_at_index[MAX_HISTORY_LEVEL];
static dhdb_t*		prev_at[MAX_HISTORY_LEVEL];
static dhdb_t*		prev_by[MAX_HISTORY_LEVEL];

dhdb_t* dhdb_path (dhdb_t *s, const char *path, dhdb_t *current)
{
	assert(s);
	assert(path);

	if (current == NULL) current = s;

	char *non_const_path = strdup(path);
	char *str = non_const_path, *saveptr, *token;
	int level = 0;
	while ((token = strtok_r(str, "/", &saveptr))) {
		str = NULL;

		if (token[0] >= '0' && token[0] <= '9') {
			int index = atoi(token);
			if (prev_search_root == s && prev_at_index[level] == index) {
				current = prev_at[level];
			}
			else {
				current = dhdb_at(current, atoi(token));
				prev_at[level] = current;
				prev_at_index[level] = index;
			}
			if (!current) {
				printf("Path search '%s' failed at level %d\n", path, level);
				return NULL;
			}
		}
		else {
			if (prev_search_root == s && prev_by[level] && !strcmp(dhdb_name(prev_by[level]), token)) {
				current = prev_by[level];
			}
			else {
				current = dhdb_by(current, token);
				prev_by[level] = current;
			}
			if (!current) {
				printf("Path search '%s' failed at level %d\n", path, level);
				return NULL;
			}
		}

		level++;
	}
	prev_search_root = s;
	free(non_const_path);

	return current;
}
