/* spki-delegate.c */

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

#include <assert.h>
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
	  "Usage: spki-delegate ...\n");
  exit(EXIT_FAILURE);
}

struct delegate_options
{
  char *chain;
  char *issuer;
  char *subject;
  char *tag;
  char *key;
  
  int propagate;
  int sign;
};

static void
parse_options(struct delegate_options *o,
	      int argc, char **argv)
{
  o->issuer = NULL;
  o->subject = NULL;
  o->tag = NULL;
  o->chain = NULL;
  o->key = NULL;
  o->propagate = 0;
  o->sign = 1;
  
  for (;;)
    {
      static const struct option options[] =
	{
	  /* Name, args, flag, val */
	  { "issuer", required_argument, NULL, 'i' },
	  { "subject", required_argument, NULL, 's' },
	  { "tag", required_argument, NULL, 't' },
	  { "chain", required_argument, NULL, 'c' },
	  { "key-file", required_argument, NULL, 'k' },

	  { "propagate", no_argument, NULL, 'p' },
	  { "no-sign", no_argument, NULL, 'n' },
	  
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
	    die("spki-delegate: Too many arguments.\n");

	  if (!o->subject)
	    die("spki-delegate: --subject is mandatory.\n");

	  if (!o->tag)
	    die("spki-delegate: --tag is mandatory.\n");

	  return;
	  
	case 'V':
	  die("spki-delegate --version not implemented\n");

	case 'i':
	  o->issuer = optarg;
	  break;

	case 'c':
	  o->chain = optarg;
	  break;
	  
	case 's':
	  o->subject = optarg;
	  break;

	case 't':
	  o->tag = optarg;
	  break;

	case 'k':
	  o->key = optarg;
	  break;

	case 'p':
	  o->propagate = 1;
	  break;

	case 'n':
	  o->sign = 0;
	  break;
	  
	case '?':
	  usage();
	}
    }
}

static const struct spki_principal *
process_principal(struct spki_acl_db *db, char *expr)
{
  struct spki_iterator i;
  struct spki_principal *principal;
  
  /* NOTE: changes expr destructively */
  if (spki_transport_iterator_first(&i, strlen(expr), expr)
      && spki_parse_principal(db, &i, &principal))
    return principal;
  else
    return NULL;
}

static int
process_sequence(struct spki_acl_db *db,
		 struct spki_5_tuple_list **sequence,
		 const struct spki_principal **subject,
		 struct nettle_buffer *buffer,
		 const char *file)
{
  struct spki_iterator i;
  
  unsigned length;
  char *data = read_file_by_name(file, 0, &length);

  if (data
      && spki_transport_iterator_first(&i, length, data))
    {
      unsigned start = i.start;

      if (spki_parse_sequence(db, &i, sequence, subject, NULL, spki_verify))
	{
	  unsigned length;
	  const uint8_t *expr = spki_parse_prevexpr(&i, start, &length);
	  assert(expr);
	  assert(length);

	  /* Copy the expression, except for the final closing
	   * parenthesis. */

	  sexp_format(buffer, "%l", length - 1, expr);
	  free(data);

	  return 1;
	}
    }

  free(data);
  return 0;
}

static int
process_tag(struct nettle_buffer *buffer, char *expr)
{
  struct spki_iterator i;

  if (spki_transport_iterator_first(&i, strlen(expr), expr) == SPKI_TYPE_TAG)
    {
      unsigned start = i.start;
      const char *tag;
      unsigned tag_length;
      if (spki_parse_skip(&i)
	  && (tag = spki_parse_prevexpr(&i, start, &tag_length)))
	{
	  sexp_format(buffer, "%l", tag_length, tag);
	  return 1;
	}
    }
  return 0;  
}

int
main(int argc, char **argv)
{
  struct delegate_options o;
  struct spki_acl_db db;
  
  struct nettle_buffer cert_buffer;
  struct nettle_buffer buffer;

  const struct spki_principal *issuer;
  const struct spki_principal *subject;
  
  parse_options(&o, argc, argv);

  spki_acl_init(&db);
  nettle_buffer_init_realloc(&cert_buffer, NULL, nettle_xrealloc);
  nettle_buffer_init_realloc(&buffer, NULL, nettle_xrealloc);

  if (o.chain)
    {
      struct spki_5_tuple_list *sequence;

      if (!process_sequence(&db, &sequence, &issuer, &buffer, o.chain))
	die("Invalid input certificate.\n");
      assert(issuer);      
    }
  else if (o.issuer)
    {
      if (! (issuer = process_principal(&db, o.issuer)))
	die("spki-delegate: Invalid issuer.\n");

      /* Start a new sequence */
      sexp_format(&buffer, "%(sequence%l",
		  cert_buffer.size, cert_buffer.contents);
    }
  else
    {
      /* We create an acl. */
      issuer = NULL;
      sexp_format(&buffer, "%(acl");
    }
  
  if (! (subject = process_principal(&db, o.subject)))
    die("spki-delegate: Invalid subject.\n");

  if (issuer)
    sexp_format(&cert_buffer, "%(cert(issuer%l)(subject%l)",
		issuer->key_length, issuer->key,
		subject->key_length, subject->key);
  else
    sexp_format(&cert_buffer, "%(entry(subject%l)",
		subject->key_length, subject->key);
  
  if (o.propagate)
    sexp_format(&cert_buffer, "(propagate)");

  if (!process_tag(&cert_buffer, o.tag))
    die("spki-delegate: Invalid tag.\n");

  sexp_format(&cert_buffer, "%)");
  sexp_format(&buffer, "%l",
	      cert_buffer.size, cert_buffer.contents);
  
  if (issuer && o.sign)
    {
      struct spki_iterator i;
      struct sign_ctx ctx;
      void *hash_ctx;
      
      char *key;
      unsigned key_length;

      if (!o.key)
	die("Automatic key search not yet implemented.\n");

      key = read_file_by_name(o.key, 0, &key_length);
      if (!key)
	die("Reading key file `%s' failed.\n", o.key);
	
      if (! (spki_transport_iterator_first(&i, key_length, key)
	     && spki_sign_init(&ctx, NULL, &i)))
	die("Invalid private key\n");

      hash_ctx = alloca(ctx.hash_algorithm->context_size);
      ctx.hash_algorithm->init(hash_ctx);
      ctx.hash_algorithm->update(hash_ctx, 
				 cert_buffer.size, cert_buffer.contents);

      spki_sign(&ctx, &buffer, hash_ctx);
      spki_sign_clear(&ctx);
    }

  nettle_buffer_clear(&cert_buffer);
  
  sexp_format(&buffer, "%)");

  if (!write_file(stdout,
		  buffer.size, buffer.contents))
    die("Writing signature failed.\n");
  
  nettle_buffer_clear(&buffer);

  return EXIT_SUCCESS;
}

