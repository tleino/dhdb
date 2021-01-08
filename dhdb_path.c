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
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef __USE_BSD
#define __USE_BSD
#endif
#ifndef __USE_XOPEN2K8
#define __USE_XOPEN2K8
#endif
#include <string.h>

#define MAX_VA_PATH_LEN		256 // Maximum length for varargs created path names
#define MAX_PATH_NAME_LEN	256 // Maximum length for path names created by dhdb_path_name
#define MAX_PICK_DEPTH		32  // Maximum number of root elements to remember when picking a path

static char _separator = '/';
static const uint32_t _token_alloc_block_size = 8;

typedef struct tokenized_path
{
	char **path;
	int max_level;
} path_token_t;

struct path_iter
{
	path_token_t	*tokens;
	dhdb_t		**root_arr;
	dhdb_t		*leaf;
	int		level;
};

static path_token_t* _tokenize_path(const char *path)
{
	struct tokenized_path *tp = malloc(sizeof(struct tokenized_path));
	int max_alloc = _token_alloc_block_size;
	tp->path = malloc(max_alloc * sizeof(void *));
	tp->max_level = 0;

	int i, sz = strlen(path), tokenBegin = 0;
	int level = 0;
	for (i = 0; i < sz; i++) {
		char *token = 0;
		bool is_last = (i == sz - 1);
		if (path[i] == _separator || is_last) {
			if (is_last) {
				token = strdup(&path[tokenBegin]);
			} else {
				token = strndup(&path[tokenBegin], i - tokenBegin);
				tokenBegin = i + 1;
			}
			tp->path[tp->max_level++] = token;
			if (tp->max_level >= max_alloc) {
				max_alloc *= 2;
				tp->path = realloc(tp->path, max_alloc * sizeof(void *));	
			}
		}
	}

	return tp;
}

static void _free_tokenized_path(path_token_t *s)
{
	for (int i = 0; i < s->max_level; i++) free(s->path[i]);
	free(s->path);
	free(s);
}

static const char* _token_for_level(path_token_t *s, int level)
{
	if (level >= s->max_level)
		level = s->max_level - 1;
	if (level < 0)
		level = 0;

	return s->path[level];
}

static const char* _va_path(const char *fmt, va_list args)
{
	static char path[MAX_VA_PATH_LEN];
	vsnprintf(path, sizeof(path), fmt, args);
	va_end(args);
	return path;
}

static dhdb_t* _path (dhdb_t *s, const char *path)
{
	dhdb_t *leaf = s;
	int i, sz = strlen(path), tokenBegin = 0;
	for (i = 0; i < sz; i++) {
		char *token = 0;
		dhdb_t *o;
		if (path[i] == _separator || i == sz - 1) {
			if (i == sz - 1) {
				token = strndup(&path[tokenBegin], i - tokenBegin + 1);
			} else {
				token = strndup(&path[tokenBegin], i - tokenBegin);
				tokenBegin = i + 1;
			}
			leaf = dhdb_by(leaf, token);
		}
		free(token);
		if (leaf == NULL) break;
	}

	return leaf;	
}

dhdb_t*	dhdb_path (dhdb_t *s, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return _path(s, _va_path(fmt, args));
}

dhdb_t* dhdb_path_first (dhdb_t *s, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return dhdb_first(_path(s, _va_path(fmt, args)));
}

const char* dhdb_path_str (dhdb_t *s, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return dhdb_str(_path(s, _va_path(fmt, args)));
}

double dhdb_path_num (dhdb_t *s, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return dhdb_num(_path(s, _va_path(fmt, args)));
}

bool dhdb_path_bool (dhdb_t *s, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return dhdb_bool(_path(s, _va_path(fmt, args)));
}

static dhdb_t* _go_back_try_again (dhdb_path_iter_t *iter)
{
	iter->level--;
	if (iter->level < 0) {
		return NULL;
	}
	const char *token = _token_for_level(iter->tokens, iter->level - 1);
	if (*token == '*') {
		iter->leaf = dhdb_next(iter->root_arr[iter->level]);
		iter->root_arr[iter->level] = iter->leaf;
		if (iter->leaf == NULL) {
			return _go_back_try_again(iter);
		}
	} else {
		return _go_back_try_again(iter);
	}
	return iter->leaf;
}

static void _dump_roots(dhdb_path_iter_t *iter)
{
	for (int i = 0; i < 10; i++) {
		dhdb_t *root = iter->root_arr[i];
		if (root)
			printf("Root %d: %s %s\n", i, dhdb_name(root), iter->level == i ? "CURRENT" : "");
	}
}

static dhdb_t* _search_until_leaf (dhdb_path_iter_t *iter)
{
	while (dhdb_is_container(iter->leaf)) {
		dhdb_t *root = iter->root_arr[iter->level];
		const char *token = _token_for_level(iter->tokens, iter->level);
		if (*token == '*') {
			iter->leaf = dhdb_first(root);
		} else {
			iter->leaf = dhdb_by(root, token);
		}
		iter->level++;
		iter->root_arr[iter->level] = iter->leaf;
	}

	// If we got leaf but we didn't actually check tokens until token max
	if (iter->leaf && iter->level < iter->tokens->max_level) {
		iter->leaf = _go_back_try_again(iter);
		if (iter->leaf)
			iter->leaf = _search_until_leaf(iter);
	}

	return iter->leaf;
}

