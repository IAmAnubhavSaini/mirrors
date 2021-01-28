/* write_buffer.c
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

#include "write_buffer.h"

#include "io.h"
#include "lsh_string.h"
#include "xalloc.h"
#include "werror.h"


#define GABA_DEFINE
#include "write_buffer.h.x"
#undef GABA_DEFINE


static void
do_write(struct abstract_write *w,
	 struct lsh_string *packet)
{
  CAST(write_buffer, closure, w);
  uint32_t length = lsh_string_length(packet);
  
  debug("write_buffer: do_write length = %i\n",
	length);
  if (!length)
    {
      lsh_string_free(packet);
      return;
    }

  if (closure->closed)
    {
      werror("write_buffer: Attempt to write data to closed buffer.\n");
      lsh_string_free(packet);
      return;
    }
  
  /* Enqueue packet */

  string_queue_add_tail(&closure->q, packet);
  
  if (!closure->length)
    /* We're making the buffer non-empty */
    lsh_oop_register_write_fd(closure->fd);
  
  closure->empty = 0;
  
  closure->length += length;

  debug("write_buffer: do_write closure->length = %i\n",
	closure->length);
}

/* Copy data as necessary, before writing.
 *
 * FIXME: Writing of large packets could probably be optimized by
 * avoiding copying it into the buffer.
 *
 * Returns 1 if the buffer is non-empty. */
int write_buffer_pre_write(struct write_buffer *buffer)
{
  uint32_t length = buffer->end - buffer->start;

  if (buffer->empty)
    return 0;
  
  if (buffer->start > buffer->block_size)
    {
      /* Copy contents to the start of the buffer. There's no
	 overlap */
      lsh_string_write(buffer->buffer, 0, length,
		       lsh_string_data(buffer->buffer) + buffer->start);
      buffer->start = 0;
      buffer->end = length;
    }

  while (length < buffer->block_size)
    {
      /* Copy more data into buffer */
      if (buffer->partial)
	{
	  uint32_t partial_left
	    = lsh_string_length(buffer->partial) - buffer->pos;
	  uint32_t buffer_left = 2*buffer->block_size - buffer->end;
	  if (partial_left <= buffer_left)
	    {
	      /* The rest of the partial packet fits in the buffer */
	      trace("write_buffer_pre_write: buffer->buffer: length = %i\n"
		    "  buffer->start: %i, buffer->end: %i\n"
		    "  partial_left: %i, buffer->pos: %i\n",
		    lsh_string_length(buffer->buffer),
		    buffer->start, buffer->end,
		    partial_left, buffer->pos);
	      
	      lsh_string_write(buffer->buffer, buffer->end,
			       partial_left,
			       lsh_string_data(buffer->partial) + buffer->pos);

	      buffer->end += partial_left;
	      length += partial_left;
	      
	      lsh_string_free(buffer->partial);
	      buffer->partial = NULL;
	    }
	  else
	    {
	      lsh_string_write(buffer->buffer, buffer->end,
			       buffer_left,
			       lsh_string_data(buffer->partial) + buffer->pos);

	      buffer->end += buffer_left;
	      length += buffer_left;
	      buffer->pos += buffer_left;

	      assert(length >= buffer->block_size);
	    }
	}
      else
	{
	  /* Dequeue a packet, if possible */
	  if (!string_queue_is_empty(&buffer->q))
	    {
	      buffer->partial = string_queue_remove_head(&buffer->q);
	      buffer->pos = 0;
	    }
	  else
	    break;
	}
    }
  buffer->empty = !length;
  return !buffer->empty;
}

void write_buffer_consume(struct write_buffer *buffer, uint32_t size)
{
  buffer->start += size;
  assert(buffer->start <= buffer->end);
  buffer->length -= size;

  if (buffer->report)
    FLOW_CONTROL_REPORT(buffer->report, size);
}

struct write_buffer *
make_write_buffer(struct lsh_fd *fd, uint32_t size)
{
  NEW(write_buffer, res);

  res->super.write = do_write;

  res->fd = fd;
  
  res->block_size = size;

  res->buffer = lsh_string_alloc(2 * size);
  
  res->empty = 1;
  res->length = 0;
  
  res->closed = 0;
  
#if 0
  res->try_write = try; 
#endif
  
  string_queue_init(&res->q);

  res->pos = 0;
  res->partial = NULL;

  res->start = res->end = 0;

  return res;
}

  
