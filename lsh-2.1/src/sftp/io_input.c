/* io_input.c */

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

#include <unistd.h>

#include "io.h"

#include "xmalloc.h"

#define SFTP_MAX_STRINGS 2

struct sftp_input
{
  int fd;
  uint32_t left;

  /* Strings that we own */
  uint8_t *strings[SFTP_MAX_STRINGS];
  unsigned used_strings;
};


int
sftp_check_input(const struct sftp_input *i, uint32_t length)
{
  return (i->left >= length);
}

int
sftp_get_data(struct sftp_input *i, uint32_t length, uint8_t *data)
{
  if (sftp_check_input(i, length))
    {
      uint8_t* buf = data;
      int j;

      while (length) 
	{
	  j = read(i->fd, buf, length);
      
	  while (-1==j && EINTR==errno)  /* Loop over EINTR */
	    j = read(i->fd, buf, length);
	  
	  if (-1==j) /* Error, and not EINTR */
	    return -1; /* Error */

	  buf += j; /* Move counters accordingly */
	  length -= j;
	  i->left -= j;
	}
      return 1; /* Success */
    }
  return 0; /* FIXME: Return code? */
}


struct sftp_input *
sftp_make_input(int fd)
{
  struct sftp_input *i = xmalloc(sizeof(struct sftp_input));

  i->fd = fd;
  i->left = 0;
  i->used_strings = 0;

  return i;
}

void
sftp_input_clear_strings(struct sftp_input *i)
{
  unsigned k;

  for (k = 0; k < i->used_strings; k++)
    sftp_free_string(i->strings[k]);

  i->used_strings = 0;
}

void
sftp_input_remember_string(struct sftp_input *i, uint8_t *s)
{
  assert(i->used_strings < SFTP_MAX_STRINGS);

  i->strings[i->used_strings++] = s;
}

int
sftp_get_eod(struct sftp_input *i)
{
  return !i->left;
}

/* Returns 1 of all was well, 0 on error, and -1 on EOF */
int
sftp_read_packet(struct sftp_input *i)
{
  uint8_t buf[4];
  int bytesread = 0;

  if (i->left) /* Unread data? */
    {
      uint8_t d;

      while (i->left &&                         /* Data remaining? */
	     0<sftp_get_data(i, 1, &d)         /* Read OK? */
	     )
	;

      /* Now, there shouldn't be any more data remaining. Next time
       * we're called, the next packet should be read (or we had an
       * error and all data is not read, if that's the case, return
       * error).
       */

      if (i->left)    /* i->left non-zero => sftp_get_data failed => error  */
	return -1; 

      return 0;
    }

  /* First, deallocate the strings. */
  sftp_input_clear_strings(i);

  while (bytesread < sizeof(buf))
    {
      int j = read(i->fd, buf+bytesread, sizeof(buf)-bytesread);

      while(-1==j && EINTR==errno)   /* Loop over EINTR */
	j = read(i->fd, buf+bytesread, sizeof(buf)-bytesread);
  
      if (-1==j) /* Not EINTR but a real error */
	return -1;

      if (j == 0)
	/* EOF */
	return bytesread ? 0 : -1;
      bytesread += j;
    }
  
  i->left = READ_UINT32(buf); /* Store packet size */
  return 1;

}
