/* spki-make-signature.c */

/* libspki
 *
 * Copyright (C) 2003 Niels Möller
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <nettle/rsa.h>

#include "certificate.h"
#include "parse.h"

#include "getopt.h"
#include "misc.h"
#include "sign.h"


static void
usage(void)
{
  fprintf(stderr,
	  "Usage: spki-make-signature [OPTION...] KEY-FILE.\n\n"
	  "  --digest        Read a digest, instead if a file to be hashed,\n"
	  "                  from stdin.\n"
	  "  --seed-file     A yarrow seed file to use.\n");
  exit(EXIT_FAILURE);
}

struct sign_options
{
  const char *key_file;
  const char *seed_file;
  int digest_mode;
};

static void
parse_options(struct sign_options *o,
	      int argc, char **argv)
{
  o->key_file = NULL;
  o->seed_file = NULL;
  o->digest_mode = 0;
  
  for (;;)
    {
      static const struct option options[] =
	{
	  /* Name, args, flag, val */
	  { "digest", no_argument, NULL, 'd' },
	  { "seed-file", required_argument, NULL, 's' },
	  { "version", no_argument, NULL, 'V' },
	  { "help", no_argument, NULL, '?' },
	  { NULL, 0, NULL, 0 }
	};

      int c;
     
      c = getopt_long(argc, argv, "V?s:w:", options, NULL);
    
      switch (c)
	{
	default:
	  abort();
	
	case -1:
	  if (optind == argc)
	    die("spki-make-signature: No key-file given.\n");

	  o->key_file = argv[optind++];
	  if (optind != argc)
	    die("spki-make-signature: Too many arguments.\n");

	  return;

	case 'V':
	  die("spki-make-signature --version not implemented\n");
	  
	case 's':
	  o->seed_file = optarg;
	  break;

	case 'd':
	  o->digest_mode = 1;
	  break;

	case '?':
	  usage();
	}
    }
}

int
main(int argc, char **argv)
{
  struct sign_options o;
  struct spki_iterator i;
  struct sign_ctx ctx;
  
  char *key;
  unsigned key_length;
  
  char *digest;

  struct nettle_buffer buffer;
  
  parse_options(&o, argc, argv);

  key = read_file_by_name(o.key_file, 0, &key_length);

  if (!key)
    die("Failed to read key-file `%s'\n", o.key_file);

  if (! (spki_transport_iterator_first(&i, key_length, key)
	 && spki_sign_init(&ctx, NULL, &i)))
    die("Invalid private key\n");
  
  if (o.digest_mode)
    {
      unsigned digest_length;
      digest = read_file(stdin,
			 ctx.hash_algorithm->digest_size + 1,
			 &digest_length);
      if (!digest || digest_length != ctx.hash_algorithm->digest_size)
	 die("Unexpected size of input digest.\n");
     }
   else
     {
       digest = hash_file(ctx.hash_algorithm, stdin);

       if (!digest)
	 die("Reading stdin failed.\n");
     }

  nettle_buffer_init_realloc(&buffer, NULL, nettle_xrealloc);
  spki_sign_digest(&ctx, &buffer, digest);

  spki_sign_clear(&ctx);

  if (!write_file(stdout, buffer.size, buffer.contents))
    die("Writing signature failed.\n");

  nettle_buffer_clear(&buffer);

  return EXIT_SUCCESS;
}
