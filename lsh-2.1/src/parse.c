/* parse.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include <nettle/bignum.h>
#include <nettle/macros.h>

#include "parse.h"

#include "format.h"
#include "list.h"
#include "werror.h"
#include "xalloc.h"

#include "parse_macros.h"

void
simple_buffer_init(struct simple_buffer *buffer,
		   uint32_t capacity, const uint8_t *data)
{
  buffer->capacity = capacity;
  buffer->pos = 0;
  buffer->data = data;
}

int
parse_uint32(struct simple_buffer *buffer, uint32_t *result)
{
  if (LEFT < 4)
    return 0;

  *result = READ_UINT32(HERE);
  ADVANCE(4);
  return 1;
}

int
parse_string(struct simple_buffer *buffer,
	     uint32_t *length, const uint8_t **start)
{
  uint32_t l;

  if (!parse_uint32(buffer, &l))
    return 0;

  if (LEFT < l)
    return 0;

  *length = l;
  *start = HERE;
  ADVANCE(l);
  return 1;
}

/* FIXME: Get rid of this memcpy? */
int
parse_octets(struct simple_buffer *buffer,
	     uint32_t length, uint8_t *start)
{
  if (LEFT < length)
    return 0;
  memcpy(start, HERE, length);
  ADVANCE(length);
  return 1;
}

struct lsh_string *
parse_string_copy(struct simple_buffer *buffer)
{
  uint32_t length;
  const uint8_t *start;
  
  if (!parse_string(buffer, &length, &start))
    return NULL;

  return ssh_format("%ls", length, start);
}

/* Initializes subbuffer to parse a string from buffer */
int
parse_sub_buffer(struct simple_buffer *buffer,
		 struct simple_buffer *subbuffer)
{
  uint32_t length;
  const uint8_t *data;

  if (!parse_string(buffer, &length, &data))
    return 0;

  simple_buffer_init(subbuffer, length, data);
  return 1;
}

int
parse_uint8(struct simple_buffer *buffer, unsigned *result)
{
  if (!LEFT)
    return 0;

  *result = HERE[0];
  ADVANCE(1);
  return 1;
}

int
parse_utf8(struct simple_buffer *buffer, uint32_t *result, unsigned *utf8_length)
{
  static const uint32_t min_value[7] =
    {
      0, 0,
           0x80,
          0x800,
        0x10000,
       0x200000,
      0x4000000,
    };

  uint32_t c;
  uint32_t value;
  unsigned length;
  unsigned i;
  
  if (!LEFT)
    return -1;

  c = HERE[0];

  if (c < 0x80)
    {
      *result = c;
      *utf8_length = 1;
      ADVANCE(1);
      return 1;
    }

  switch(c & 0xF0)
    {
    default:
      ADVANCE(1);
      *utf8_length = 1;
      return 0;
    case 0xC0:
    case 0xD0:
      /* Format 110y yyyy 10xx xxxx, 11 bits */
      length = 2;
      value = c & 0x1F;      
      break;
    case 0xE0:
      /* Format 1110 zzzz 10yy yyyy 10xx xxxx, 16 bits */
      length = 3;
      value = c & 0x0F;
      break;
    case 0xF0:
      switch(c & 0x0E)
	{
	case 0: case 2: case 4: case 6:
	  /* Format 1111 0www 10zz zzzz 10yy yyyy 10xx xxxx, 21 bits */
	  length = 4;
	  value = c & 0x07;
	  break;
	case 8: case 0xA:
	  /* Format 1111 10xx 10ww wwww 10zz zzzz 10yy yyyy 10xx xxxx, 26 bits */
	  length = 5;
	  value = c & 0x03;
	  break;
	case 0xC:
	  /* Format 1111 110y 10xx xxxx 10ww wwww 10zz zzzz 10yy yyyy 10xx xxxx, 31 bits */
	  length = 6;
	  value = c & 0x01;
	  break;
	default:
	  /* Invalid format 1111 111x */
	  ADVANCE(1);
	  return 0;
	}
      break;
    }
  if (LEFT < length)
    {
      ADVANCE(LEFT);
      *utf8_length = 1;
      return 0;
    }
  c = HERE[1];
  
  if ( (c & 0xC0) != 0x80)
    {
      ADVANCE(1);
      *utf8_length = 1;
      return 0;  
    }
  value = (value << 6) | (c & 0x3f);

  for(i = 2; i<length; i++)
    {
      c = HERE[i];
      if ( (c & 0xC0) != 0x80)
	{
	  ADVANCE(i);
	  *utf8_length = i;
	  return 0;
	}
      value = (value << 6) | (c & 0x3f);
    }

  ADVANCE(length);
  *utf8_length = length;
  
  /* Check for overlong sequences */
  if (value < min_value[length])
    return 0;

  /* Surrogates and non-characters should not appear in utf8 text. */
  if ( (value >= 0xd800 && value <0xe000)
       || value == 0xfffe || value == 0xffff)
    return 0;
  
  *result = value;
  
  return 1;
}  
      
