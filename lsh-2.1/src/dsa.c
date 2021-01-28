/* dsa.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels Möller
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

#include <nettle/bignum.h>
#include <nettle/dsa.h>
#include <nettle/sexp.h>
#include <nettle/sha.h>

#include "publickey_crypto.h"

#include "atoms.h"
#include "format.h"
#include "lsh_string.h"
#include "parse.h"
#include "randomness.h"
#include "sexp.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

#include "dsa.c.x" 

/* The standard says that DSA public keys are at most 1024 bits, i.e.
 * 128 octets. We are a little more liberal than that. Note that
 * allowing really large keys opens for Denial-of-service attacks. */

#define DSA_MAX_OCTETS 256
#define DSA_MAX_BITS (8 * DSA_MAX_OCTETS)

/* DSA signatures */

/* GABA:
   (class
     (name dsa_algorithm)
     (super signature_algorithm)
     (vars
       (random object randomness)))
*/

/* GABA:
   (class
     (name dsa_verifier)
     (super verifier)
     (vars
       (key indirect-special "struct dsa_public_key"
            #f dsa_public_key_clear)))
*/

/* GABA:
   (class
     (name dsa_signer)
     (super signer)
     (vars
       (verifier object dsa_verifier)
       (random object randomness)
       (key indirect-special "struct dsa_private_key"
            #f dsa_private_key_clear)))
*/

static int
do_dsa_verify(struct verifier *c, int algorithm,
	      uint32_t length,
	      const uint8_t *msg,
	      uint32_t signature_length,
	      const uint8_t *signature_data)
{
  CAST(dsa_verifier, self, c);
  struct sha1_ctx hash;

  struct simple_buffer buffer;

  int res = 0;

  struct dsa_signature sv;

  trace("do_dsa_verify: Verifying %a signature\n", algorithm);
  dsa_signature_init(&sv);
  
  switch (algorithm)
    {
    case ATOM_SSH_DSS:
      {
	/* NOTE: draft-ietf-secsh-transport-X.txt (x <= 07) uses an extra
	 * length field, which should be removed in the next version. */
	
	uint32_t buf_length;
	const uint8_t *buf;
	int atom;
      
	simple_buffer_init(&buffer, signature_length, signature_data);
	if (!(parse_atom(&buffer, &atom)
	      && (atom == ATOM_SSH_DSS)
	      && parse_string(&buffer, &buf_length, &buf)
	      && buf_length == 2 * DSA_SHA1_Q_OCTETS
	      && parse_eod(&buffer)))
	  goto fail;

	nettle_mpz_set_str_256_u(sv.r, DSA_SHA1_Q_OCTETS, buf);
	nettle_mpz_set_str_256_u(sv.s, DSA_SHA1_Q_OCTETS,
				 buf + DSA_SHA1_Q_OCTETS);

	break;
      }

      /* It doesn't matter here which flavour of SPKI is used. */
    case ATOM_SPKI_SIGN_RSA:
    case ATOM_SPKI_SIGN_DSS:
    case ATOM_SPKI:
      {
	struct sexp_iterator i;
	
	const uint8_t *names[2] = { "r", "s" };
	struct sexp_iterator values[2];
	
	if (! (sexp_iterator_first(&i, signature_length,  signature_data)
	       && sexp_iterator_enter_list(&i)
	       && sexp_iterator_assoc(&i, 2, names, values)
	       && nettle_mpz_set_sexp(sv.r, DSA_SHA1_Q_BITS, &values[0])
	       && nettle_mpz_set_sexp(sv.s, DSA_SHA1_Q_BITS, &values[1])) )
	  goto fail;

	break;
      }
    default:
      fatal("do_dsa_verify: Internal error!\n");
    }

  sha1_init(&hash);
  sha1_update(&hash, length, msg);
  
  res = dsa_sha1_verify(&self->key, &hash, &sv);
 fail:

  dsa_signature_clear(&sv);

  return res;
}


static struct lsh_string *
do_dsa_public_key(struct verifier *s)
{
  CAST(dsa_verifier, self, s);
  return ssh_format("%a%n%n%n%n",
		    ATOM_SSH_DSS,
		    self->key.p, self->key.q,
		    self->key.g, self->key.y);
}

static struct lsh_string *
do_dsa_public_spki_key(struct verifier *s, int transport)
{
  CAST(dsa_verifier, self, s);

  return lsh_string_format_sexp(transport,
				"(%0s(%0s(%0s%b)(%0s%b)(%0s%b)(%0s%b)))",
				"public-key",  "dsa",
				"p", self->key.p,
				"q", self->key.q,
				"g", self->key.g,
				"y", self->key.y);
}

static void
init_dsa_verifier(struct dsa_verifier *self)
{
  /* FIXME: The allocator could do this kind of initialization
   * automatically. */
  dsa_public_key_init(&self->key);

  self->super.verify = do_dsa_verify;
  self->super.public_spki_key = do_dsa_public_spki_key;
  self->super.public_key = do_dsa_public_key;
}


/* Alternative constructor using a key of type ssh-dss, when the atom
 * "ssh-dss" is already read from the buffer. */
