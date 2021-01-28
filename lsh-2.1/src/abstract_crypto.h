/* abstract_crypto.h
 *
 * Interface to block cryptos and hash functions
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

#ifndef LSH_ABSTRACT_CRYPTO_H_INCLUDED
#define LSH_ABSTRACT_CRYPTO_H_INCLUDED

#include <nettle/nettle-meta.h>

#include "list.h"

/* Forward declaration, real declaration in nettle/sexp.h. */
struct sexp_iterator;

#define GABA_DECLARE
#include "abstract_crypto.h.x"
#undef GABA_DECLARE

/* GABA:
   (class
     (name crypto_instance)
     (vars
       (block_size . uint32_t)
       ; Length must be a multiple of the block size.
       ; NOTE: src == dst is allowed, but no other overlaps.
       (crypt method void
              "uint32_t length"
	      "struct lsh_string *dst" "uint32_t di"
	      "const struct lsh_string *src" "uint32_t si")))
*/

#define CRYPT(instance, length, dst, di, src, si) \
((instance)->crypt((instance), (length), (dst), (di), (src), (si)))

#define CRYPTO_ENCRYPT 0
#define CRYPTO_DECRYPT 1

/* GABA:
   (class
     (name crypto_algorithm)
     (vars
       (block_size . uint32_t)
       (key_size . uint32_t)
       (iv_size . uint32_t)
       (make_crypt method (object crypto_instance)
                   "int mode" "const uint8_t *key" "const uint8_t *iv")))
*/

#define MAKE_CRYPT(crypto, mode, key, iv) \
((crypto)->make_crypt((crypto), (mode), (key), (iv)))     

#define MAKE_ENCRYPT(crypto, key, iv) \
     MAKE_CRYPT((crypto), CRYPTO_ENCRYPT, (key), (iv))

#define MAKE_DECRYPT(crypto, key, iv) \
     MAKE_CRYPT((crypto), CRYPTO_DECRYPT, (key), (iv))


/* Hashes. */
/* We have no need for methods here, hashes are sufficiently regular
 * that struct nettle_hash sufficies. Only reason to use methods would
 * be to make it possible to use hashes and macs interchangably, and
 * that doesn't seem terribly useful. */

/* GABA:
   (class
     (name hash_instance)
     (vars
       (type . "const struct nettle_hash *")
       (ctx var-array char)))
*/

/* Happens to work for both hash_instance and hash_algorithm. */
#define HASH_SIZE(h) ((h)->type->digest_size)

/* FIXME: Maybe change these back to macros? */
void
hash_update(struct hash_instance *self,
	    uint32_t length, const uint8_t *data);

/* Returns digest in a newly allocated string. */
struct lsh_string *
hash_digest_string(struct hash_instance *self);

struct hash_instance *
hash_copy(struct hash_instance *self);

/* GABA:
   (class
     (name hash_algorithm)
     (vars
       (type . "const struct nettle_hash *")))
*/

struct hash_instance *
make_hash(const struct hash_algorithm *self);

/* GABA:
   (class
     (name mac_instance)
     (vars
       (mac_size . uint32_t)
       (update method void 
	       "uint32_t length" "const uint8_t *data")
       ; Returns the string, for convenience
       (digest method "struct lsh_string *" "struct lsh_string *res" "uint32_t pos")))
*/

#define MAC_UPDATE(instance, length, data) \
((instance)->update((instance), (length), (data)))

#define MAC_DIGEST(instance, res, pos) \
((instance)->digest((instance), (res), (pos)))

#define MAC_DIGEST_STRING(instance) \
MAC_DIGEST((instance), lsh_string_alloc((instance)->mac_size), 0)

/* GABA:
   (class
     (name mac_algorithm)
     (vars
       (mac_size . uint32_t)
       ; Recommended key size
       (key_size . uint32_t)
       (make_mac method (object mac_instance)
                 "uint32_t length" "const uint8_t *key")))
*/

#define MAKE_MAC(m, l, key) ((m)->make_mac((m), (l), (key)))


/* GABA:
   (class
     (name verifier)
     (vars
       (verify method int
               "int algorithm"
       	       "uint32_t length" "const uint8_t *data"
	       "uint32_t signature_length" "const uint8_t *signature_data")

       (public_key method (string))
       
       ; Returns (public-key (<pub-sig-alg-id> <s-expr>*))
       (public_spki_key method (string) "int transport")))
*/

#define VERIFY(verifier, algorithm, length, data, slength, sdata) \
((verifier)->verify((verifier), (algorithm), (length), (data), (slength), (sdata)))

#define PUBLIC_KEY(verifier) ((verifier)->public_key((verifier)))
#define PUBLIC_SPKI_KEY(verifier, t) ((verifier)->public_spki_key((verifier), (t)))


/* GABA:
   (class
     (name signer)
     (vars
       ; Returns a non-spki signature
       (sign method (string)
             "int algorithm" "uint32_t length" "const uint8_t *data")

       (get_verifier method (object verifier))))
*/

#define SIGN(signer, algorithm, length, data) \
((signer)->sign((signer), (algorithm), (length), (data)))
#define SIGNER_GET_VERIFIER(signer) ((signer)->get_verifier((signer)))


/* GABA:
   (class
     (name signature_algorithm)
     (vars
       ; Iterators should point past the algorithm tag
       (make_signer method (object signer)
                    "struct sexp_iterator *i")
		    
       (make_verifier method (object verifier)
                      "struct sexp_iterator *i")))
*/

#define MAKE_SIGNER(a, i) \
((a)->make_signer((a), (i)))

#define MAKE_VERIFIER(a, i) \
((a)->make_verifier((a), (i)))


/* Simple hashing */
struct lsh_string *
hash_string(const struct hash_algorithm *a,
	    const struct lsh_string *in,
	    int free);

/* Used only by the testsuite */
struct lsh_string *
mac_string(struct mac_algorithm *a,
	   const struct lsh_string *key,
	   int kfree,
	   const struct lsh_string *in,
	   int ifree);

struct lsh_string *
crypt_string(struct crypto_instance *c,
	     const struct lsh_string *in,
	     int free);

struct lsh_string *
crypt_string_pad(struct crypto_instance *c,
		 const struct lsh_string *in,
		 int free);

struct lsh_string *
crypt_string_unpad(struct crypto_instance *c,
		   const struct lsh_string *in,
		   int free);

#endif /* LSH_ABSTRACT_CRYPTO_H_INCLUDED */
