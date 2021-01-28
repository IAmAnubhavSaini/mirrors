/* string.h
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

#include "lsh.h"

/* Can we avoid this dependency? */
#include <nettle/bignum.h>
#include <nettle/base64.h>
#include <nettle/nettle-meta.h>

/* The memory allocation model (for strings) is as follows:
 *
 * Packets are allocated when they are needed. A packet may be passed
 * through a chain of processing functions, until it is finally
 * discarded or transmitted, at which time it is deallocated.
 * Processing functions may deallocate their input packets and
 * allocate fresh packets to pass on; therefore, any data from a
 * packet that is needed later must be copied into some other storage.
 *
 * At any time, each packet is own by a a particular processing
 * function. Pointers into a packet are valid only while you own it.
 * */

#if WITH_ZLIB
#if HAVE_ZLIB_H
#include <zlib.h>
#endif
#endif

#if DEBUG_ALLOC
struct lsh_string *
lsh_string_alloc_clue(uint32_t size, const char *clue);

#define lsh_string_alloc(size) \
  (lsh_string_alloc_clue((size), (__FILE__ ":" STRING_LINE)))

void
lsh_string_final_check(void);

unsigned
lsh_get_number_of_strings(void);

#else /* !DEBUG_ALLOC */
struct lsh_string *
lsh_string_alloc(uint32_t size);
#endif /* !DEBUG_ALLOC */

uint32_t
lsh_string_length(const struct lsh_string *s);

const uint8_t *
lsh_string_data(const struct lsh_string *s);

/* Expands to length and data pointer, useful for function calls.
   Unfortunately doesn't work for macros. */
#define STRING_LD(s) lsh_string_length((s)), lsh_string_data((s))

/* Returns an ordinary NUL-terminated string, or NULL if the string
 * contains any NUL-character. */
const char *
lsh_get_cstring(const struct lsh_string *s);

int
lsh_string_eq(const struct lsh_string *a, const struct lsh_string *b);

int
lsh_string_eq_l(const struct lsh_string *a, uint32_t length, const uint8_t *b);

int
lsh_string_prefixp(const struct lsh_string *prefix,
		       const struct lsh_string *s);

uint32_t
lsh_string_sequence_number(const struct lsh_string *s);

void
lsh_string_set_sequence_number(struct lsh_string *s, uint32_t n);


void
lsh_string_putc(struct lsh_string *s, uint32_t i, uint8_t c);

void
lsh_string_set(struct lsh_string *s, uint32_t start, uint32_t length, uint8_t c);

void
lsh_string_write(struct lsh_string *s, uint32_t start, uint32_t length,
		 const uint8_t *data);

void
lsh_string_write_uint32(struct lsh_string *s, uint32_t start, uint32_t n);

void
lsh_string_write_xor(struct lsh_string *s, uint32_t start, uint32_t length,
		     const uint8_t *data);

void
lsh_string_write_string(struct lsh_string *s, uint32_t pos,
			const struct lsh_string *data);

/* Wrapper for nettle_mpz_get_str_256 */
void
lsh_string_write_bignum(struct lsh_string *s, uint32_t start,
			uint32_t legth, const mpz_t n);

/* NOTE: Destructive, returns the string only for convenience. */
struct lsh_string *
lsh_string_trunc(struct lsh_string *s, uint32_t length);


void
lsh_string_crypt(struct lsh_string *dst, uint32_t di,
		 const struct lsh_string *src, uint32_t si,
		 uint32_t length,
		 nettle_crypt_func f, void *ctx);

void
lsh_string_cbc_encrypt(struct lsh_string *dst, uint32_t di,
		       const struct lsh_string *src, uint32_t si,
		       uint32_t length,
		       uint32_t block_size, uint8_t *iv,
		       nettle_crypt_func f, void *ctx);

void
lsh_string_cbc_decrypt(struct lsh_string *dst, uint32_t di,
		       const struct lsh_string *src, uint32_t si,
		       uint32_t length,
		       uint32_t block_size, uint8_t *iv,
		       nettle_crypt_func f, void *ctx);

void
lsh_string_ctr_crypt(struct lsh_string *dst, uint32_t di,
		     const struct lsh_string *src, uint32_t si,
		     uint32_t length,
		     uint32_t block_size, uint8_t *iv,
		     nettle_crypt_func f, void *ctx);

void
lsh_string_write_hash(struct lsh_string *s, uint32_t start,
		      const struct nettle_hash *type, void *ctx);

void
lsh_string_write_hmac(struct lsh_string *s, uint32_t start,
		      const struct nettle_hash *type, uint32_t length,
		      const void *outer, const void *inner, void *state);

void
lsh_string_write_random(struct lsh_string *s, uint32_t start,
			struct randomness *r, uint32_t length);

struct lsh_string *
lsh_string_random(struct randomness *r, uint32_t length);

struct lsh_string *
lsh_string_ntop(int family, uint32_t length, const void *addr);

int
lsh_string_read(struct lsh_string *s, uint32_t start,
		int fd, uint32_t length);

struct lsh_string *
lsh_string_format_sexp(int transport, const char *format, ...);

#if WITH_ZLIB
int
lsh_string_zlib(struct lsh_string *s, uint32_t start,
		int (*f)(z_stream *z, int flush),
		z_stream *z, int flush, uint32_t length);
#endif

/* Base64 decodes a string in place */
int
lsh_string_base64_decode(struct lsh_string *s);

unsigned
lsh_string_base64_encode_update(struct lsh_string *s, uint32_t start,
				struct base64_encode_ctx *ctx,
				uint32_t length, const uint8_t *src);

unsigned
lsh_string_base64_encode_final(struct lsh_string *s, uint32_t start,
			       struct base64_encode_ctx *ctx);

int
lsh_string_transport_iterator_first(struct lsh_string *s,
				    struct sexp_iterator *iterator);

