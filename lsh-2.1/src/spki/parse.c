/* parse.c */

/* libspki
 *
 * Copyright (C) 2002 Niels Möller
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
#include <string.h>

#include <nettle/sexp.h>

#include "parse.h"
#include "tag.h"

/* Automatically generated files */

/* FIXME: Is there any way to get gperf to declare this function
 * static? */
const struct spki_assoc *
spki_gperf (const char *str, unsigned int len);

#include "spki-gperf.h"

/* Make sure that i->type is set to SPKI_TYPE_SYNTAX_ERROR on all
 * failures. */
enum spki_type
spki_parse_fail(struct spki_iterator *i)
{
  i->type = 0;
  return 0;
}

enum spki_type
spki_intern(struct spki_iterator *i)
{  
  if (i->sexp.type == SEXP_ATOM
      && !i->sexp.display)
    {
      const struct spki_assoc *assoc
	= spki_gperf(i->sexp.atom, i->sexp.atom_length);

      if (assoc && sexp_iterator_next(&i->sexp))
	return assoc->id;
    }
  
  return spki_parse_fail(i);;
}

/* NOTE: Uses SPKI_TYPE_SYNTAX_ERROR (= 0) for both unknown types and
 * syntax errors. */
enum spki_type
spki_parse_type(struct spki_iterator *i)
{
  i->start = i->sexp.start;
  switch(i->sexp.type)
    {
    case SEXP_END:
      i->type = SPKI_TYPE_END_OF_EXPR;
      break;

    case SEXP_LIST:
      i->type = (sexp_iterator_enter_list(&i->sexp))
	? spki_intern(i) : 0;
      
      break;

    case SEXP_ATOM:
      i->type = 0;
      break;
    }
  return i->type;
}

enum spki_type
spki_iterator_first(struct spki_iterator *i,
		    unsigned length, const uint8_t *expr)
{
  i->start = 0;
  if (sexp_iterator_first(&i->sexp, length, expr))
    return spki_parse_type(i);

  return spki_parse_fail(i);
}

/* FIXME: Delete this function? */
enum spki_type
spki_iterator_first_sexp(struct spki_iterator *i,
			 const struct sexp_iterator *sexp)
{
  i->start = 0;
  i->sexp = *sexp;

  return spki_parse_type(i);
}

enum spki_type
spki_parse_end(struct spki_iterator *i)
{
  if (i->type && i->sexp.type == SEXP_END
      && sexp_iterator_exit_list(&i->sexp))
    return spki_parse_type(i);
  else
    return spki_parse_fail(i);
}

/* Check the type of an expression, and enter if it the type
 * matches. */
enum spki_type
spki_check_type(struct spki_iterator *i, enum spki_type type)
{
  if (i->type != type)
    return spki_parse_fail(i);

  return spki_parse_type(i);
}

enum spki_type
spki_parse_skip(struct spki_iterator *i)
{
  if (sexp_iterator_exit_list(&i->sexp))
    return spki_parse_type(i);
  else
    return spki_parse_fail(i);
}

const uint8_t *
spki_parse_prevexpr(struct spki_iterator *i,
		    unsigned start, unsigned *length)
{
  assert(start < i->start);
  *length = i->start - start;
  return i->sexp.buffer + start;
}

const uint8_t *
spki_parse_string(struct spki_iterator *i,
		  unsigned *length)
{
  if (i->sexp.type == SEXP_ATOM
      && ! i->sexp.display)
    {
      const uint8_t *contents = i->sexp.atom;
      *length = i->sexp.atom_length;
      
      if (sexp_iterator_next(&i->sexp))
	return contents;
    }
  return NULL;
}

enum spki_type
spki_parse_hash(struct spki_iterator *i,
		struct spki_hash_value *hash)
{
  if (i->type == SPKI_TYPE_HASH
      && (hash->type = spki_intern(i))
      && (hash->digest = spki_parse_string(i, &hash->length)))
    return spki_parse_end(i);
  else
    return spki_parse_fail(i);
}

enum spki_type
spki_parse_principal(struct spki_acl_db *db, struct spki_iterator *i,
		     struct spki_principal **principal)
{  
  switch (i->type)
    {
    default:
      return spki_parse_fail(i);

    case SPKI_TYPE_PUBLIC_KEY:
      {
	unsigned start = i->start;

	unsigned key_length;
	const uint8_t *key;
	
	if (!spki_parse_skip(i))
	  return spki_parse_fail(i);

	key = spki_parse_prevexpr(i, start, &key_length);

	assert(key);
	
	if ( (*principal = spki_principal_by_key(db, key_length, key)) )
	  return i->type;
	else
	  return spki_parse_fail(i);
      }

    case SPKI_TYPE_HASH:
      {
	struct spki_hash_value hash;

	if (spki_parse_hash(i, &hash))
	  {
	    if (hash.type == SPKI_TYPE_MD5
		&& hash.length == MD5_DIGEST_SIZE)
	      *principal = spki_principal_by_md5(db, hash.digest);

	    else if (hash.type == SPKI_TYPE_SHA1
		     && hash.length == SHA1_DIGEST_SIZE)
	      *principal = spki_principal_by_sha1(db, hash.digest);
	    else
	      return spki_parse_fail(i);

	    return i->type;
	  }
	return spki_parse_fail(i);
      }
    } 
}

enum spki_type
spki_parse_subject(struct spki_acl_db *db, struct spki_iterator *i,
		   struct spki_principal **principal)
{
  if (!spki_check_type(i, SPKI_TYPE_SUBJECT)
      || !spki_parse_principal(db, i, principal))
    return spki_parse_fail(i);

  return spki_parse_end(i);
}

