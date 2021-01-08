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

#ifndef __DHDB_PATH_H__
#define __DHDB_PATH_H__

#include "dhdb.h"

typedef struct path_iter dhdb_path_iter_t;

/* Configures the separator used by the dhdb path, default is '/' */
void	dhdb_path_internal_set_separator	(char separator);

/* Set element to a path, automatically creates required nodes if they are missing */
dhdb_t*	dhdb_path_set		(dhdb_t *s, dhdb_t *v, const char *path, ...);
dhdb_t*	dhdb_path_set_str	(dhdb_t *s, const char *str, const char *path, ...);
dhdb_t*	dhdb_path_set_num	(dhdb_t *s, double num, const char *path, ...);
dhdb_t*	dhdb_path_set_bool	(dhdb_t *s, bool flag, const char *path, ...);
dhdb_t*	dhdb_path_set_null	(dhdb_t *s, const char *path, ...);

/* Returns path name of any node */
const char*	dhdb_path_name	(dhdb_t *s);

/*
Simple path API which returns dhdb_t* pointer for an exact match of the path
- Allows iteration with dhdb_next, dhdb_prev, dhdb_parent, dhdb_first, etc.
- Can return non-leaf nodes
- Performs better than dhdb_path_pick
*/
dhdb_t*		dhdb_path	(dhdb_t *s, const char *path, ...);
dhdb_t*		dhdb_path_first	(dhdb_t *s, const char *path, ...);
const char*	dhdb_path_str	(dhdb_t *s, const char *path, ...);
double		dhdb_path_num	(dhdb_t *s, const char *path, ...);
bool		dhdb_path_bool	(dhdb_t *s, const char *path, ...);

/*
Advanced path API that flattens hierarchy and returns only leaf nodes
- Provides its own iterator
- Allows search with wildcards
- Slower than dhdb_path
- Allows construction of path from varargs
- TODO: Search of arrays with ranges
*/
dhdb_t*	dhdb_path_pick_first_only	(dhdb_t *s, const char *path, ...);
dhdb_t*	dhdb_path_pick_first		(dhdb_t *s, dhdb_path_iter_t **iter, const char *path, ...);
dhdb_t*	dhdb_path_pick_next		(dhdb_path_iter_t *iter);
dhdb_t*	dhdb_path_pick_free		(dhdb_path_iter_t **iter);

/* For debugging: dumps of leaf nodes of a path to stdout or to fd */
void	dhdb_path_pick_dump	(dhdb_t *s, const char *path, ...);
void	dhdb_path_pick_dump_fd	(dhdb_t *s, int fd, const char *path, ...);

#endif
