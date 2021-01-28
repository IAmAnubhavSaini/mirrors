/* publickey_crypto.c
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

#include "publickey_crypto.h"

#include "atoms.h"
#include "connection.h"
#include "crypto.h"
#include "format.h"
#include "parse.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "publickey_crypto.h.x"
#undef GABA_DEFINE

struct keypair *
make_keypair(uint32_t type,
	     struct lsh_string *public,
	     struct signer *private)
{
  NEW(keypair, self);
  
  self->type = type;
  self->public = public;
  self->private = private;
  return self;
}

int
zn_range(const struct zn_group *G, const mpz_t x)
{
  return ( (mpz_sgn(x) == 1) && (mpz_cmp(x, G->modulo) < 0) );
}

void
zn_invert(const struct zn_group *G, mpz_t res, const mpz_t x)
{
  if (!mpz_invert(res, x, G->modulo))
    fatal("zn_invert: element is non-invertible\n");

  /* NOTE: In gmp-2, mpz_invert sometimes generates negative inverses. */
  assert (mpz_sgn(res) > 0);
}

void
zn_mul(const struct zn_group *G, mpz_t res, const mpz_t a, const mpz_t b)
{
  mpz_mul(res, a, b);
  mpz_fdiv_r(res, res, G->modulo);
}

void
zn_exp(const struct zn_group *G, mpz_t res, const mpz_t g, const mpz_t e)
{
  mpz_powm(res, g, e, G->modulo);
}

void
zn_exp_ui(const struct zn_group *G, mpz_t res, const mpz_t g, uint32_t e)
{
  mpz_powm_ui(res, g, e, G->modulo);
}

int
zn_add(const struct zn_group *G,
       mpz_t res, const mpz_t a, const mpz_t b)
{
  mpz_add(res, a, b);
  mpz_fdiv_r(res, res, G->modulo);

  return mpz_sgn(res);
}

int 
zn_sub(const struct zn_group *G,
       mpz_t res, const mpz_t a, const mpz_t b)
{
  mpz_sub(res, a, b);
  mpz_fdiv_r(res, res, G->modulo);
  
  return mpz_sgn(res);
}

static struct zn_group *
make_ssh_group(const char *prime, unsigned g, int primitive)
{
  NEW (zn_group, G);

  /* Prime and generator as defined in
   * draft-ietf-secsh-transport-07.txt. */

  mpz_init_set_str(G->modulo, prime, 16);

  mpz_init_set_ui(G->generator, g);
  mpz_init(G->order);
  mpz_sub_ui(G->order, G->modulo, 1);

  if (!primitive)
    mpz_fdiv_q_2exp(G->order, G->order, 1);
  
  return G;
}

/* The group for diffie-hellman-group1-sha1, also "Well known group 2"
   in RFC 2412. */
const struct zn_group *
make_ssh_group1(void)
{
  /* 2^1024 - 2^960 - 1 + 2^64 * { [2^894 pi] + 129093 } */
  return make_ssh_group("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
			"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
			"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
			"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
			"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
			"FFFFFFFFFFFFFFFF", 2, 0);
}

/* The group for diffie-hellman-group14-sha1, also "Well known group
   14" in RFC 3526. */
const struct zn_group *
make_ssh_group14(void)
{
  /* 2^2048 - 2^1984 - 1 + 2^64 * { [2^1918 pi] + 124476 } */
  return make_ssh_group("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
			"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
			"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
			"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
			"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3D"
			"C2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F"
			"83655D23DCA3AD961C62F356208552BB9ED529077096966D"
			"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3B"
			"E39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9"
			"DE2BCBF6955817183995497CEA956AE515D2261898FA0510"
			"15728E5A8AACAA68FFFFFFFFFFFFFFFF", 2, 0);
}

const struct zn_group *
make_ssh_ring_srp_1(void)
{
  /* Same prime as in diffie-hellman-group1-sha1, but use the
     primitive root 5 as the generator. */
  return make_ssh_group("FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
			"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
			"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
			"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
			"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
			"FFFFFFFFFFFFFFFF", 5, 1);
}
