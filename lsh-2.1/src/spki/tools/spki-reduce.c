/* spki-reduce.c */

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

#include "certificate.h"
#include "parse.h"

#include "getopt.h"
#include "misc.h"

static void
usage(void)
{
  fprintf(stderr,
	  "Usage: spki-reduce ...\n");
  exit(EXIT_FAILURE);
}

struct reduce_options
{
  const char *acls;
  const char *tag;
};

static void
parse_options(struct reduce_options *o,
	      int argc, char **argv)
{
  o->acls = NULL;
  o->tag = NULL;
  
  for (;;)
    {
      static const struct option options[] =
	{
	  /* Name, args, flag, val */
	  { "acl-file", required_argument, NULL, 'a' },
	  { "tag", required_argument, NULL, 't' },

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
	  if (optind != argc)
	    die("spki-reduce: Too many arguments.\n");

	  if (!o->acls)
	    die("spki-reduce: --acl-file is mandatory.\n");

	  return;
	  
	case 'V':
	  die("spki-delegate --version not implemented\n");

	case 'a':
	  o->acls = optarg;
	  break;

	case 't':
	  o->tag = optarg;
	  break;

	case '?':
	  usage();
	}
    }
}

static int
process_acls(struct spki_acl_db *db, const char *acls)
{
  struct spki_iterator i;
  int res;
  
  unsigned length;
  char *data = read_file_by_name(acls, 0, &length);

  res = (data
	 && spki_transport_iterator_first(&i, length, data)
	 && spki_acl_process(db, &i));

  free(data);

  return res;
}

static int
process_sequence(struct spki_acl_db *db,
		 struct spki_5_tuple_list **sequence,
		 const struct spki_principal **subject,
		 FILE *f)
{
  struct spki_iterator i;
  int res = 0;
  
  unsigned length;
  char *data = read_file(f, 0, &length);

  if (data
      && spki_transport_iterator_first(&i, length, data))
    res = spki_parse_sequence(db, &i, sequence, subject, NULL, spki_verify);

  free(data);

  return res;
}

static int
filter(struct spki_acl_db *db UNUSED,
       void *ctx, struct spki_5_tuple *tuple)
{
  const struct spki_principal *subject = (struct spki_principal *) ctx;
  return (!tuple->issuer
	  && subject == spki_principal_normalize(tuple->subject));
}

int
main(int argc, char **argv)
{
  struct reduce_options o;
  struct spki_acl_db db;
  struct spki_5_tuple_list *sequence;
  struct spki_5_tuple_list *reduced;
  const struct spki_principal *subject;

  struct nettle_buffer buffer;
  
  parse_options(&o, argc, argv);

  spki_acl_init(&db);

  if (!process_acls(&db, o.acls))
    die("Invalid ACL list.\n");
     
  if (!process_sequence(&db, &sequence, &subject, stdin))
    die("Invalid certificate.\n");

  reduced = spki_5_tuple_reduce(&db, sequence);

  /* FIXME: Leaks a list. */
  reduced = spki_5_tuple_list_filter(&db, reduced,
				     (void *) subject, filter);

  if (!reduced)
    die("No authorization.\n");

  nettle_buffer_init_realloc(&buffer, NULL, nettle_xrealloc);
  
  spki_acl_format(reduced, &buffer);

  if (!write_file(stdout, buffer.size, buffer.contents))
    die("Writing signature failed.\n");

  nettle_buffer_clear(&buffer);

  return EXIT_SUCCESS;
}
