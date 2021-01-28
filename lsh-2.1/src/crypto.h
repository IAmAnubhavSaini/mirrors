/* crypto.h
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

#ifndef LSH_CRYPTO_H_INCLUDED
#define LSH_CRYPTO_H_INCLUDED

#include "abstract_crypto.h"

extern struct crypto_algorithm crypto_aes256_cbc_algorithm;
extern struct crypto_algorithm crypto_aes256_ctr_algorithm;
extern struct crypto_algorithm crypto_arcfour_algorithm;
extern struct crypto_algorithm crypto_blowfish_cbc_algorithm;
extern struct crypto_algorithm crypto_cast128_cbc_algorithm;
extern struct crypto_algorithm crypto_des3_cbc_algorithm;
extern struct crypto_algorithm crypto_serpent256_cbc_algorithm;
extern struct crypto_algorithm crypto_twofish256_cbc_algorithm;

extern const struct hash_algorithm crypto_sha1_algorithm;
extern const struct hash_algorithm crypto_md5_algorithm;

struct mac_algorithm *
make_hmac_algorithm(const struct hash_algorithm *h);

/* 10 million iterations would take 5 hours on my machine */
#define PKCS5_MAX_ITERATIONS 10000000

struct lsh_string *
pkcs5_derive_key(struct mac_algorithm *prf,
		 const struct lsh_string *password,
		 const struct lsh_string *salt,
		 uint32_t iterations,
		 uint32_t key_length);

#endif /* LSH_CRYPTO_H_INCLUDED */
