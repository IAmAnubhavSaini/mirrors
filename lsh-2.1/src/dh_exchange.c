/* dh_exchange.c
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

#include "publickey_crypto.h"

#include "connection.h"
#include "crypto.h"
#include "format.h"
#include "lsh_string.h"
#include "randomness.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"

void
init_dh_instance(const struct dh_method *m,
		 struct dh_instance *self,
		 struct ssh_connection *c)
{
  struct lsh_string *s;
  /* FIXME: The allocator could do this kind of initialization
   * automatically. */
  mpz_init(self->e);
  mpz_init(self->f);
  mpz_init(self->secret);

  self->K = NULL;
  
  self->method = m;
  self->hash = make_hash(m->H);
  self->exchange_hash = NULL;

  debug("init_dh_instance\n"
	" V_C: %pS\n", c->versions[CONNECTION_CLIENT]);
  debug(" V_S: %pS\n", c->versions[CONNECTION_SERVER]);
  debug(" I_C: %xS\n", c->literal_kexinits[CONNECTION_CLIENT]);
  debug(" I_S: %xS\n", c->literal_kexinits[CONNECTION_SERVER]);

  s = ssh_format("%S%S%S%S",
		 c->versions[CONNECTION_CLIENT],
		 c->versions[CONNECTION_SERVER],
		 c->literal_kexinits[CONNECTION_CLIENT],
		 c->literal_kexinits[CONNECTION_SERVER]);
  hash_update(self->hash, STRING_LD(s));

  lsh_string_free(s);  
}

struct dh_method *
make_dh(const struct zn_group *G,
	const struct hash_algorithm *H,
	struct randomness *r)
{
  NEW(dh_method, res);

  assert(r->quality == RANDOM_GOOD);
  
  res->G = G;
  res->H = H;
  res->random = r;
  
  return res;
  
}

struct dh_method *
make_dh1(struct randomness *r)
{
  return make_dh(make_ssh_group1(), &crypto_sha1_algorithm, r);
}

struct dh_method *
make_dh14(struct randomness *r)
{
  return make_dh(make_ssh_group14(), &crypto_sha1_algorithm, r);
}

/* R is set to a random, secret, exponent, and V set to is g^r */
void
dh_generate_secret(const struct dh_method *self,
		   mpz_t r, mpz_t v)
{
  mpz_t tmp;

  /* Generate a random number, 1 < x < O(G) = (p-1)/2 */
  mpz_init_set(tmp, self->G->order);  
  mpz_sub_ui(tmp, tmp, 1);
  nettle_mpz_random(r, self->random, lsh_random, tmp);
  mpz_add_ui(r, r, 1);
  mpz_clear(tmp);

  zn_exp(self->G, v, self->G->generator, r);
}

struct lsh_string *
dh_make_client_msg(struct dh_instance *self)
{
  dh_generate_secret(self->method, self->secret, self->e);
  return ssh_format("%c%n", SSH_MSG_KEXDH_INIT, self->e);
}

int
dh_process_client_msg(struct dh_instance *self,
		      struct lsh_string *packet)
{
  struct simple_buffer buffer;
  unsigned msg_number;
  mpz_t tmp;
  
  simple_buffer_init(&buffer, STRING_LD(packet));

  if (! (parse_uint8(&buffer, &msg_number)
	 && (msg_number == SSH_MSG_KEXDH_INIT)
	 && parse_bignum(&buffer, self->e, 0)
	 && (mpz_cmp_ui(self->e, 1) > 0)
	 && zn_range(self->method->G, self->e)
	 && parse_eod(&buffer) ))
    return 0;

  mpz_init(tmp);
  
  zn_exp(self->method->G, tmp, self->e, self->secret);
  self->K = ssh_format("%ln", tmp);

  mpz_clear(tmp);
  
  return 1;
}

void
dh_hash_update(struct dh_instance *self,
	       struct lsh_string *s, int free)
{
  debug("dh_hash_update: %xS\n", s);
  
  hash_update(self->hash, STRING_LD(s));
  if (free)
    lsh_string_free(s);
}

/* Hashes e, f, and the shared secret key */
void
dh_hash_digest(struct dh_instance *self)
{
  dh_hash_update(self, ssh_format("%n%n%S",
				  self->e, self->f,
				  self->K), 1);
  self->exchange_hash = hash_digest_string(self->hash);

  debug("dh_hash_digest: %xS\n", self->exchange_hash);  
}

void
dh_make_server_secret(struct dh_instance *self)
{
  dh_generate_secret(self->method, self->secret, self->f);
}

struct lsh_string *
dh_make_server_msg(struct dh_instance *self,
		   struct lsh_string *server_key,
		   int hostkey_algorithm,
		   struct signer *s)
{
  dh_hash_update(self, ssh_format("%S", server_key), 1);
  dh_hash_digest(self);

  return ssh_format("%c%S%n%fS",
		    SSH_MSG_KEXDH_REPLY,
		    server_key,
		    self->f, SIGN(s, hostkey_algorithm,
				  lsh_string_length(self->exchange_hash),
				  lsh_string_data(self->exchange_hash)));
}

/* Returns the host key. */
struct lsh_string *
dh_process_server_msg(struct dh_instance *self,
		      struct lsh_string **signature,
		      struct lsh_string *packet)
{
  struct simple_buffer buffer;
  unsigned msg_number;
  mpz_t tmp;

  struct lsh_string *key = NULL;
  struct lsh_string *s = NULL;
  
  simple_buffer_init(&buffer, STRING_LD(packet));

  if (! (parse_uint8(&buffer, &msg_number)
	 && (msg_number == SSH_MSG_KEXDH_REPLY)
	 && (key = parse_string_copy(&buffer))
	 /* FIXME: Pass a more restrictive limit to parse_bignum. */
	 && (parse_bignum(&buffer, self->f, 0))
	 && (mpz_cmp_ui(self->f, 1) > 0)
	 && zn_range(self->method->G, self->f)
	 && (s = parse_string_copy(&buffer))
	 && parse_eod(&buffer)))
    {
      lsh_string_free(key);
      lsh_string_free(s);
      return NULL;
    }

  mpz_init(tmp);
  
  zn_exp(self->method->G, tmp, self->f, self->secret);
  self->K = ssh_format("%ln", tmp);

  mpz_clear(tmp);

  dh_hash_update(self, ssh_format("%S", key), 1);
  dh_hash_digest(self);
    
  *signature = s;
  return key;
}

