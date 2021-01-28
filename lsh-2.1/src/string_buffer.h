/* string_buffer.h
 *
 * Functions for building strings whose lengths are not known from the
 * start.
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

#ifndef LSH_STRING_BUFFER_H_INCLUDED
#define LSH_STRING_BUFFER_H_INCLUDED

/* We optimize for operations where we can guess an upper limit which
 * is reasonable most of the time. */

#include "lsh.h"

struct string_node;

struct string_buffer
{
#if 0
  /* Fail if the buffer grows larger than this value; zero means that
   * there is no limit. */
  uint32_t max;
  
  /* Amount of space to allocate at a time */
  uint32_t increment;
#endif

  struct lsh_string *partial; /* Partial block. */

  uint32_t left;
  uint32_t pos;

  /* List of blocks beyond the first one */
  struct string_node *tail;
#if 0
  unsigned nlist; /* Number of nodes */
#endif
  uint32_t total; /* Total string length, in list (i.e. not including
		 * partial) */
};

void string_buffer_init(struct string_buffer *buffer, uint32_t guess);

#if 0
int string_buffer_putc(struct string_buffer *buffer, uint8_t c);
int string_buffer_write(struct string_buffer *buffer,
			uint32_t length, const uint8_t *s);
#endif

void string_buffer_clear(struct string_buffer *buffer);

/* Assumes that the buffer->partial string is full */
void string_buffer_grow(struct string_buffer *buffer, uint32_t increment);

struct lsh_string *string_buffer_final(struct string_buffer *buffer,
				       uint32_t left);

#endif /* LSH_STRING_BUFFER_H_INCLUDED */
