/* string.c
 *
 * String handling. The point is to keep *all* manipulation of strings
 * and other buffers in this file.
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2003 Niels Möller
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

#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <nettle/buffer.h>
#include <nettle/cbc.h>
#include <nettle/ctr.h>
#include <nettle/hmac.h>
#include <nettle/macros.h>
#include <nettle/memxor.h>
#include <nettle/sexp.h>

#include "lsh_string.h"

#include "abstract_crypto.h"
#include "randomness.h"
#include "werror.h"
#include "xalloc.h"

/* First, a bunch of string operations that don't depend on the
   internal structure. */

int
lsh_string_eq_l(const struct lsh_string *a,
		uint32_t length, const uint8_t *b)
{
  return (lsh_string_length(a) == length
	  && !memcmp(lsh_string_data(a), b, length));
}

int
lsh_string_eq(const struct lsh_string *a, const struct lsh_string *b)
{
  return lsh_string_eq_l(a, STRING_LD(b));
}

int
lsh_string_prefixp(const struct lsh_string *prefix,
		   const struct lsh_string *s)
{
  uint32_t plength = lsh_string_length(prefix);
  return ( (plength <= lsh_string_length(s))
	   && !memcmp(lsh_string_data(prefix), lsh_string_data(s), plength));
}


void
lsh_string_write_string(struct lsh_string *s, uint32_t pos,
			const struct lsh_string *data)
{
  lsh_string_write(s, pos, STRING_LD(data));
}

struct lsh_string *
lsh_string_random(struct randomness *r, uint32_t length)
{
  struct lsh_string *s = lsh_string_alloc(length);
  lsh_string_write_random(s, 0, r, length);

  return s;
}

#if DEBUG_ALLOC
struct lsh_string_header
{
  int magic; /* For a sentinel value */
  /* Where/how the string was allocated */
  const char *clue;
  struct lsh_string *prev;
  struct lsh_string *next;
};
#endif  /* DEBUG_ALLOC */

struct lsh_string
{
#if DEBUG_ALLOC
  struct lsh_string_header header;
#endif
  /* Attached to read packets. Used only for generating proper
     SSH_MSG_UNIMPLEMENTED replies. */
  uint32_t sequence_number;
  /* NOTE: The allocated size may be larger than the string length. */
  uint32_t length; 
  uint8_t data[1];
};


uint32_t
lsh_string_length(const struct lsh_string *s)
{
  return s->length;
}

const uint8_t *
lsh_string_data(const struct lsh_string *s)
{
  return s->data;
}

/* Returns an ordinary NUL-terminated string, or NULL if the string
 * contains any NUL-character. */
const char *
lsh_get_cstring(const struct lsh_string *s)
{
  return (s && !memchr(s->data, '\0', s->length) ? s->data : NULL);
}

uint32_t
lsh_string_sequence_number(const struct lsh_string *s)
{
  return s->sequence_number;
}

void
lsh_string_set_sequence_number(struct lsh_string *s, uint32_t n)
{
  s->sequence_number = n;
}

void
lsh_string_putc(struct lsh_string *s, uint32_t i, uint8_t c)
{
  assert(i < s->length);
  s->data[i] = c;
}

#define ASSERT_ROOM(s, start, l) do {		\
  assert((start) <= (s)->length);		\
  assert((l) <= (s)->length - (start));		\
} while(0)

void
lsh_string_set(struct lsh_string *s, uint32_t start, uint32_t length, uint8_t c)
{
  ASSERT_ROOM(s, start, length);
  memset(s->data + start, c, length);
  assert(!s->data[s->length]);  
}

void
lsh_string_write(struct lsh_string *s, uint32_t start, uint32_t length,
		 const uint8_t *data)
{
  ASSERT_ROOM(s, start, length);

  memcpy(s->data + start, data, length);

  assert(!s->data[s->length]);
}

void
lsh_string_write_uint32(struct lsh_string *s, uint32_t start, uint32_t n)
{
  ASSERT_ROOM(s, start, 4);

  WRITE_UINT32(s->data + start, n);

  assert(!s->data[s->length]);
}

void
lsh_string_write_xor(struct lsh_string *s, uint32_t start, uint32_t length,
		     const uint8_t *data)
{
  assert(length);
  ASSERT_ROOM(s, start, length);

  memxor(s->data + start, data, length);

  assert(!s->data[s->length]);
}

void
lsh_string_write_bignum(struct lsh_string *s, uint32_t start,
			uint32_t length, const mpz_t n)
{
  ASSERT_ROOM(s, start, length);
  nettle_mpz_get_str_256(length, s->data + start, n);

  assert(!s->data[s->length]);  
}
     

/* NOTE: Destructive, returns the string only for convenience. */
struct lsh_string *
lsh_string_trunc(struct lsh_string *s, uint32_t length)
{
  assert(length <= s->length);
  s->length = length;
  /* NUL-terminate */
  s->data[length] = 0;

  return s;
}