int
parse_boolean(struct simple_buffer *buffer, int *result)
{
  if (!LEFT)
    return 0;
  *result = HERE[0];
  ADVANCE(1);
  return 1;
}

int
parse_bignum(struct simple_buffer *buffer, mpz_t result, uint32_t limit)
{
  uint32_t length;
  const uint8_t *digits;

  if (!parse_string(buffer, &length, &digits))
    return 0;

  /* NOTE: If the reciever expects an integer less than 256^limit,
   * there may still be limit + 1 digits, as a leading zero is
   * sometimes required to resolve signedness. */
  if (limit && (length > (limit + 1)))
    return 0;

  nettle_mpz_set_str_256_s(result, length, digits);

  return 1;
}

int
parse_atom(struct simple_buffer *buffer, int *result)
{
  uint32_t length;
  const uint8_t *start;

  if ( (!parse_string(buffer, &length, &start))
       || length > 64)
    return 0;

  *result = lookup_atom(length, start);

  return 1;
}

/* NOTE: This functions record the fact that it has read to the end of
 * the buffer by setting the position to *beyond* the end of the
 * buffer. */
static int
parse_next_atom(struct simple_buffer *buffer, int *result)
{
  uint32_t i;

  assert (buffer->pos <=buffer->capacity);

  for(i = 0; i < LEFT; i++)
    {
      if (HERE[i] == ',')
	break;
      if (i == 64)
	/* Atoms can be no larger than 64 characters */
	return 0;
    }

  /* NOTE: ssh-2.0.13, server string "SSH-1.99-2.0.13
   * (non-commercial)", sends USERAUTH_FAILURE messages with strings
   * like "publickey,password,". It's not entirely clear to me if that
   * is allowed for the spec, but it seems safe and straightforward to
   * treat empty atoms as any other unknown atom. */
  if (!i)
    {
      verbose("parse_next_atom: Received an empty atom.\n");
      /* Treat empty atoms as unknown */
      *result = 0;
    }
  else
    *result = lookup_atom(i, HERE);

  ADVANCE(i+1);  /* If the atom was terminated at the end of the
		  * buffer, rather than by a comma, this points beyond
		  * the end of the buffer */
  return 1;
}

struct int_list *
parse_atoms(struct simple_buffer *buffer, unsigned limit)
{
  unsigned count;
  unsigned i;
  struct int_list *res;

  assert(limit);

  if (!LEFT)
    return make_int_list(0, -1);
  
  /* Count commas (no commas means one atom) */
  for (i = buffer->pos, count = 1; i < buffer->capacity; i++)
    if (buffer->data[i] == ',')
      {
	if (count >= limit)
	  return NULL;
	count++;
      }

  res = alloc_int_list(count);

  for (i = 0; i < count; i++)
    {
      if (!parse_next_atom(buffer, LIST(res)+i))
	{
	  KILL(res);
	  return NULL;
	}
    }

  return res;
}

struct int_list *
parse_atom_list(struct simple_buffer *buffer, unsigned limit)
{
  struct simple_buffer sub_buffer;

  if (!parse_sub_buffer(buffer, &sub_buffer))
    return NULL;

  return parse_atoms(&sub_buffer, limit);
}

/* Used by client_x11.c, for parsing xauth */
int
parse_uint16(struct simple_buffer *buffer, uint32_t *result)
{
  if (LEFT < 2)
    return 0;

  *result = READ_UINT16(HERE);
  ADVANCE(2);
  return 1;
}

int
parse_string16(struct simple_buffer *buffer,
	       uint32_t *length, const uint8_t **start)
{
  uint32_t l;

  if (!parse_uint16(buffer, &l))
    return 0;

  if (LEFT < l)
    return 0;

  *length = l;
  *start = HERE;
  ADVANCE(l);
  return 1;
}

void
parse_rest(struct simple_buffer *buffer,
	   uint32_t *length, const uint8_t **start)
{
  *length = LEFT;
  *start = HERE;

  ADVANCE(*length);
}

struct lsh_string *
parse_rest_copy(struct simple_buffer *buffer)
{
  uint32_t length = LEFT;
  struct lsh_string *s = ssh_format("%ls", length, HERE);

  ADVANCE(length);
  assert(!LEFT);

  return s;
}

/* Returns success (i.e. 1) iff there is no data left */
int
parse_eod(struct simple_buffer *buffer)
{
  return !LEFT;
}