struct verifier *
parse_ssh_dss_public(struct simple_buffer *buffer)
{
  NEW(dsa_verifier, res);
  init_dsa_verifier(res);

  if (parse_bignum(buffer, res->key.p, DSA_MAX_OCTETS)
      && (mpz_sgn(res->key.p) == 1)
      && parse_bignum(buffer, res->key.q, DSA_SHA1_Q_OCTETS)
      && (mpz_sgn(res->key.q) == 1)
      && mpz_sizeinbase(res->key.q, 2) == DSA_SHA1_Q_BITS
      && (mpz_cmp(res->key.q, res->key.p) < 0) /* q < p */ 
      && parse_bignum(buffer, res->key.g, DSA_MAX_OCTETS)
      && (mpz_sgn(res->key.g) == 1)
      && (mpz_cmp(res->key.g, res->key.p) < 0) /* g < p */ 
      && parse_bignum(buffer, res->key.y, DSA_MAX_OCTETS) 
      && (mpz_sgn(res->key.y) == 1)
      && (mpz_cmp(res->key.y, res->key.p) < 0) /* y < p */
      && parse_eod(buffer))
    
    return &res->super;

  else
    {
      KILL(res);
      return NULL;
    }
}

  
/* Creating signatures */

static void
dsa_blob_write(struct lsh_string *buf, uint32_t pos,
	       const struct dsa_signature *signature)
{
  lsh_string_write_bignum(buf, pos, DSA_SHA1_Q_OCTETS, signature->r);
  lsh_string_write_bignum(buf, pos + DSA_SHA1_Q_OCTETS, DSA_SHA1_Q_OCTETS,
			  signature->s);
}

static struct lsh_string *
do_dsa_sign(struct signer *c,
	    int algorithm,
	    uint32_t msg_length,
	    const uint8_t *msg)
{
  CAST(dsa_signer, self, c);
  struct dsa_signature sv;
  struct sha1_ctx hash;
  struct lsh_string *signature;

  trace("do_dsa_sign: Signing according to %a\n", algorithm);

  dsa_signature_init(&sv);
  sha1_init(&hash);
  sha1_update(&hash, msg_length, msg);

  if (dsa_sha1_sign(&self->verifier->key, &self->key,
		    self->random, lsh_random, &hash, &sv))
    /* Build signature */
    switch (algorithm)
      {
      case ATOM_SSH_DSS:
	{
	  uint32_t blob_pos;
	
	  /* NOTE: draft-ietf-secsh-transport-X.txt (x <= 07) uses an extra
	   * length field, which should be removed in the next version. */
	  signature = ssh_format("%a%r", ATOM_SSH_DSS,
				 2 * DSA_SHA1_Q_OCTETS, &blob_pos);
	  dsa_blob_write(signature, blob_pos, &sv);

	  break;
	}
	/* It doesn't matter here which flavour of SPKI is used. */
      case ATOM_SPKI_SIGN_RSA:
      case ATOM_SPKI_SIGN_DSS:
      case ATOM_SPKI:
	/* Format: "((1:r20:<r>)(1:s20:<s>))". */
	signature = lsh_string_format_sexp(0, "((r%b)(s%b))",
					   sv.r, sv.s);
	
	break;
      default:
	fatal("do_dsa_sign: Internal error, unexpected algorithm %a.\n",
	      algorithm);
      }
  else
    signature = NULL;

  dsa_signature_clear(&sv);

  return signature;
}

static struct verifier *
do_dsa_get_verifier(struct signer *s)
{
  CAST(dsa_signer, self, s);
  return &self->verifier->super;
}


static struct verifier *
make_dsa_verifier(struct signature_algorithm *self UNUSED,
		  struct sexp_iterator *i)
{
  NEW(dsa_verifier, res);
  init_dsa_verifier(res);

  if (dsa_keypair_from_sexp_alist(&res->key, NULL, DSA_MAX_BITS, DSA_SHA1_Q_BITS, i))
    return &res->super;

  KILL(res);
  return NULL;
}

static struct signer *
make_dsa_signer(struct signature_algorithm *c,
		struct sexp_iterator *i)
{
  CAST(dsa_algorithm, self, c);
  NEW(dsa_verifier, verifier);
  NEW(dsa_signer, res);

  init_dsa_verifier(verifier);
  
  dsa_private_key_init(&res->key);

  if (dsa_keypair_from_sexp_alist(&verifier->key, &res->key, DSA_MAX_BITS, DSA_SHA1_Q_BITS, i))
    {
      res->random = self->random;
      res->verifier = verifier;
      res->super.sign = do_dsa_sign;
      res->super.get_verifier = do_dsa_get_verifier;
      
      return &res->super;
    }

  KILL(res);
  KILL(verifier);
  return NULL;
}

struct signature_algorithm *
make_dsa_algorithm(struct randomness *random)
{
  NEW(dsa_algorithm, dsa);

  dsa->super.make_signer = make_dsa_signer;
  dsa->super.make_verifier = make_dsa_verifier;
  dsa->random = random;

  return &dsa->super;
}


struct verifier *
make_ssh_dss_verifier(const struct lsh_string *public)
{
  struct simple_buffer buffer;
  int atom;
  
  simple_buffer_init(&buffer, STRING_LD(public));

  return ( (parse_atom(&buffer, &atom)
	    && (atom == ATOM_SSH_DSS))
	   ? parse_ssh_dss_public(&buffer)
	   : NULL);
}
