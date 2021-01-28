/* dummy.c
 *
 * Dummy implementations of functions referenced by ssh_format,
 * to make it possible to link lshg without nettle and gmp. */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2004 Niels Möller
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

#include <stdlib.h>

#include <nettle/base64.h>
#include <nettle/bignum.h>
#include <nettle/buffer.h>
#include <nettle/cbc.h>
#include <nettle/ctr.h>
#include <nettle/hmac.h>
#include <nettle/memxor.h>
#include <nettle/nettle-meta.h>
#include <nettle/sexp.h>

#include "lsh.h"

/* Referenced by ssh_format.c */
unsigned
nettle_mpz_sizeinbase_256_s(const mpz_t x UNUSED)
{ abort(); }

unsigned
nettle_mpz_sizeinbase_256_u(const mpz_t x UNUSED)
{ abort(); }

void
cbc_encrypt(void *ctx UNUSED, nettle_crypt_func f UNUSED,
	    unsigned block_size UNUSED, uint8_t *iv UNUSED,
	    unsigned length UNUSED, uint8_t *dst UNUSED,
	    const uint8_t *src UNUSED)
{ abort(); }

void
cbc_decrypt(void *ctx UNUSED, nettle_crypt_func f UNUSED,
	    unsigned block_size UNUSED, uint8_t *iv UNUSED,
	    unsigned length UNUSED, uint8_t *dst UNUSED,
	    const uint8_t *src UNUSED)
{ abort(); }

void
ctr_crypt(void *ctx UNUSED, nettle_crypt_func f UNUSED,
	  unsigned block_size UNUSED, uint8_t *iv UNUSED,
	  unsigned length UNUSED, uint8_t *dst UNUSED,
	  const uint8_t *src UNUSED)
{ abort(); }

void
hmac_digest(const void *outer UNUSED, const void *inner UNUSED, void *state UNUSED,
	    const struct nettle_hash *hash UNUSED,
	    unsigned length UNUSED, uint8_t *digest UNUSED)
{ abort(); }

unsigned
sexp_vformat(struct nettle_buffer *buffer UNUSED,
	     const char *format UNUSED, va_list args UNUSED)
{ abort(); }

unsigned
sexp_transport_vformat(struct nettle_buffer *buffer UNUSED,
		       const char *format UNUSED, va_list args UNUSED)
{ abort(); }

int
sexp_transport_iterator_first(struct sexp_iterator *iterator UNUSED,
			      unsigned length UNUSED, uint8_t *input UNUSED)
{ abort(); }

void
nettle_buffer_init_size(struct nettle_buffer *buffer UNUSED,
			unsigned length UNUSED, uint8_t *space UNUSED)
			
{ abort(); }


/* Referenced by lsh_string.c */
uint8_t *
memxor(uint8_t *dst UNUSED, const uint8_t *src UNUSED, size_t n UNUSED)
{ abort(); }

void
nettle_mpz_get_str_256(unsigned length UNUSED, uint8_t *s UNUSED, const mpz_t x UNUSED)
{ abort(); }

void
base64_encode_init(struct base64_encode_ctx *ctx UNUSED)
{ abort(); }

unsigned
base64_encode_update(struct base64_encode_ctx *ctx UNUSED,
		     uint8_t *dst UNUSED,
		     unsigned length UNUSED,
		     const uint8_t *src UNUSED)
{ abort(); }

unsigned
base64_encode_final(struct base64_encode_ctx *ctx UNUSED,
		    uint8_t *dst UNUSED)
{ abort(); }

void
base64_decode_init(struct base64_decode_ctx *ctx UNUSED)
{ abort(); }

int
base64_decode_update(struct base64_decode_ctx *ctx UNUSED,
		     unsigned *dst_length UNUSED,
		     uint8_t *dst UNUSED,
		     unsigned src_length UNUSED,
		     const uint8_t *src UNUSED)
{ abort(); }

int
base64_decode_final(struct base64_decode_ctx *ctx UNUSED)
{ abort(); }

/* Referenced by parse.c */
void
nettle_mpz_set_str_256_s(mpz_t x UNUSED,
			 unsigned length UNUSED, const uint8_t *s UNUSED)
{ abort(); }

/* Referenced by werror.c */
size_t
mpz_sizeinbase (const mpz_t x UNUSED, int base UNUSED)
{ abort(); }

char *
mpz_get_str (char *s UNUSED, int base UNUSED, const mpz_t x UNUSED)
{ abort(); }
