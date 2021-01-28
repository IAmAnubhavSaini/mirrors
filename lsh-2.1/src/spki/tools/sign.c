/* sign.c */

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

#include <stdlib.h>

#include <nettle/buffer.h>
#include <nettle/nettle-meta.h>
#include <nettle/rsa.h>

#include "sign.h"

#include "parse.h"


int
spki_sign_init(struct sign_ctx *ctx,
	       struct spki_principal *principal,
	       /* Private key */
	       struct spki_iterator *i)
{
  enum spki_type type;

  if (!spki_check_type(i, SPKI_TYPE_PRIVATE_KEY))
    return 0;
  
  switch ( (type = i->type) )
    {
    case SPKI_TYPE_RSA_PKCS1_MD5:
      ctx->hash_algorithm = &nettle_md5;
      break;

    case SPKI_TYPE_RSA_PKCS1_SHA1:
      ctx->hash_algorithm = &nettle_sha1;
      break;

    default:
      return 0;
    }

  rsa_public_key_init(&ctx->pub);
  rsa_private_key_init(&ctx->priv);

  if (!rsa_keypair_from_sexp_alist(&ctx->pub, &ctx->priv, 0, &i->sexp))
    return 0;

  ctx->type = &spki_type_names[type];
  ctx->principal = principal;
  
  return 1;
}

void
spki_sign_clear(struct sign_ctx *ctx)
{
  rsa_public_key_clear(&ctx->pub);
  rsa_private_key_clear(&ctx->priv);
}

int
spki_sign_digest(struct sign_ctx *ctx,
		 struct nettle_buffer *buffer,
		 const uint8_t *digest)
{
  mpz_t s;
  int res;
  
  res = sexp_format(buffer,
		    "%(signature(hash %0s%s)",
		    ctx->hash_algorithm->name,
		    ctx->hash_algorithm->digest_size, digest);
  if (!res)
    return 0;

  if (ctx->principal)
    res = sexp_format(buffer, "%l",
		      ctx->principal->key_length, ctx->principal->key);
  else
    res = sexp_format(buffer, "(public-key (%s (n%b)(e%b)))",
		      ctx->type->length, ctx->type->name,
		      ctx->pub.n, ctx->pub.e);

  if (!res)
    return 0;
  
  mpz_init(s);

  if (ctx->hash_algorithm == &nettle_md5)
    rsa_md5_sign_digest(&ctx->priv, digest, s);
  else if (ctx->hash_algorithm == &nettle_sha1)
    rsa_sha1_sign_digest(&ctx->priv, digest, s);
  else
    /* Internal error */
    abort();
  		
  res = sexp_format(buffer, "(%s%b)%)",
		    ctx->type->length, ctx->type->name,
		    s);
  
  mpz_clear(s);

  return res;
}

int
spki_sign(struct sign_ctx *ctx,
	  struct nettle_buffer *buffer,
	  void *hash_ctx)
{
  unsigned length = ctx->hash_algorithm->digest_size;
  uint8_t *digest = alloca(length);

  ctx->hash_algorithm->digest(hash_ctx, length, digest);

  return spki_sign_digest(ctx, buffer, digest);
}
