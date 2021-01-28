/* rsa.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000 Niels Möller
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

#include <nettle/bignum.h>
#include <nettle/rsa.h>
#include <nettle/sexp.h>
#include <nettle/sha.h>

#include "publickey_crypto.h"

#include "atoms.h"
#include "format.h"
#include "parse.h"
#include "lsh_string.h"
#include "sexp.h"
#include "werror.h"
#include "xalloc.h"

#include "rsa.c.x"

/* We don't allow keys larger than 5000 bits (i.e. 625 octets). Note
 * that allowing really large keys opens for Denial-of-service
 * attacks. */

#define RSA_MAX_OCTETS 625
#define RSA_MAX_BITS (8 * RSA_MAX_OCTETS)

/* GABA:
   (class
     (name rsa_verifier)
     (super verifier)
     (vars
       (key indirect-special "struct rsa_public_key"
            #f rsa_public_key_clear)))
*/

/* GABA:
   (class
     (name rsa_signer)
     (super signer)
     (vars
       (verifier object rsa_verifier)
       (key indirect-special "struct rsa_private_key"
            #f rsa_private_key_clear)))
*/

/* NOTE: For now, always use sha1. */
static int
do_rsa_verify(struct verifier *v,
	      int algorithm,
	      uint32_t length,
	      const uint8_t *msg,
	      uint32_t signature_length,
	      const uint8_t *signature_data)
{
  CAST(rsa_verifier, self, v);
  struct sha1_ctx hash;
  
  mpz_t s;
  int res = 0;

  trace("do_rsa_verify: Verifying %a signature\n", algorithm);
  
  mpz_init(s);
  
  switch(algorithm)
    {
    case ATOM_SSH_RSA:
      {
	struct simple_buffer buffer;
	uint32_t length;
	const uint8_t *digits;
	int atom;
	
	simple_buffer_init(&buffer, signature_length, signature_data);

	if (!(parse_atom(&buffer, &atom)
	      && (atom == ATOM_SSH_RSA)
	      && parse_string(&buffer, &length, &digits)
	      && (length <= self->key.size)
	      && parse_eod(&buffer) ))
	  goto fail;

	nettle_mpz_set_str_256_u(s, length, digits);

	break;
      }
      
      /* It doesn't matter here which flavour of SPKI is used. */
    case ATOM_SPKI_SIGN_RSA:
    case ATOM_SPKI_SIGN_DSS:
    case ATOM_SPKI:
      {
	struct sexp_iterator i;

	if (! (sexp_iterator_first(&i, signature_length, signature_data)
	       && nettle_mpz_set_sexp(s, 8 * self->key.size, &i)))
	  goto fail;

	break;
      }
      
    default:
      fatal("do_rsa_verify: Internal error!\n");
    }

  sha1_init(&hash);
  sha1_update(&hash, length, msg);
  res = rsa_sha1_verify(&self->key, &hash, s);

 fail:
  mpz_clear(s);
  
  return res;
}

static struct lsh_string *
do_rsa_public_key(struct verifier *s)
{
  CAST(rsa_verifier, self, s);

  return ssh_format("%a%n%n", ATOM_SSH_RSA,
		     self->key.e, self->key.n);
}

static struct lsh_string *
do_rsa_public_spki_key(struct verifier *s, int transport)
{
  CAST(rsa_verifier, self, s);

  /* NOTE: The algorithm name "rsa-pkcs1-sha1" is the SPKI standard,
   * and what lsh-1.2 used. "rsa-pkcs1" makes more sense, and is what
   * gnupg uses internally (I think), and was used by some late
   * lsh-1.3.x versions.
   *
   * However, since it doesn't matter much, for now we follow the SPKI
   * standard and stay compatible with lsh-1.2. */
  /* FIXME: Use nettle's rsa_keypair_to_sexp. */
  return lsh_string_format_sexp(transport, "(public-key(rsa-pkcs1-sha1(n%b)(e%b)))",
				self->key.n, self->key.e);
}


/* NOTE: To initialize an rsa verifier, one must
 *
 * 1. Call this function.
 * 2. Initialize the modulo n and exponent e.
 * 3. Call rsa_prepare_public_key.
 */
