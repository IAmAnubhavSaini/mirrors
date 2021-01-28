/* charset.h
 *
 * Translate local characterset to and from utf8.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels Möller
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LSH_CHARSET_H_INCLUDED
#define LSH_CHARSET_H_INCLUDED

#include "lsh.h"

/* FIXME: Use charsets real objects, instead of using fixed constants */

#define CHARSET_UTF8 0
#define CHARSET_LATIN1 1
#define CHARSET_USASCII 2

void set_local_charset(int charset);

uint32_t local_to_ucs4(int c);
int ucs4_to_local(uint32_t c);

enum utf8_flag
  {
    /* If set, characters that can not be represented in
       the local charset are replaced by '?' */
    utf8_replace = 1,
    
    /* If set, control characters are treated as never existing in any
       local character set */
    utf8_paranoid = 2,

    /* If set, also invalid utf8 sequences are replaced by '?' */
    utf8_tolerant = 4
  };

struct lsh_string *local_to_utf8(struct lsh_string *s, int free);

/* Returns NULL if the UTF-8 encoding is invalid, or not
   representatble in the local charset (also depending on the given
   flags). */
struct lsh_string *utf8_to_local(struct lsh_string *s, enum utf8_flag flags, int free);
struct lsh_string *low_utf8_to_local(uint32_t length, const uint8_t *s, enum utf8_flag flags);

#endif /* LSH_CHARSET_H_INCLUDED */
