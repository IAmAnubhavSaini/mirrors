/* publickey_crypto.h
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

#ifndef LSH_PUBLICKEY_CRYPTO_H_INCLUDED
#define LSH_PUBLICKEY_CRYPTO_H_INCLUDED

#include "abstract_crypto.h"
#include "parse.h"

#define GABA_DECLARE
#include "publickey_crypto.h.x"
#undef GABA_DECLARE

/* GABA:
   (class
     (name keypair)
     (vars
       ; Atom identifying algorithm type. Needed mostly to know when to invoke the
       ; ssh2 ssh-dss bug-compatibility kludge. 
       (type . int)
       (public string)
       (private object signer)))
*/

struct keypair *
make_keypair(uint32_t type,
	     struct lsh_string *public,
	     struct signer *private);


/* Groups. All group elements are represented by bignums. */

/* GABA:
   (class
     (name zn_group)
     (vars
       (modulo bignum)
       (generator bignum)
       (order bignum)))
*/

int
zn_range(const struct zn_group *G, const mpz_t x);

void
zn_invert(const struct zn_group *G, mpz_t res, const mpz_t x);

void
zn_mul(const struct zn_group *G, mpz_t res, const mpz_t x, const mpz_t y);

void
zn_exp(const struct zn_group *G, mpz_t res, const mpz_t x, const mpz_t e);

void
zn_exp_ui(const struct zn_group *G, mpz_t res, const mpz_t x, uint32_t e);

/* Ring structure needed by SRP */
int
zn_add(const struct zn_group *G,
       mpz_t res, const mpz_t a, const mpz_t b);

int 
zn_sub(const struct zn_group *G,
       mpz_t res, const mpz_t a, const mpz_t b);

const struct zn_group *
make_ssh_group1(void);

const struct zn_group *
make_ssh_group14(void);

const struct zn_group *
make_ssh_ring_srp_1(void);

/* DH key exchange, with authentication */
/* GABA:
   (class
     (name dh_method)
     (vars
       (G const object zn_group)
       (H const object hash_algorithm)
       (random object randomness)))
*/

/* State common for both DH keyechange and SRP, for both client and
 * server. */
/* GABA:
   (struct
     (name dh_instance)
     (vars
       (method const object dh_method)
       (e bignum)       ; Client value
       (f bignum)       ; Server value

       (secret bignum)  ; This side's secret exponent

       ; Currently, K doesn't include any length header.
       (K string)
       (hash object hash_instance)
       (exchange_hash string)))
*/

     
/* Creates client message */
struct lsh_string *
dh_make_client_msg(struct dh_instance *self);

/* Receives client message */
int
dh_process_client_msg(struct dh_instance *self,
		      struct lsh_string *packet);

/* Includes more data to the exchange hash. */
void
dh_hash_update(struct dh_instance *self,
	       struct lsh_string *s, int free);

/* Generates server's secret exponent */
void
dh_make_server_secret(struct dh_instance *self);

/* Creates server message */
struct lsh_string *
dh_make_server_msg(struct dh_instance *self,
		   struct lsh_string *server_key,
		   int hostkey_algorithm,
		   struct signer *s);

/* Decodes server message, but does not verify its signature. */
struct lsh_string *
dh_process_server_msg(struct dh_instance *self,
		      struct lsh_string **signature,
		      struct lsh_string *packet);

/* Verifies server's signature */
int
dh_verify_server_msg(struct dh_instance *self,
		     struct verifier *v);

void
dh_generate_secret(const struct dh_method *self,
		   mpz_t r, mpz_t v);

void
dh_hash_digest(struct dh_instance *self);

struct dh_method *
make_dh(const struct zn_group *G,
	const struct hash_algorithm *H,
	struct randomness *r);

struct dh_method *
make_dh1(struct randomness *r);

struct dh_method *
make_dh14(struct randomness *r);

void
init_dh_instance(const struct dh_method *m,
		 struct dh_instance *self,
		 struct ssh_connection *c);

/* RSA support */
extern struct signature_algorithm rsa_sha1_algorithm;

/* Non spki keys */
struct verifier *
parse_ssh_rsa_public(struct simple_buffer *buffer);

struct verifier *
make_ssh_rsa_verifier(const struct lsh_string *public);


/* DSA signatures */

struct signature_algorithm *
make_dsa_algorithm(struct randomness *random);

/* Non spki keys */
struct verifier *
parse_ssh_dss_public(struct simple_buffer *buffer);

struct verifier *
make_ssh_dss_verifier(const struct lsh_string *public);


#endif /* LSH_PUBLICKEY_CRYPTO_H_INCLUDED */
