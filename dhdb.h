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

#ifndef __dhdb_H__
#define __dhdb_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct dhdbValue dhdb_t;

#define DHDB_VALUE_UNDEFINED		0
#define DHDB_VALUE_OBJECT		1
#define DHDB_VALUE_ARRAY		2
#define DHDB_VALUE_NUMBER		3
#define DHDB_VALUE_STRING		4
#define DHDB_VALUE_BOOL			5
#define DHDB_VALUE_NULL			6
#define NUM_DHDB_VALUE			7

/* Creation and destroying */
dhdb_t*		dhdb_create	();
void		dhdb_free	(dhdb_t *s);

/* Getting type and length of JSON types */
int		dhdb_len		(dhdb_t *s);
uint8_t		dhdb_type		(dhdb_t *s);
const char*	dhdb_name		(dhdb_t *s);
bool		dhdb_is_container	(dhdb_t *s);
uint32_t	dhdb_size		(dhdb_t *s);

/* Generic value search */
dhdb_t*		dhdb_by		(dhdb_t *s, const char *name);
dhdb_t*		dhdb_at		(dhdb_t *s, int idx);
dhdb_t*		dhdb_first	(dhdb_t *s);
dhdb_t*		dhdb_last	(dhdb_t *s);
dhdb_t*		dhdb_next	(dhdb_t *s);
dhdb_t*		dhdb_prev	(dhdb_t *s);
dhdb_t*		dhdb_parent	(dhdb_t *s);

/* Conversion from JSON type to C type with search helpers */
double		dhdb_num	(dhdb_t *s);
double		dhdb_num_by	(dhdb_t *s, const char *name);
double		dhdb_num_at	(dhdb_t *s, int idx);

const char*	dhdb_str	(dhdb_t *s);
const char*	dhdb_str_by	(dhdb_t *s, const char *name);
const char*	dhdb_str_at	(dhdb_t *s, int idx);

/* Setting string and number values */
void		dhdb_set_str	(dhdb_t *s, const char *str);
void		dhdb_set_num	(dhdb_t *s, double num);
void		dhdb_set_bool	(dhdb_t *s, bool val);
void		dhdb_set_null	(dhdb_t *s);

/* Setting object values */
void		dhdb_set_obj		(dhdb_t *s, const char *field, dhdb_t *val);
void		dhdb_set_obj_str	(dhdb_t *s, const char *field, const char *str);
void		dhdb_set_obj_num	(dhdb_t *s, const char *field, double num);

/* Creating an array or adding to an array */
void		dhdb_add_str		(dhdb_t *s, const char *str);
void		dhdb_add_num		(dhdb_t *s, double num);
void		dhdb_add		(dhdb_t *s, dhdb_t *v);
dhdb_t*		dhdb_set_array		(dhdb_t *s); /* Necessary only for creating an empty array */

/* Changing item type (needed only by editors such as for changing object array to plain array) */
//void		dhdb_set_type	(dhdb_t *s, uint8_t type);

#endif
