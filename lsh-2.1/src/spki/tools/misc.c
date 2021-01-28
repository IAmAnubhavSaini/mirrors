/* misc.c */

/* libspki
 *
 * Copyright (C) 2003 Niels Möller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdarg.h>
#include <stdlib.h>

#include "misc.h"

void *
xalloc(size_t size)
{
  void *p = malloc(size);
  if (!p)
    die("Virtual memory exhausted.\n");
  return p;
}

void
die(const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  exit(EXIT_FAILURE);
}

#define BUFSIZE 1000

/* NOTE: More or less the same as as nettle/examples/io.c */
uint8_t *
hash_file(const struct nettle_hash *hash, FILE *f)
{
  void *ctx = alloca(hash->context_size);
  hash->init(ctx);
  
  for (;;)
    {
      char buffer[BUFSIZE];
      size_t res = fread(buffer, 1, sizeof(buffer), f);
      if (ferror(f))
	return NULL;
      
      hash->update(ctx, res, buffer);
      if (feof(f))
	{
	  uint8_t *digest = xalloc(hash->digest_size);
	  hash->digest(ctx, hash->digest_size, digest);
	  return digest;
	}
    }
  
}

/* If size is > 0, read at most that many bytes. If size == 0,
 * read until EOF. Allocates the buffer dynamically. */
char *
read_file(FILE *f, unsigned max_size, unsigned *length)
{
  unsigned size;
  unsigned done;
  char *buffer;
  buffer = NULL;

  if (max_size && max_size < 100)
    size = max_size;
  else
    size = 100;
  
  for (size = 100, done = 0;
       (!max_size || done < max_size) && !feof(f);
       size *= 2)
    {
      char *p;

      if (max_size && size > max_size)
	size = max_size;

      /* Space for terminating NUL */
      p = realloc(buffer, size + 1);

      if (!p)
	{
	fail:
	  fclose(f);
	  free(buffer);

	  return NULL;
	}

      buffer = p;
      done += fread(buffer + done, 1, size - done, f);

      if (ferror(f))
	goto fail;
    }
  
  /* NUL-terminate the data. */
  buffer[done] = '\0';

  *length = done;
  return buffer;
}

/* If size is > 0, read at most that many bytes. If size == 0,
 * read until EOF. Allocates the buffer dynamically. */
char *
read_file_by_name(const char *name, unsigned max_size, unsigned *length)
{
  char *data;
  FILE *f;
    
  f = fopen(name, "rb");
  if (!f)
    return 0;

  data = read_file(f, max_size, length);
  fclose(f);

  return data;
}

int
write_file(FILE *f, unsigned size, const char *buffer)
{
  unsigned res;
  
  res = fwrite(buffer, 1, size, f);
  
  if (res < size || ferror(f))
    res = 0;

  return res > 0;
}
