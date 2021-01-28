/* sign.h
 *
 * Helper functions for creating signatures. */

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

#ifndef LIBSPKI_TOOLS_SIGN_H_INCLUDED
#define LIBSPKI_TOOLS_SIGN_H_INCLUDED

#include <nettle/rsa.h>

#include "certificate.h"

struct nettle_buffer;

struct sign_ctx
{
  /* For now, supports only RSA */
  struct rsa_public_key pub;
  struct rsa_private_key priv;

  const struct spki_type_name *type;

  struct spki_principal *principal;
  
  const struct nettle_hash *hash_algorithm;
};

int
spki_sign_init(struct sign_ctx *ctx,
	       /* Public key. If NULL, constructed from private key */
	       struct spki_principal *principal,
	       struct spki_iterator *private);

void
spki_sign_clear(struct sign_ctx *ctx);

int
spki_sign(struct sign_ctx *ctx,
	  struct nettle_buffer *buffer,
	  void *hash_ctx);

int
spki_sign_digest(struct sign_ctx *ctx,
		 struct nettle_buffer *buffer,
		 const uint8_t *digest);

#endif /* LIBSPKI_TOOLS_SIGN_H_INCLUDED */
