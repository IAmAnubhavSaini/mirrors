/* spki.h
 *
 * An implementation of SPKI certificate checking
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2002, Niels Möller
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

#ifndef LSH_SPKI_H_INCLUDED
#define LSH_SPKI_H_INCLUDED

#include <time.h>

#include "alist.h"
#include "publickey_crypto.h"

#include "spki/certificate.h"

#define GABA_DECLARE
# include "spki.h.x"
#undef GABA_DECLARE


struct lsh_string *
make_ssh_hostkey_tag(const char *host);

struct verifier *
spki_make_verifier(struct alist *algorithms,
		   struct sexp_iterator *i,
		   int *algorithm_name);

struct signer *
spki_sexp_to_signer(struct alist *algorithms,
		    struct sexp_iterator *e,
		    int *type);

struct signer *
spki_make_signer(struct alist *algorithms,
		 const struct lsh_string *key,
		 int *algorithm_name);

struct lsh_string *
spki_hash_data(const struct hash_algorithm *algorithm,
	       int algorithm_name,
	       uint32_t length, uint8_t *data);


/* Keeps track of spki_subjects and their keys.
 *
 * We try to make sure that subjects within one context can be
 * compared pointer-wise. I.e. if we get several (public-key ...) and
 * (hash ...) expressions representing the same principal, we merge
 * them into a single spki_subject object. However, there is one case
 * in which the simple method fails: If we encounter several (hash
 * ...) expressions with different hash algorithms, before we
 * encounter the non-hashed (public-key ...) expression. So we must
 * use use spki_principal_normalize when comparing principals. */

/* GABA:
   (class
     (name spki_context)
     (vars
       ;; Available public key algorithms
       
       (algorithms object alist)
       ;; We use the spki_principal struct defined by libskpi,
       ;; and cache verifier objects in its verifier field.
       ;; So we must traverse the list at gc time.
       
       (db indirect-special "struct spki_acl_db"
           do_spki_acl_db_mark do_spki_acl_db_free)))
*/

struct spki_principal *
spki_lookup(struct spki_context *self,
	    unsigned length,
	    const uint8_t *key,
	    /* If non-NULL, use this verifier for
	       the subject. Useful for non-SPKI keys. */
	    struct verifier *v);

int
spki_authorize(struct spki_context *self,
	       const struct spki_principal *subject,
	       time_t t,
	       /* A tag expression */
	       const struct lsh_string *access);

struct spki_context *
make_spki_context(struct alist *algorithms);

int
spki_add_acl(struct spki_context *ctx,
	     struct spki_iterator *i);


struct lsh_string *
spki_pkcs5_encrypt(struct randomness *r,
                   struct lsh_string *label,
		   uint32_t prf_name,
		   struct mac_algorithm *prf,
		   int crypto_name,
		   struct crypto_algorithm *crypto,
		   uint32_t salt_length,
		   struct lsh_string *password,
		   uint32_t iterations,
                   const struct lsh_string *data);

struct lsh_string *
spki_pkcs5_decrypt(struct alist *mac_algorithms,
                   struct alist *crypto_algorithms,
                   struct interact *interact,
                   struct lsh_string *expr);

#endif /* LSH_SPKI_H_INCLUDED */
