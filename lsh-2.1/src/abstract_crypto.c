/* abstract_crypto.c
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
# include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include "abstract_crypto.h"

#include "crypto.h"
#include "format.h"
#include "lsh_string.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "abstract_crypto.h.x"
#undef GABA_DEFINE


struct lsh_string *
hash_string(const struct hash_algorithm *a,
	    const struct lsh_string *in,
	    int free)
{
  struct hash_instance *hash = make_hash(a);
  struct lsh_string *out;

  hash_update(hash, STRING_LD(in));
  out = hash_digest_string(hash);

  KILL(hash);
  if (free)
    lsh_string_free(in);

  return out;
}

struct lsh_string *
mac_string(struct mac_algorithm *a,
	   const struct lsh_string *key,
	   int kfree,
	   const struct lsh_string *in,
	   int ifree)
{
  struct lsh_string *out;
  struct mac_instance *mac
    = MAKE_MAC(a, lsh_string_length(key), lsh_string_data(key));

  MAC_UPDATE(mac, lsh_string_length(in), lsh_string_data(in));
  out = MAC_DIGEST_STRING(mac);

  KILL(mac);
  
  if (kfree)
    lsh_string_free(key);
  if (ifree)
    lsh_string_free(in);

  return out;
}

struct lsh_string *
crypt_string(struct crypto_instance *c,
	     const struct lsh_string *in,
	     int free)
{
  struct lsh_string *out;
  uint32_t length = lsh_string_length(in);
  
  if (c->block_size && (length % c->block_size))
    return NULL;

  if (free)
    {
      /* Do the encryption in place. The type cast is permissible
       * because we're conceptually freeing the string and reusing the
       * storage. */
      out = (struct lsh_string *) in;
    }
  else
    /* Allocate fresh storage. */
    out = lsh_string_alloc(length);
  
  CRYPT(c, length, out, 0, in, 0);
  
  return out;
}

/* FIXME: Missing testcases. This is only used for encrypted private
 * keys */
struct lsh_string *
crypt_string_pad(struct crypto_instance *c,
		 const struct lsh_string *in,
		 int free)
{
  struct lsh_string *s;
  uint32_t length = lsh_string_length(in);
  uint32_t pos;
  uint32_t pad = c->block_size - (length % c->block_size);
  
  assert(pad);
  
  s = ssh_format(free ? "%lfS%lr" : "%lS%lr", in, pad, &pos);
  /* Use RFC 1423 and "generalized RFC 1423" as described in
   * PKCS#5 version 2. */
  lsh_string_set(s, pos, pad, pad);

  return crypt_string(c, s, 1);
}

struct lsh_string *
crypt_string_unpad(struct crypto_instance *c,
		   const struct lsh_string *in,
		   int free)
{
  struct lsh_string *out;
  uint32_t pad;
  uint32_t length = lsh_string_length(in);
  assert(length);
  
  out = crypt_string(c, in, free);
  if (!out)
    return NULL;

  length = lsh_string_length(out);
  pad = lsh_string_data(out)[length - 1];

  if ( (pad > 0) && (pad <= c->block_size) )
    {
      /* This should not happen, as crypt string
       * must return a multiple of the block size. */
      assert(pad <= length);

      lsh_string_trunc(out, length - pad);
      return out;
    }
  else
    {
      lsh_string_free(out);
      return NULL;
    }
}