static void
init_rsa_verifier(struct rsa_verifier *self)
{
  /* FIXME: The allocator could do this kind of initialization
   * automatically. */
  rsa_public_key_init(&self->key);
  
  self->super.verify = do_rsa_verify;
  self->super.public_key = do_rsa_public_key;
  self->super.public_spki_key = do_rsa_public_spki_key;
}

/* Alternative constructor using a key of type ssh-rsa, when the atom
 * "ssh-rsa" is already read from the buffer. */
struct verifier *
parse_ssh_rsa_public(struct simple_buffer *buffer)
{
  NEW(rsa_verifier, res);
  init_rsa_verifier(res);

  if (parse_bignum(buffer, res->key.e, RSA_MAX_OCTETS)
      && (mpz_sgn(res->key.e) == 1)
      && parse_bignum(buffer, res->key.n, RSA_MAX_OCTETS)
      && (mpz_sgn(res->key.n) == 1)
      && (mpz_cmp(res->key.e, res->key.n) < 0)
      && parse_eod(buffer)
      && rsa_public_key_prepare(&res->key))
    return &res->super;

  else
    {
      KILL(res);
      return NULL;
    }
}

/* Signature creation */

static struct lsh_string *
do_rsa_sign(struct signer *s,
	    int algorithm,
	    uint32_t msg_length,
	    const uint8_t *msg)
{
  CAST(rsa_signer, self, s);
  struct lsh_string *res;
  struct sha1_ctx hash;
  mpz_t signature;

  trace("do_rsa_sign: Signing according to %a\n", algorithm);
  
  mpz_init(signature);
  sha1_init(&hash);
  sha1_update(&hash, msg_length, msg);

  rsa_sha1_sign(&self->key, &hash, signature);

  switch (algorithm)
    {
    case ATOM_SSH_RSA:
      /* Uses the encoding:
       *
       * string ssh-rsa
       * string signature-blob
       */
  
      res = ssh_format("%a%un", ATOM_SSH_RSA, signature);
      break;

      /* It doesn't matter here which flavour of SPKI is used. */
    case ATOM_SPKI_SIGN_RSA:
    case ATOM_SPKI_SIGN_DSS:
    case ATOM_SPKI:
      /* FIXME: Add hash algorithm to signature value? */
      res = lsh_string_format_sexp(0, "%b", signature);
      break;
    default:
      fatal("do_rsa_sign: Internal error!\n");
    }
  mpz_clear(signature);
  return res;
}

static struct verifier *
do_rsa_get_verifier(struct signer *s)
{
  CAST(rsa_signer, self, s);

  return &self->verifier->super;
}

static struct verifier *
make_rsa_verifier(struct signature_algorithm *s UNUSED,
		  struct sexp_iterator *i)
{
  NEW(rsa_verifier, res);
  init_rsa_verifier(res);

  if (rsa_keypair_from_sexp_alist(&res->key, NULL, RSA_MAX_BITS, i)
      && rsa_public_key_prepare(&res->key))
    return &res->super;

  KILL(res);
  return NULL;
}


static struct signer *
make_rsa_signer(struct signature_algorithm *s UNUSED,
		struct sexp_iterator *i)
{
  NEW(rsa_verifier, verifier);
  NEW(rsa_signer, res);

  init_rsa_verifier(verifier);
  
  rsa_private_key_init(&res->key);
  if (rsa_keypair_from_sexp_alist(&verifier->key, &res->key, RSA_MAX_BITS, i)
      && rsa_public_key_prepare(&verifier->key)
      && rsa_private_key_prepare(&res->key)
      && res->key.size == verifier->key.size)
    {
      res->verifier = verifier;

      res->super.sign = do_rsa_sign;
      res->super.get_verifier = do_rsa_get_verifier;
      
      return &res->super;
    }
  KILL(res);
  KILL(res->verifier);
  return NULL;
}

struct signature_algorithm rsa_sha1_algorithm =
  { STATIC_HEADER, make_rsa_signer, make_rsa_verifier };

struct verifier *
make_ssh_rsa_verifier(const struct lsh_string *public)
{
  struct simple_buffer buffer;
  int atom;
  
  simple_buffer_init(&buffer, STRING_LD(public));

  return ( (parse_atom(&buffer, &atom)
	    && (atom == ATOM_SSH_RSA))
	   ? parse_ssh_rsa_public(&buffer)
	   : NULL);
}
