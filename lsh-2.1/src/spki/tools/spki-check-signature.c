/* spki-check-signature.c */

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

#include "certificate.h"
#include "parse.h"

#include "getopt.h"
#include "misc.h"


static void
usage(void)
{
  fprintf(stderr, "spki-check-signature [ --no-data ] SIGNATURE\n");
  exit(EXIT_FAILURE);
}

struct check_options
{
  int with_data;
  /* NOTE: We actually use that the argv string is writable. */
  char *signature;
};

static void
parse_options(struct check_options *o,
	      int argc, char **argv)
{
  o->with_data = 1;

  for (;;)
    {
      static const struct option options[] =
	{
	  /* Name, args, flag, val */
	  { "no-data", no_argument, NULL, 'n' },
	  { "version", no_argument, NULL, 'V' },
	  { "help", no_argument, NULL, '?' },
	  { NULL, 0, NULL, 0 }
	};
      int c;
     
      c = getopt_long(argc, argv, "V?", options, NULL);
    
      switch (c)
	{
	default:
	  abort();
	
	case -1:
	  if (optind == argc)
	    die("spki-check-signature: No signature given.\n");

	  o->signature = argv[optind++];
	  if (optind != argc)
	    die("spki-check-signature: Too many arguments.\n");

	  return;

	case 'V':
	  die("spki-check-signature --version not implemented\n");

	case 'n':
	  o->with_data = 0;
	  break;

	case '?':
	  usage();
	}
    }
}

int
main(int argc, char **argv)
{
  struct check_options o;
  
  struct spki_acl_db db;
  struct sexp_iterator sexp;
  struct spki_iterator i;

  struct spki_hash_value hash;
  struct spki_principal *principal;

  parse_options(&o, argc, argv);
  
  spki_acl_init(&db);

  if (sexp_transport_iterator_first(&sexp, strlen(o.signature), o.signature)
      && spki_iterator_first_sexp(&i, &sexp)
      && spki_check_type(&i, SPKI_TYPE_SIGNATURE)
      && spki_parse_hash(&i, &hash)
      && spki_parse_principal(&db, &i, &principal))
  {
    if (o.with_data)
      {
	const struct nettle_hash *hash_algorithm;
	uint8_t *digest;
	
	switch (hash.type)
	  {
	  default:
	    die("Unsupported hash algorithm.");
	  case SPKI_TYPE_MD5:
	    hash_algorithm = &nettle_md5;
	    break;
	  case SPKI_TYPE_SHA1:
	    hash_algorithm = &nettle_sha1;
	    break;
	  }

	if (hash.length != hash_algorithm->digest_size)
	  die("Incorrect hash (bad length)\n");

	digest = hash_file(hash_algorithm, stdin);
	if (!digest)
	  die("Reading stdin failed.\n");

	if (memcmp(digest, hash.digest, hash_algorithm->digest_size))
	  die("Hash value doesn't match input.\n");
      }
    
    if (!spki_verify(NULL, &hash, principal, &i))
      die("Bad signature\n");
    if (!spki_parse_end(&i))
      die("Invalid signature expression\n");
  }
  else
    die("Invalid signature expression\n");

  spki_acl_clear(&db);
  return EXIT_SUCCESS;
}
