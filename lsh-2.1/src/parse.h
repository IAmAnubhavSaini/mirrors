/* parse.h
 *
 * Parses the data formats used in ssh packets.
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

#ifndef LSH_PARSE_H_INCLUDED
#define LSH_PARSE_H_INCLUDED

#include <nettle/bignum.h>

#include "atoms.h"

/* Simple buffer
 * NOTE: All instances are allocated on the stack.
 * No object header is needed. */

struct simple_buffer
{
  uint32_t capacity;
  uint32_t pos;
  const uint8_t *data;
};

void
simple_buffer_init(struct simple_buffer *buffer,
		   uint32_t capacity, const uint8_t *data);

/* Returns 1 on success, 0 on failure */
int
parse_uint32(struct simple_buffer *buffer, uint32_t *result);

/* Only records length and start pointer */
int
parse_string(struct simple_buffer *buffer,
	     uint32_t *length, const uint8_t **start);

/* Copies a given number of octets, without any length header */
int
parse_octets(struct simple_buffer *buffer,
	     uint32_t length, uint8_t *start);

/* Copies a substring */
struct lsh_string *
parse_string_copy(struct simple_buffer *buffer);

/* Initializes subbuffer to parse a string from buffer */
int
parse_sub_buffer(struct simple_buffer *buffer,
		 struct simple_buffer *subbuffer);

int
parse_uint8(struct simple_buffer *buffer, unsigned *result);

/* Returns 1 on success, 0 on error, and -1 at end of buffer */
int
parse_utf8(struct simple_buffer *buffer, uint32_t *result, unsigned *utf8_length);

int parse_boolean(struct simple_buffer *buffer, int *result);

int
parse_bignum(struct simple_buffer *buffer, mpz_t result, uint32_t limit);

int
parse_atom(struct simple_buffer *buffer, int *result);

/* Reads a list of zero or more atoms. The buffer should hold the list
 * body; the length field should already be stripped off (usually by
 * parse_sub_buffer()). */
struct int_list *
parse_atoms(struct simple_buffer *buffer, unsigned limit);

/* Creates a list of integers. The 0 atom means an unknown atom was
 * read. Returns a NULL pointer on error. */
struct int_list *
parse_atom_list(struct simple_buffer *buffer, unsigned limit);

int
parse_uint16(struct simple_buffer *buffer, uint32_t *result);

int
parse_string16(struct simple_buffer *buffer,
	       uint32_t *length, const uint8_t **start);

void
parse_rest(struct simple_buffer *buffer,
	   uint32_t *length, const uint8_t **start);

/* Copies the rest of the buffer into a string. */
struct lsh_string *
parse_rest_copy(struct simple_buffer *buffer);

/* Returns success (i.e. 1) iff there is no data left */
int
parse_eod(struct simple_buffer *buffer);

#endif /* LSH_PARSE_H_INCLUDED */
