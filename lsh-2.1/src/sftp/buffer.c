/* buffer.c
 *
 * Reading and writing sftp data.
 */

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
#include <stdarg.h>
#include <stdio.h>

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <stdlib.h>

#include "buffer.h"

#include "xmalloc.h"
#include "sftp.h"


/* Input */

#define GET_DATA(i, buf) \
(sftp_get_data((i), sizeof((buf)), (buf)))

int
sftp_get_uint8(struct sftp_input *i, uint8_t *value)
{
  return sftp_get_data(i, 1, value);    
}

int
sftp_get_uint32(struct sftp_input *i, uint32_t *value)
{
  uint8_t buf[4];
  if (!GET_DATA(i, buf))
    return 0;

  *value = READ_UINT32(buf);
  return 1;
}

uint8_t *
sftp_get_string(struct sftp_input *i, uint32_t *length)
{
  uint8_t *data;

  if (!(sftp_get_uint32(i, length) && sftp_check_input(i, *length)))
    return NULL;

  data = xmalloc(*length + 1);

  if (!sftp_get_data(i, *length, data))
    {
      free(data);
      return NULL;
    }

  /* NUL-terminate, for convenience */
  data[*length] = '\0';

  return data;
}

void
sftp_free_string(uint8_t *s)
{
  free(s);
}

uint8_t *
sftp_get_string_auto(struct sftp_input *i, uint32_t *length)
{
  uint8_t *data;

  data = sftp_get_string(i, length);

  if (!data)
    return NULL;

  /* Remember the string. */
  sftp_input_remember_string(i, data);
  
  return data;
}


/* Output */


#define PUT_DATA(o, buf) \
(sftp_put_data((o), sizeof((buf)), (buf)))

void
sftp_put_uint32(struct sftp_output *o, uint32_t value)
{
  uint8_t buf[4];

  WRITE_UINT32(buf, value);
  PUT_DATA(o, buf);
}

void
sftp_put_string(struct sftp_output *o, uint32_t length, const uint8_t *data)
{
  sftp_put_uint32(o, length);
  sftp_put_data(o, length, data);
}


uint32_t
sftp_put_printf(struct sftp_output *o, const char *format, ...)
{
  /* Initial buffer space */
  int needed;
  int length;
  
  for (needed = 100;; needed *= 2)
    {
      va_list args;
      uint8_t *current;
      
      va_start(args, format);

      current = sftp_put_start(o, needed);
      length = vsnprintf(current, needed, format, args);
      
      va_end(args);

      if ( (length >= 0) && (length < needed))
	break;
    }
  sftp_put_end(o, length);
  
  return length;
}

/* If SIZE > 0 it is the desired field length, and
 * smaller output is padded with blanks. */
uint32_t
sftp_put_strftime(struct sftp_output *o, uint32_t size, const char *format,
		  const struct tm *tm)
{
  /* Initial buffer space */
  size_t needed;
  size_t length;
  uint8_t *current;
  
  for (needed = size ? size : 100;; needed *= 2)
    {
      current = sftp_put_start(o, needed);
      length = strftime(current, needed, format, tm);

      if ( (length > 0) && (length < needed))
	break;
    }

  while (length < size)
    current[length++] = ' ';

  sftp_put_end(o, length);
  
  return length;
}


/* 64-bit stuff */
#if SIZEOF_OFF_T > 4
#define READ_UINT64(p)				\
(  (((off_t) (p)[0]) << 56)			\
 | (((off_t) (p)[1]) << 48)			\
 | (((off_t) (p)[2]) << 40)			\
 | (((off_t) (p)[3]) << 32)			\
 | (((off_t) (p)[4]) << 24)			\
 | (((off_t) (p)[5]) << 16)			\
 | (((off_t) (p)[6]) << 8)			\
 |  ((off_t) (p)[7]))


int
sftp_get_uint64(struct sftp_input *i, off_t *value)
{
  uint8_t buf[8];
  if (!GET_DATA(i, buf))
    return 0;

  *value = READ_UINT64(buf);
  return 1;
}

