/* io_output.c */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001 Niels Möller
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>

#include <unistd.h>

#include "io.h"

#include "xmalloc.h"

struct sftp_output
{
  int fd;

  /* The message type is the first byte of a message, after the
   * length. */
  uint8_t msg;

  /* The next word is either the id, or the version. */
  uint32_t first;

  /* The rest of the packet is variable length. */
  uint8_t *data;
  uint32_t size;
  uint32_t i;
};


/* The first part of the buffer is always
 *
 * uint32 length
 * uint8  msg
 * uint32 id/version
 */

struct sftp_output *
sftp_make_output(int fd)
{
  struct sftp_output *o = xmalloc(sizeof(struct sftp_output));

  o->fd = fd;
  o->data = NULL;
  o->size = 0;
  o->i = 0;

  return o;
}

void
sftp_set_msg(struct sftp_output *o, uint8_t msg)
{
  o->msg = msg;
}

void
sftp_set_id(struct sftp_output *o, uint32_t id)
{
  o->first = id;
}

static void
sftp_check_output(struct sftp_output *o, uint32_t length)
{
  uint32_t needed = o->i + length;
  if (!o->data || (needed > o->size))
  {
    uint32_t size = 2 * needed + 40;
    o->data = xrealloc(o->data, size);

    o->size = size;
  }
}

void
sftp_put_data(struct sftp_output *o, uint32_t length, const uint8_t *data)
{
  sftp_check_output(o, length);

  memcpy(o->data + o->i, data, length);
  o->i += length;
}

void
sftp_put_uint8(struct sftp_output *o, uint8_t value)
{
  sftp_check_output(o, 1);

  o->data[o->i++] = value;
}

uint8_t *
sftp_put_start(struct sftp_output *o, uint32_t length)
{
  sftp_check_output(o, length);

  return o->data + o->i;
}

void
sftp_put_end(struct sftp_output *o, uint32_t length)
{
  o->i += length;
}

uint32_t
sftp_put_reserve_length(struct sftp_output *o)
{
  uint32_t index;
  sftp_check_output(o, 4);

  index = o->i;
  o->i += 4;

  return index;
}

void
sftp_put_length(struct sftp_output *o,
		uint32_t index,
		uint32_t length)
{
  assert( (index + 4) < o->i);
  WRITE_UINT32(o->data + index, length);
}

void
sftp_put_final_length(struct sftp_output *o,
		      uint32_t index)
{
  sftp_put_length(o, index, o->i - index - 4);
}

void
sftp_put_reset(struct sftp_output *o,
	       uint32_t index)
{
  assert(index < o->i);
  o->i = index;
}

int
sftp_write_packet(struct sftp_output *o)
{
  int j;
  int written = 0;
  uint32_t length = o->i + 5;
  uint8_t buf[9];

  WRITE_UINT32(buf, length);
  buf[4] = o->msg;
  WRITE_UINT32(buf + 5, o->first);

  /* Write 9 bytes from buf */

  while (written<9) 
    {
      j = write(o->fd, buf+written, 9-written);
      
      while (-1==j && errno==EINTR)  /* Loop over EINTR */
	j =  write(o->fd, buf+written, 9-written);;
      
      if (-1==j) /* Error, and not EINTR */
	return -1; /* Error */
      
      written += j; /* Move counters accordingly */      
    }

  /* Write o->i bytes from data */

  written = 0; /* Reset counter */

  while (written<o->i) 
    {
      j = write(o->fd, o->data+written, o->i-written);
      
      while (-1==j && errno==EINTR)  /* Loop over EINTR */
	j =  write(o->fd, o->data+written, o->i-written);;
      
      if (-1==j) /* Error, and not EINTR */
	return -1; /* Error */
      
      written += j; /* Move counters accordingly */      
    }

  o->i = 0;

  return 1;
}

int
sftp_packet_size(struct sftp_output* out)
{
  return out->i;
}