void
lsh_string_crypt(struct lsh_string *dst, uint32_t di,
		 const struct lsh_string *src, uint32_t si,
		 uint32_t length,
		 nettle_crypt_func f, void *ctx)
{
  ASSERT_ROOM(dst, di, length);
  ASSERT_ROOM(src, si, length);

  assert (src != dst || di == si);
  f(ctx, length, dst->data + di, src->data+ si);

  assert(!dst->data[dst->length]);
}

void
lsh_string_cbc_encrypt(struct lsh_string *dst, uint32_t di,
		       const struct lsh_string *src, uint32_t si,
		       uint32_t length,
		       uint32_t block_size, uint8_t *iv,
		       nettle_crypt_func f, void *ctx)
{
  ASSERT_ROOM(dst, di, length);
  ASSERT_ROOM(src, si, length);

  /* Equal source and destination is ok, but no overlaps. */
  assert (src != dst || di == si);
  cbc_encrypt(ctx, f, block_size, iv,
	      length, dst->data + di, src->data + si);

  assert(!dst->data[dst->length]);
}

void
lsh_string_cbc_decrypt(struct lsh_string *dst, uint32_t di,
		       const struct lsh_string *src, uint32_t si,
		       uint32_t length,
		       uint32_t block_size, uint8_t *iv,
		       nettle_crypt_func f, void *ctx)
{
  ASSERT_ROOM(dst, di, length);
  ASSERT_ROOM(src, si, length);

  /* Equal source and destination is ok, but no overlaps. */
  assert (src != dst || di == si);
  cbc_decrypt(ctx, f, block_size, iv,
	      length, dst->data + di, src->data + si);

  assert(!dst->data[dst->length]);
}

void
lsh_string_ctr_crypt(struct lsh_string *dst, uint32_t di,
		     const struct lsh_string *src, uint32_t si,
		     uint32_t length,
		     uint32_t block_size, uint8_t *iv,
		     nettle_crypt_func f, void *ctx)
{
  ASSERT_ROOM(dst, di, length);
  ASSERT_ROOM(src, si, length);

  /* Equal source and destination is ok, but no overlaps. */
  assert (src != dst || di == si);
  ctr_crypt(ctx, f, block_size, iv,
	    length, dst->data + di, src->data + si);

  assert(!dst->data[dst->length]);
}

void
lsh_string_write_hash(struct lsh_string *s, uint32_t start,
		      const struct nettle_hash *type, void *ctx)
{
  ASSERT_ROOM(s, start, type->digest_size);
  type->digest(ctx, type->digest_size, s->data + start);
  assert(!s->data[s->length]);
}

/* FIXME: Requires linking with nettle */
void
lsh_string_write_hmac(struct lsh_string *s, uint32_t start,
		      const struct nettle_hash *type, uint32_t length,
		      const void *outer, const void *inner, void *state)
{
  ASSERT_ROOM(s, start, length);
  hmac_digest(outer, inner, state,
	      type, length, s->data + start);
  assert(!s->data[s->length]);
}

void
lsh_string_write_random(struct lsh_string *s, uint32_t start,
			struct randomness *r, uint32_t length)
{
  ASSERT_ROOM(s, start, length);
  RANDOM(r, length, s->data + start);
  assert(!s->data[s->length]);
}

#if WITH_IPV6
struct lsh_string *
lsh_string_ntop(int family, uint32_t length, const void *addr)
{
  struct lsh_string *s = lsh_string_alloc(length + 1);

  /* Does inet_ntop always use lower case letters? If not, we
   * should perhaps lowercase the result explicitly. */

  if (!inet_ntop(family, addr,
		 s->data, s->length))
    fatal("inet_ntop failed for IPv6 address.\n");

  lsh_string_trunc(s, strlen(s->data));

  return s;
}
#endif

/* FIXME: Always try to read as much as fits in the string? */
int
lsh_string_read(struct lsh_string *s, uint32_t start,
		int fd, uint32_t length)
{
  int res;
  assert(length);
  ASSERT_ROOM(s, start, length);

  res = read(fd, s->data + start, length);

  assert(!s->data[s->length]);

  return res;
}

/* Formatting s-expressions */
struct lsh_string *
lsh_string_format_sexp(int transport, const char *format, ...)
{
  struct lsh_string *s;
  va_list args;
  unsigned length;
  struct nettle_buffer buffer;

  unsigned (*vformat)(struct nettle_buffer *, const char *, va_list)
    = transport ? sexp_transport_vformat : sexp_vformat;
  
  va_start(args, format);
  length = vformat(NULL, format, args);
  va_end(args);

  s = lsh_string_alloc(length);
  nettle_buffer_init_size(&buffer, s->length, s->data);

  va_start(args, format);
  length = vformat(&buffer, format, args);
  va_end(args);

  assert(length == lsh_string_length(s));

  return s;
}