#define WRITE_UINT64(p, i)			\
do {						\
  (p)[0] = ((i) >> 56) & 0xff;			\
  (p)[1] = ((i) >> 48) & 0xff;			\
  (p)[2] = ((i) >> 40) & 0xff;			\
  (p)[3] = ((i) >> 32) & 0xff;			\
  (p)[4] = ((i) >> 24) & 0xff;			\
  (p)[5] = ((i) >> 16) & 0xff;			\
  (p)[6] = ((i) >> 8) & 0xff;			\
  (p)[7] = (i) & 0xff;				\
} while(0)

void
sftp_put_uint64(struct sftp_output *o, off_t value)
{
  uint8_t buf[8];

  WRITE_UINT64(buf, value);
  PUT_DATA(o, buf);
}

#else /* SIZEOF_OFF_T <= 4 */

/* Fail for too large numbers. */
int
sftp_get_uint64(struct sftp_input *i, off_t *value)
{
  uint32_t high;
  uint32_t low;

  if (sftp_get_uint32(i, &high)
      && !high
      && sftp_get_uint32(i, &low))
    {
      *value = low;
      return 1;
    }
  else
    return 0;
}

void
sftp_put_uint64(struct sftp_output *o, off_t value)
{
  sftp_put_uint32(o, 0);
  sftp_put_uint32(o, value);
}

#endif /* SIZEOF_OFF_T <= 4 */



/* General functions */

void
sftp_clear_attrib(struct sftp_attrib *a)
{
  a->flags = 0;
  a->size = 0;
  a->uid = 0;
  a->gid = 0;
  a->permissions = 0;
  a->atime = 0;
  a->mtime = 0;
};

int
sftp_skip_extension(struct sftp_input *i)
{
  uint32_t length;
  uint8_t *data;
  unsigned j;
  
  /* Skip name and value*/
  for (j = 0; j<2; j++)
    {
      if (!(data = sftp_get_string(i, &length)))
	return 0;
    }
  sftp_input_clear_strings(i);
  
  return 1;
}

int
sftp_get_attrib(struct sftp_input *i, struct sftp_attrib *a)
{
  sftp_clear_attrib(a);
  
  if (!sftp_get_uint32(i, &a->flags))
    return 0;

  if (a->flags & SSH_FILEXFER_ATTR_SIZE)
    {
      if (!sftp_get_uint64(i, &a->size))
	return 0;
    }

  if (a->flags & SSH_FILEXFER_ATTR_UIDGID)
    {
      if (!sftp_get_uint32(i, &a->uid))
	return 0;

      if (!sftp_get_uint32(i, &a->gid))
	return 0;
    }

  if (a->flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      if (!sftp_get_uint32(i, &a->permissions))
	return 0;
    }

  if (a->flags & SSH_FILEXFER_ATTR_ACMODTIME)
    {
      if (!sftp_get_uint32(i, &a->atime))
	return 0;

      if (!sftp_get_uint32(i, &a->mtime))
	return 0;
    }

  if (a->flags & SSH_FILEXFER_ATTR_EXTENDED)
    {
      uint32_t count;
      uint32_t n;

      if (!sftp_get_uint32(i, &count))
	return 0;

      /* Just skip the extensions */
      for (n = 0; n < count; n++)
	if (!sftp_skip_extension(i))
	  return 0;
    }
  return 1;
}

void
sftp_put_attrib(struct sftp_output *o, const struct sftp_attrib *a)
{
  assert(!(a->flags & SSH_FILEXFER_ATTR_EXTENDED));
  
  sftp_put_uint32(o, a->flags);

  if (a->flags & SSH_FILEXFER_ATTR_SIZE)
    sftp_put_uint64(o, a->size);

  if (a->flags & SSH_FILEXFER_ATTR_UIDGID)
    {
      sftp_put_uint32(o, a->uid);
      sftp_put_uint32(o, a->gid);
    }

  if (a->flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    sftp_put_uint32(o, a->permissions);

  if (a->flags & SSH_FILEXFER_ATTR_ACMODTIME)
    {
      sftp_put_uint32(o, a->atime);
      sftp_put_uint32(o, a->mtime);
    }
}