enum spki_type
spki_parse_issuer(struct spki_acl_db *db, struct spki_iterator *i,
		  struct spki_principal **principal)
{
  if (!spki_check_type(i, SPKI_TYPE_ISSUER)
      || !spki_parse_principal(db, i, principal))
    return spki_parse_fail(i);

  return spki_parse_end(i);
}

enum spki_type
spki_parse_tag(struct spki_acl_db *db, struct spki_iterator *i,
	       struct spki_tag **tag)
{
  if ( i->type == SPKI_TYPE_TAG
       && (*tag = spki_tag_compile(db->realloc_ctx, db->realloc,
				   &i->sexp)) )
    return spki_parse_end(i);
  else
    return spki_parse_fail(i);
}

enum spki_type
spki_parse_date(struct spki_iterator *i,
		struct spki_date *d)
{
  unsigned date_length;
  const uint8_t *date_string;
  enum spki_type next;
  
  if ((date_string = spki_parse_string(i, &date_length))
      && date_length == SPKI_DATE_SIZE
      && date_string[4] == '-'
      && date_string[7] == '-'
      && date_string[10] == '_'
      && date_string[13] == ':'
      && date_string[16] == ':'
      && (next = spki_parse_end(i))) 
    {
      memcpy(d->date, date_string, SPKI_DATE_SIZE);
      return next;
    }
  return spki_parse_fail(i);
}

enum spki_type
spki_parse_valid(struct spki_iterator *i,
		 struct spki_5_tuple *tuple)
{
  if (!spki_check_type(i, SPKI_TYPE_VALID))
    return spki_parse_fail(i);
  
  if (i->type == SPKI_TYPE_NOT_BEFORE)
    {
      if (spki_parse_date(i, &tuple->not_before))
	tuple->flags |= SPKI_NOT_BEFORE;
    }

  if (i->type == SPKI_TYPE_NOT_AFTER)
    {
      if (spki_parse_date(i, &tuple->not_after))
	tuple->flags |= SPKI_NOT_AFTER;
    }

  /* Online tests not supported. */
  return spki_parse_end(i);  
}

static int
spki_parse_uint32(struct spki_iterator *i, uint32_t *x)
{
  return sexp_iterator_get_uint32(&i->sexp, x);
}

/* Requires that the version number be zero. */
enum spki_type
spki_parse_version(struct spki_iterator *i)
{
  uint32_t version;

  if (i->type == SPKI_TYPE_VERSION
      && spki_parse_uint32(i, &version)
      && version == 0)
    return spki_parse_end(i);
  else
    return spki_parse_fail(i);
}

/* The acl must already be initialized. */
enum spki_type
spki_parse_acl_entry(struct spki_acl_db *db, struct spki_iterator *i,
		     struct spki_5_tuple *acl)
{
  /* Syntax:
   *
   * ("entry" <principal> <delegate>? <tag> <valid>? <comment>?) */

  if (!spki_check_type(i, SPKI_TYPE_ENTRY))
    return spki_parse_fail(i);
  
  /* NOTE: draft-ietf-spki-cert-structure-06.txt has a raw <subj-obj>,
   * but that should be changed. */

  spki_parse_subject(db, i, &acl->subject);

  if (i->type == SPKI_TYPE_PROPAGATE)
    {
      acl->flags |= SPKI_PROPAGATE;
      spki_parse_end(i);
    }

  spki_parse_tag(db, i, &acl->tag);

  if (i->type == SPKI_TYPE_COMMENT)
    spki_parse_skip(i);
      
  if (i->type == SPKI_TYPE_VALID)
    spki_parse_valid(i, acl);

  return spki_parse_end(i);
}

struct spki_5_tuple_list *
spki_parse_acl(struct spki_acl_db *db, struct spki_iterator *i)
{
  struct spki_5_tuple_list *list = NULL;

  if (!spki_check_type(i,  SPKI_TYPE_ACL))
    return 0;

  if (i->type == SPKI_TYPE_VERSION)
    spki_parse_version(i);

  for (;;)
    switch (i->type)
      {
      case SPKI_TYPE_END_OF_EXPR:
	if (spki_parse_end(i))
	  return list;

	/* Fall through */
      default:
      fail:
	spki_5_tuple_list_release(db, list);
	return NULL;
      case SPKI_TYPE_ENTRY:
	{
	  struct spki_5_tuple *acl = spki_5_tuple_cons_new(db, &list);
	  if (!acl)
	    goto fail;

	  if (!spki_parse_acl_entry(db, i, acl))
	    goto fail;
	}
    }
}

/* The cert must already be initialized. */
enum spki_type
spki_parse_cert(struct spki_acl_db *db, struct spki_iterator *i,
		struct spki_5_tuple *cert)
{
  if (!spki_check_type(i, SPKI_TYPE_CERT))
    return spki_parse_fail(i);
  
  if (i->type == SPKI_TYPE_VERSION)
    spki_parse_version(i);

  if (i->type == SPKI_TYPE_DISPLAY)
    spki_parse_skip(i);

  spki_parse_issuer(db, i, &cert->issuer);

  if (i->type == SPKI_TYPE_ISSUER_INFO)
    spki_parse_skip(i);    

  spki_parse_subject(db, i, &cert->subject);
  
  if (i->type == SPKI_TYPE_SUBJECT_INFO)
    spki_parse_skip(i);    

  if (i->type == SPKI_TYPE_PROPAGATE)
    {
      cert->flags |= SPKI_PROPAGATE;
      spki_parse_end(i);
    }

  spki_parse_tag(db, i, &cert->tag);
    
  if (i->type == SPKI_TYPE_VALID)
    spki_parse_valid(i, cert);

  if (i->type == SPKI_TYPE_COMMENT)
    spki_parse_skip(i);    

  return spki_parse_end(i);
}