static dhdb_t* _pick_first_matching_leaf (dhdb_path_iter_t *iter)
{
	if (iter->leaf) {
		const char *token = _token_for_level(iter->tokens, iter->level);
		if (*token == '*')
			iter->leaf = dhdb_next(iter->leaf);
		if (iter->leaf == NULL || *token != '*') {
			iter->leaf = _go_back_try_again(iter);
			iter->leaf = _search_until_leaf(iter);
		}
	} else {
		dhdb_t *root = iter->root_arr[iter->level];
		const char *token = _token_for_level(iter->tokens, iter->level);
		if (*token == '*') {
			iter->leaf = dhdb_first(root);
		} else {
			iter->leaf = dhdb_by(root, token);
		}		
		iter->level++;
		iter->root_arr[iter->level] = iter->leaf;
		iter->leaf = _search_until_leaf(iter);
	}
	return iter->leaf;
}

static dhdb_t* _find_leaf (dhdb_path_iter_t *iter)
{
	return _pick_first_matching_leaf(iter);
}

static void _path_pick_dump_fd(dhdb_t *root, int fd, const char *path)
{
	path_token_t *tokens = _tokenize_path(path);
	dhdb_t *leaf = 0;
	int level = 0;

	// TODO: fd support
	//while (leaf = _find_leaf(&root, leaf, &level, tokens)) {
	//	dhdb_dump(leaf);
	//}
	_free_tokenized_path(tokens);
}

void dhdb_path_pick_dump(dhdb_t *root, const char *fmt, ...)
{	
	va_list args; va_start(args, fmt);
	return _path_pick_dump_fd(root, fileno(stdout), _va_path(fmt, args));
}

static dhdb_t* _path_pick (dhdb_path_iter_t *iter)
{
	iter->leaf = _find_leaf(iter);
	return iter->leaf;
}

dhdb_t* dhdb_path_pick_first (dhdb_t *s, dhdb_path_iter_t **iter, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	const char *path = _va_path(fmt, args);
	*iter = malloc(sizeof(struct path_iter));
	(*iter)->tokens = _tokenize_path(path);
	(*iter)->leaf = NULL;
	(*iter)->level = 0;
	(*iter)->root_arr = (dhdb_t **) calloc(MAX_PICK_DEPTH, sizeof(dhdb_t *));
	(*iter)->root_arr[0] = s;

	return _path_pick(*iter);
}

dhdb_t* dhdb_path_pick_next (dhdb_path_iter_t *iter)
{
	return _path_pick(iter);
}

dhdb_t* dhdb_path_pick_free (dhdb_path_iter_t **iter)
{
	_free_tokenized_path((*iter)->tokens);
	free((*iter)->root_arr);
	free(*iter);
	*iter = 0;
	return NULL;
}

void dhdb_path_internal_set_separator(char separator)
{
	_separator = separator;
}

const char* dhdb_path_name(dhdb_t *s)
{
	static char buf[MAX_PATH_NAME_LEN];
	memset(&buf, 0, sizeof(buf));
	int len = sizeof(buf) - 1;
	dhdb_t *parent = s;
	// Go up from the leaf up to the upmost root and while doing it,
	// insert names to buf but write to the end of the buf, not beginning...
	for (; parent; parent = dhdb_parent(parent)) {
		const char *name = dhdb_name(parent);
		if (!name) break;
		int sz = strlen(name);
		len -= sz;
		if (len < 0)
			len = 0;
		strncpy(&buf[len], name, sz);
		buf[len + sz] = '.';
		len--;
	}
	buf[sizeof(buf) - 1] = 0;
	return &buf[len + 1];
}

static dhdb_t* _set_path (dhdb_t *s, dhdb_t *v, const char *path)
{
	path_token_t *tokens = _tokenize_path(path);
	dhdb_t *node;

	for (int i = 0; i < tokens->max_level; i++) {
		const char *token = tokens->path[i];
		node = dhdb_by(s, token);
		if (node == NULL) {
			node = dhdb_create();
			dhdb_set_obj(s, token, node);
		}
		s = node;
		if (i == tokens->max_level - 1) {
			dhdb_set_from(node, v);
			dhdb_free(v);
		}
	}

	_free_tokenized_path(tokens);
	return node;
}

dhdb_t* dhdb_path_set (dhdb_t *s, dhdb_t *v, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return _set_path(s, v, _va_path(fmt, args));
}

dhdb_t* dhdb_path_set_str (dhdb_t *s, const char *str, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return _set_path(s, dhdb_create_str(str), _va_path(fmt, args));
}

dhdb_t* dhdb_path_set_num (dhdb_t *s, double num, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return _set_path(s, dhdb_create_num(num), _va_path(fmt, args));
}

dhdb_t* dhdb_path_set_bool (dhdb_t *s, bool flag, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return _set_path(s, dhdb_create_bool(flag), _va_path(fmt, args));
}

dhdb_t* dhdb_path_set_null (dhdb_t *s, const char *fmt, ...)
{
	va_list args; va_start(args, fmt);
	return _set_path(s, dhdb_create_null(), _va_path(fmt, args));
}
