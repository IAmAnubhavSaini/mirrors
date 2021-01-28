/* lsh-keygen.c
 *
 * Generic key-generation program. Writes a spki-packaged private key
 * on stdout. You would usually pipe this to some other program to
 * extract the public key, encrypt the private key, and save the
 * results in two separate files.
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <nettle/dsa.h>
#include <nettle/rsa.h>

#include "crypto.h"
#include "environ.h"
#include "format.h"
#include "io.h"
#include "lsh_string.h"
#include "publickey_crypto.h"
#include "randomness.h"
#include "sexp.h"
#include "version.h"
#include "werror.h"
#include "xalloc.h"

#include "lsh_argp.h"

#include "lsh-keygen.c.x"

/* Uses a 30-bit public exponetnt for RSA. */
#define E_SIZE 30

/* Option parsing */

const char *argp_program_version
= "lsh-keygen-" VERSION;

const char *argp_program_bug_address = BUG_ADDRESS;

#define OPT_SERVER 0x200

/* GABA:
   (class
     (name lsh_keygen_options)
     (vars
       (server . int)
       ; 'd' means dsa, 'r' rsa
       (algorithm . int)
       (level . int)))
*/

static struct lsh_keygen_options *
make_lsh_keygen_options(void)
{
  NEW(lsh_keygen_options, self);
  self->server = 0;
  self->level = -1;
  self->algorithm = 'r';
  return self;
}

static const struct argp_option
main_options[] =
{
  /* Name, key, arg-name, flags, doc, group */
  { "algorithm", 'a', "Algorithm", 0, "DSA or RSA. "
    "Default is to generate RSA keys", 0 },
  { "server", OPT_SERVER, NULL, 0, "Use the server's seed-file", 0 },
  { "nist-level", 'l', "Security level", 0, "For DSA keys, this is the "
    "NIST security level: Level 0 uses 512-bit primes, level 8 uses "
    "1024 bit primes, and the default is 8. For RSA keys, it's the "
    "bit length of the modulus, and the default is 2048 bits.", 0 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

static const struct argp_child
main_argp_children[] =
{
  { &werror_argp, 0, "", 0 },
  { NULL, 0, NULL, 0}
};

static error_t
main_argp_parser(int key, char *arg, struct argp_state *state)
{
  CAST(lsh_keygen_options, self, state->input);

  switch(key)
    {
    default:
      return ARGP_ERR_UNKNOWN;
    case ARGP_KEY_INIT:
      state->child_inputs[0] = NULL;
      break;

    case ARGP_KEY_END:
      switch (self->algorithm)
	{
	case 'd':
	  if (self->level < 0)
	    self->level = 8;
	  else if (self->level > 8)
	    argp_error(state, "Security level for DSA should be in the range 0-8.");
	  
	  break;
	case 'r':
	  if (self->level < 0)
	    self->level = 2048;
	  else if (self->level < 512)
	    argp_error(state, "RSA keys should be at least 512 bits.");
	  break;
	default:
	  fatal("Internal error!\n");
	}
      break;
	  
    case 'l':
	{
	  char *end;
	  long l = strtol(arg, &end, 0);
	      
	  if (!*arg || *end)
	    {
	      argp_error(state, "Invalid security level.");
	      break;
	    }
	  if (l<0) 
	    {
	      argp_error(state, "Security level can't be negative.");
	      break;
	    }
	  self->level = l;
	  break;
	}

    case 'a':
      if (!strcasecmp(arg, "dsa"))
	self->algorithm = 'd';
      else if (!strcasecmp(arg, "rsa"))
	self->algorithm = 'r';
      else
	argp_error(state, "Unknown algorithm. The supported algorithms are "
		   "RSA and DSA.");
      
      break;
    case OPT_SERVER:
      self->server = 1;
      break;
    }
  return 0;
}

static const struct argp
main_argp =
{ main_options, main_argp_parser, 
  "[-l LEVEL] [-a ALGORITHM]",
  ( "Generates a new private key for the given algorithm and security level.\v"
    "You will usually want to pipe the new key into a program like lsh-writekey, "
    "to split it into its private and public parts, and optionally encrypt "
    "the private information."),
  main_argp_children,
  NULL, NULL
};

static void
progress(void *ctx UNUSED, int c)
{
  char buf[2];
  buf[0] = c; buf[1] = '\0';
  if (c != 'e')
    werror_progress(buf);
}

static struct lsh_string *
dsa_generate_key(struct randomness *r, unsigned level)
{
  struct dsa_public_key public;
  struct dsa_private_key private;
  struct lsh_string *key = NULL;

  dsa_public_key_init(&public);
  dsa_private_key_init(&private);

  assert(r->quality == RANDOM_GOOD);
  
  if (dsa_generate_keypair(&public, &private,
			   r, lsh_random,
			   NULL, progress,
			   512 + 64 * level, DSA_SHA1_Q_BITS))
    {
      key =
	lsh_string_format_sexp(0,
			       "(private-key(dsa(p%b)(q%b)(g%b)(y%b)(x%b)))",
			       public.p, public.q, public.g, public.y,
			       private.x);
    }

  dsa_public_key_clear(&public);
  dsa_private_key_clear(&private);
  return key;
}

static struct lsh_string *
rsa_generate_key(struct randomness *r, uint32_t bits)
{
  struct rsa_public_key public;
  struct rsa_private_key private;
  struct lsh_string *key = NULL;

  rsa_public_key_init(&public);
  rsa_private_key_init(&private);

  assert(r->quality == RANDOM_GOOD);
  
  if (rsa_generate_keypair(&public, &private,
			   r, lsh_random,
			   NULL, progress,
			   bits, E_SIZE))
    {
      /* FIXME: Use rsa-pkcs1 or rsa-pkcs1-sha1? */
      /* FIXME: Some code duplication with
	 rsa.c:do_rsa_public_spki_key */
      key = lsh_string_format_sexp(0, "(private-key(rsa-pkcs1(n%b)(e%b)"
				   "(d%b)(p%b)(q%b)(a%b)(b%b)(c%b)))",
				   public.n, public.e,
				   private.d, private.p, private.q,
				   private.a, private.b, private.c);
    }
  rsa_public_key_clear(&public);
  rsa_private_key_clear(&private);
  return key;
}
int
main(int argc, char **argv)
{
  struct lsh_keygen_options * options
    = make_lsh_keygen_options();

  struct lsh_string *key;
  struct randomness *r;

  const struct exception *e;
  
  argp_parse(&main_argp, argc, argv, 0, NULL, options);

  r = (options->server
       ? make_system_random()
       : make_user_random(getenv(ENV_HOME)));
  
  if (!r)
    {
      werror("Failed to initialize randomness generator.\n");
      return EXIT_FAILURE;
    }

  /* FIXME: Optionally use interactive keyboard input to get some more
   * entropy */
  assert(r->quality == RANDOM_GOOD);
  
  switch (options->algorithm)
    {
    case 'd':
      key = dsa_generate_key(r, options->level);
      break;
    case 'r':
      key = rsa_generate_key(r, options->level);
      break;
    default:
      fatal("Internal error!\n");
    }

  /* Now, output a private key spki structure. */

  e = write_raw(STDOUT_FILENO, STRING_LD(key));

  if (e)
    {
      werror("lsh-keygen: %z\n", e->msg);
      return EXIT_FAILURE;
    }
  
  return EXIT_SUCCESS;
}
