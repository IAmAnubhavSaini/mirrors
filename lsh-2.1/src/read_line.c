/* read_line.c
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

#include "read_line.h"

#include "lsh_string.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "read_line.h.x"
#undef GABA_DEFINE

#include "read_line.c.x"

/* GABA:
   (class
     (name read_line)
     (super read_handler)
     (vars
       (handler object line_handler)
       (e object exception_handler)
       
       ; Line buffer       
       (pos . uint32_t)
       (buffer string)))
*/

static uint32_t
do_read_line(struct read_handler **h,
	     uint32_t available,
	     const uint8_t *data)
{
  CAST(read_line, self, *h);

  uint8_t *eol;
  uint32_t consumed;
  uint32_t tail;
  uint32_t length;

  if (!available)
    {
      /* FIXME: Should we use some other exception type for this? */
      EXCEPTION_RAISE(self->e,
		      make_protocol_exception(0, "Unexpected EOF"));
      *h = NULL;
      return 0;
    }
  
  eol = memchr(data, 0x0a, available);

  if (!eol)
    {
      /* No newline character yet */
      if (available + self->pos >= 255)
	{
	  /* Too long line */
	  EXCEPTION_RAISE(self->e,
			  make_protocol_exception(0, "Line too long."));
	}
      else
	{
	  lsh_string_write(self->buffer, self->pos, available, data);
	  self->pos += available;
	}
      return available;
    }

  tail = eol - data; /* Excludes the newline character */
  consumed = tail + 1; /* Includes newline character */

  if ( (self->pos + consumed) > MAX_LINE)
    {
      /* Too long line */
      EXCEPTION_RAISE(self->e,
		      make_protocol_exception(0, "Line too long."));
      return available;
    }

  /* Ok, now we have a line. Copy it into the buffer. */
  lsh_string_write(self->buffer, self->pos, tail, data);
  length = self->pos + tail;
  
  /* Exclude carriage return character, if any */
  if (length && (data[length-1] == 0xd))
    length--;

  /* NOTE: This call can modify both self->handler and *h. */
  PROCESS_LINE(self->handler, h,
	       length, lsh_string_data(self->buffer), self->e);
		    
  /* Reset */
  self->pos = 0;

  return consumed;
}

struct read_handler *make_read_line(struct line_handler *handler,
				    struct exception_handler *e)
{
  NEW(read_line, closure);
  assert(e);
  
  closure->super.handler = do_read_line;
  closure->pos = 0;

  closure->buffer = lsh_string_alloc(MAX_LINE);
  closure->handler = handler;
  closure->e = e;
  
  return &closure->super;
}

  