#if WITH_ZLIB
int
lsh_string_zlib(struct lsh_string *s, uint32_t start,
		int (*f)(z_stream *z, int flush),
		z_stream *z, int flush, uint32_t length)
{
  int res;
  
  ASSERT_ROOM(s, start, length);

  z->next_out = s->data + start;
  z->avail_out = length;
  
  res = f(z, flush);
  
  assert(!s->data[s->length]);
  return res;    
}
#endif

/* Base64 decodes a string in place */
int
lsh_string_base64_decode(struct lsh_string *s)
{
  struct base64_decode_ctx ctx;
  uint32_t done = s->length;

  base64_decode_init(&ctx);

  if (base64_decode_update(&ctx, &done, s->data,
			   s->length, s->data)
      && base64_decode_final(&ctx))
    {
      lsh_string_trunc(s, done);
      return 1;
    }
  return 0;
}

#if 0
unsigned
lsh_string_put_base64_single(struct lsh_string *s, uint32_t start,
			     struct base64_encode_ctx *ctx, uint8_t c)
{
  unsigned res;
  ASSERT_ROOM(s, start, 2);
  res = base64_encode_single(ctx, s->data + start, c);
  assert(!s->data[s->length]);
  return res;    
}
#endif

unsigned
lsh_string_base64_encode_update(struct lsh_string *s, uint32_t start,
				struct base64_encode_ctx *ctx,
				uint32_t length, const uint8_t *src)
{
  unsigned res;
  ASSERT_ROOM(s, start, BASE64_ENCODE_LENGTH(length));
  res = base64_encode_update(ctx, s->data + start, length, src);
  assert(!s->data[s->length]);
  return res;    
}

unsigned
lsh_string_base64_encode_final(struct lsh_string *s, uint32_t start,
			       struct base64_encode_ctx *ctx)
{
  unsigned res;
  ASSERT_ROOM(s, start, BASE64_ENCODE_FINAL_LENGTH);
  res = base64_encode_final(ctx, s->data + start);
  assert(!s->data[s->length]);
  return res;    
}

/* Decodes input string inplace. */
int
lsh_string_transport_iterator_first(struct lsh_string *s,
				    struct sexp_iterator *iterator)
{
  return sexp_transport_iterator_first(iterator,
				       s->length, s->data);
}


/* Allocation */
#if DEBUG_ALLOC
static struct lsh_string *all_strings = NULL;
static unsigned number_of_strings = 0;

static void
sanity_check_string_list(void)
{
  unsigned i = 0;
  struct lsh_string *s;

  if (!all_strings)
    {
      assert(!number_of_strings);
      return;
    }
  assert(!all_strings->header.prev);
  
  for(i = 0, s = all_strings; s; s = s->header.next, i++)
    {
      if (s->header.next)
	{
	  assert(s->header.next->header.prev = s);
	}
    }
  assert (i == number_of_strings);
}
#endif

#if DEBUG_ALLOC
#undef lsh_string_alloc
static
#endif

struct lsh_string *
lsh_string_alloc(uint32_t length)
{
  /* NOTE: The definition of the struct contains a char array of
   * length 1, so the below includes space for a terminating NUL. */
  
  struct lsh_string *s
    = lsh_malloc(sizeof(struct lsh_string) + length);

  if (!s)
    fatal("Virtual memory exhausted");

  s->length = length;
  s->data[length] = '\0';
  s->sequence_number = 0;
  
  return s;
}

#if DEBUG_ALLOC
struct lsh_string *
lsh_string_alloc_clue(uint32_t length, const char *clue)
{
  struct lsh_string *s = lsh_string_alloc(length);

  sanity_check_string_list();
  
  s->header.magic = -1717;
  number_of_strings++;

  s->header.clue = clue;
  s->header.next = all_strings;
  s->header.prev = NULL;
  if (s->header.next)
    s->header.next->header.prev = s;
  all_strings = s;

  sanity_check_string_list();

  return s;  
}

void
lsh_string_final_check(void)
{
  if (number_of_strings)
    {
      struct lsh_string *s;
      werror("gc_final: %i strings leaked!\n", number_of_strings);
      for (s = all_strings; s; s = s->header.next)
	werror("  clue: %z\n", s->header.clue);
      fatal("gc_final: Internal error!\n");
    }
}

unsigned
lsh_get_number_of_strings(void)
{
  return number_of_strings;
}

#endif

void
lsh_string_free(const struct lsh_string *s)
{
  if (!s)
    return;

#if DEBUG_ALLOC
  sanity_check_string_list();

  assert(number_of_strings);
  number_of_strings--;

  if (s->header.magic != -1717)
    fatal("lsh_string_free: Not string!\n");
  if (s->data[s->length])
    fatal("lsh_string_free: String not NUL-terminated.\n");

  if (s->header.next)
    s->header.next->header.prev = s->header.prev;
  
  if (s->header.prev)
    s->header.prev->header.next = s->header.next;
  else
    {
      assert (all_strings == s);
      all_strings = s->header.next;
    }
  
  sanity_check_string_list();	
#endif
  
  lsh_free(s);
}
