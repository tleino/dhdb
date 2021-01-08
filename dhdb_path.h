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

// Array accessing
// "0"		Picks item at index 0
// "1"		Picks item at index 1
// "1-2"	Picks items at indexes 1 - 2
// "0-2"	Picks items at indexes 0 - 2
// "2-<"	Picks items at indexes 2 - end
// "<"		Picks the last item
// "<<"		Creates new item after the last item
//
// Object accessing
// "field"	Pick field with name 'field'
// "field*"	Picks field whose name begins with string 'field'
// "*field"	Picks field whose name ends with string 'field'
//
// Type accessing
// =number()	Only pick values of type number
// =string()	Only pick values of type string
//
// Value accessing
// =number(34);	Only pick values of type number that has number 34
// =string("Hello"); Only pick values of type string that has value 'hello'
// =number(<34); Only pick values of type number that has number that is smaller than 34
// =number(>34); Only pick values of type number that has number that is smaller than 34
//
// Type and value accessing, use dhdb_filter
//
// dhdb_path(s, "0-2/field/subfield");
// dhdb_path(s, "%d-%d/field/subfield", min, max);
// dhdb_path(s, "<</field/subfield");
//
// dhdb_filter(dhdb_t *s, 
//
//

// "field/subfield"	Pick field with name 'field' and then pick field with name 'subfiend'
//
// Mixed accessing
// "1/field"
// "field/2.."

#include "dhdb.h"

dhdb_t* 	dhdb_path	(dhdb_t *s, const char *path, dhdb_t *current);

/*
	dhdb_t *s = dhdb_create();
	// ...
	dhdb_t *current = NULL;
	while (current = dhdb_path(s, "..2", current)) {
	}

	// OR pick single result
	dhdb_t *s = dhdb_create();
	// ...
	dhdb_t *current = dhdb_path(s, "..2", NULL);
*/

#endif
