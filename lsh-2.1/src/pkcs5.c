/* pkcs5.c
 *
 * PKCS#5 "PBKDF2" style key derivation.
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

#include <nettle/memxor.h>

#include "crypto.h"

#include "lsh_string.h"
#include "xalloc.h"

/* NOTE: The PKCS#5 v2 spec doesn't recommend or specify any
 * particular value of the iteration count.
 *
 * To get a feeling for reasonable ranges, I've done some benchmarking
 * on my system, an old SparcStation 4 with a bogomips rating of about
 * 110. For testing, I'm using SHA1-HMAC with the password = "gazonk",
 * salt = "pepper", and I generate a 32 octet key, suitable for
 * triple-DES CBC.
 *
 * Measured timings:
 *
 *    Iterations    Elapsed time (seconds)
 *             1            0.02
 *            10            0.02
 *           100            0.06
 *          1000            0.35
 *        10 000            3.27
 *       100 000           33.36
 *      1000 000          330.84
 *
 * What is reasonable?
 *
 * With 1000 iterations, key derivation is still doable (and probably
 * almost bearable) on slow machines. A real slow i386 could be 50
 * times slower than my machine, and 1000 iterations might take 20
 * seconds.
 *
 * On the other hand, the key derivation involved in a small
 * dictionary attack trying 10000 passwords would take about an hour
 * on my machine. And perhaps only a few minutes on a modern office
 * machine.
 *
 * Based on this, it seems that something between 1000 and 10000 is a
 * reasonable number of iterations (today, May 2000). */

struct lsh_string *
pkcs5_derive_key(struct mac_algorithm *prf,
		 const struct lsh_string *password,
		 const struct lsh_string *salt,
		 uint32_t iterations,
		 uint32_t key_length)
{
  struct mac_instance *m = MAKE_MAC(prf,
				    lsh_string_length(password),
				    lsh_string_data(password));
  struct lsh_string *key = lsh_string_alloc(key_length);
  uint32_t left = key_length;
  uint32_t pos = 0;
  
  /* Set up the block counter buffer. This will never have more than
   * the last few bits set (using sha1, 8 bits = 5100 bytes of key) so
   * we only change the last byte. */

  uint8_t block_count[4] = { 0, 0, 0, 1 }; 

  struct lsh_string *digest = lsh_string_alloc(prf->mac_size);
  struct lsh_string *buffer = lsh_string_alloc(prf->mac_size);

  assert(iterations);
  assert(key_length <= 255 * prf->mac_size);
  
  for (;; block_count[3]++)
    {
      uint32_t i;
      assert(block_count[3]);
      
      /* First iterate */
      MAC_UPDATE(m, lsh_string_length(salt), lsh_string_data(salt));
      MAC_UPDATE(m, 4, block_count);
      MAC_DIGEST(m, buffer, 0);

      for (i = 1; i < iterations; i++)
	{
	  MAC_UPDATE(m, prf->mac_size, lsh_string_data(buffer));
	  MAC_DIGEST(m, digest, 0);
	  lsh_string_write_xor(buffer, 0, STRING_LD(digest));
	}

      assert(pos + left == key_length);
      
      if (left <= prf->mac_size)
	{
	  lsh_string_write(key, pos, left, lsh_string_data(buffer));
	  break;
	}
      else
	{
	  lsh_string_write_string(key, pos, buffer);
	  pos += prf->mac_size;
	  left -= prf->mac_size;
	}
    }
  KILL(m);
  lsh_string_free(digest);
  lsh_string_free(buffer);

  return key;
}
