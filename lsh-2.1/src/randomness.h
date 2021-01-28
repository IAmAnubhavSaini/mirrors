/* randomness.h
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

#ifndef LSH_RANDOMNESS_H_INCLUDED
#define LSH_RANDOMNESS_H_INCLUDED

#include "abstract_crypto.h"

#include "exception.h"

enum random_source_type
  {
    /* Trivial data such as timing info and pids. */
    RANDOM_SOURCE_TRIVIA,
    /* Remote end padding. */
    RANDOM_SOURCE_REMOTE,
    /* Data occasionally read from /dev/random or similar. */
    RANDOM_SOURCE_DEVICE,
    /* Data that is secret but not terribly random, such as user
     * passwords or private keys. */
    RANDOM_SOURCE_SECRET,
    /* For reread seed files. */
    RANDOM_SOURCE_NEW_SEED,
    RANDOM_NSOURCES
  };

/* Randomness that is for "pad only" should be used only for iv:s and
 * random padding. */
enum randomness_quality { RANDOM_GOOD, RANDOM_PAD_ONLY };

#define GABA_DECLARE
#include "randomness.h.x"
#undef GABA_DECLARE

/* GABA:
   (class
     (name randomness)
     (vars
       (quality . "enum randomness_quality")
       (random method void "uint32_t length" "uint8_t *dst")
       ; To be used only for SOURCE_REMOTE and SOURCE_SECRET
       (add method void "enum random_source_type"
                        "uint32_t length" "const uint8_t *data")))
*/

#define RANDOM(r, length, dst) ((r)->random((r), (length), (dst)))
#define RANDOM_ADD(r, t, length, data) ((r)->add((r), (t), (length), (dst)))

/* This is not really a constructor, as the randomness collector uses
 * global state. */
struct randomness *
random_init(struct lsh_string *seed_file_name);

/* Creates a more efficient but less secure generator by buffering
 * another generator. */
struct randomness *
make_buffered_random(struct randomness *);

struct randomness *
make_user_random(const char *home);

struct randomness *
make_system_random(void);

/* Randomness function matching nettle's expectations. */
void
lsh_random(void *x, unsigned length, uint8_t *data);

#endif /* LSH_RANDOMNESS_H_INCLUDED */
