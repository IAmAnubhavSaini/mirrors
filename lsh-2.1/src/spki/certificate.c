/* SPKI functions */

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
#include <stdlib.h>
#include <string.h>

#include <nettle/md5.h>
#include <nettle/sha.h>
#include <nettle/sexp.h>

#include "certificate.h"
#include "parse.h"
#include "tag.h"

void
spki_acl_init(struct spki_acl_db *db)
{
  db->realloc_ctx = NULL;
  db->realloc = nettle_realloc;
  db->first_principal = NULL;
  db->acls = NULL;
}

void
spki_acl_clear(struct spki_acl_db *db)
{
  spki_principal_free_chain(db, db->first_principal);
  db->first_principal = NULL;
  spki_5_tuple_list_release(db, db->acls);
  db->acls = NULL;
}

uint8_t *
spki_dup(struct spki_acl_db *db,
	 unsigned length, const uint8_t *data)
{
  uint8_t *n = SPKI_MALLOC(db, length);

  if (n)
    memcpy(n, data, length);

  return n;
}

#define HASH(result, method, length, data)			\
do {								\
  struct method##_ctx ctx;					\
  method##_init(&ctx);						\
  method##_update(&ctx, length, data);				\
  method##_digest(&ctx, sizeof(result->method), result->method);\
} while (0)
  
static void
hash_data(struct spki_hashes *hashes,
	  unsigned length, const uint8_t *data)
{
  HASH(hashes, md5, length, data);
  HASH(hashes, sha1, length, data);
}
#undef HASH

static struct spki_principal *
spki_principal_add_key(struct spki_acl_db *db,
		       unsigned key_length,  const uint8_t *key,
		       const struct spki_hashes *hashes)
{
  SPKI_NEW(db, struct spki_principal, principal);
  if (!principal)
    return NULL;

  principal->alias = NULL;
  principal->verifier = NULL;
  
  if (!(principal->key = spki_dup(db, key_length, key)))
    {
      SPKI_FREE(db, principal);
      return NULL;
    }

  principal->key_length = key_length;

  if (hashes)
    principal->hashes = *hashes;
  else
    hash_data(&principal->hashes, key_length, key);

  principal->flags = SPKI_PRINCIPAL_MD5 | SPKI_PRINCIPAL_SHA1;
  
  principal->next = db->first_principal;
  db->first_principal = principal;
  
  return principal;
}

static struct spki_principal *
spki_principal_add_md5(struct spki_acl_db *db,
		       const uint8_t *md5)
{
  SPKI_NEW(db, struct spki_principal, principal);
  if (!principal)
    return NULL;

  principal->key = NULL;
  principal->alias = NULL;
  principal->verifier = NULL;

  memcpy(principal->hashes.md5, md5, sizeof(principal->hashes.md5));
  principal->flags = SPKI_PRINCIPAL_MD5;
  
  principal->next = db->first_principal;
  db->first_principal = principal;
  
  return principal;
}

static struct spki_principal *
spki_principal_add_sha1(struct spki_acl_db *db,
			const uint8_t *sha1)
{
  SPKI_NEW(db, struct spki_principal, principal);
  if (!principal)
    return NULL;

  principal->key = NULL;
  principal->alias = NULL;
  principal->verifier = NULL;

  memcpy(principal->hashes.sha1, sha1, sizeof(principal->hashes.sha1));
  principal->flags = SPKI_PRINCIPAL_SHA1;
  
  principal->next = db->first_principal;
  db->first_principal = principal;
  
  return principal;
}

#define HASH_MATCH(flags, h1, h2)				\
  (((flags) == SPKI_PRINCIPAL_MD5				\
    && !memcmp((h1).md5, (h2).md5, sizeof((h1).md5)))		\
   || ((flags) == SPKI_PRINCIPAL_SHA1				\
       && !memcmp((h1).sha1, (h2).sha1, sizeof((h1).sha1))))

static void
spki_principal_fix_aliases(struct spki_principal *principal)
{
  struct spki_principal *s;

  for (s = principal->next; s; s = s->next)
    {
      if (s->key || s->alias)
	continue;

      if (HASH_MATCH(s->flags, s->hashes, principal->hashes))
	s->alias = principal;
    }
}

struct spki_principal *
spki_principal_by_key(struct spki_acl_db *db,
		      unsigned key_length, const uint8_t *key)
{
  struct spki_principal *s;
  struct spki_hashes hashes;

  hash_data(&hashes, key_length, key);
  
  for (s = db->first_principal; s; s = s->next)
    {
      if (s->key)
	{
	  /* The key is known */
	  if (s->key_length == key_length
	      && !memcmp(s->key, key, key_length))
	    return s;
	}
      else
	/* Check hashes, exactly one should be present */
	if (HASH_MATCH(s->flags, s->hashes, hashes))
	  {
	    s->key = spki_dup(db, key_length, key);
	    if (!s->key)
	      return NULL;
	    s->key_length = key_length;
	    s->hashes = hashes;
	    s->flags |= (SPKI_PRINCIPAL_MD5 | SPKI_PRINCIPAL_SHA1);

	    spki_principal_fix_aliases(s);
	    return s;
	  }
    }

  /* Add a new entry */
  return spki_principal_add_key(db, key_length, key, &hashes);
}

struct spki_principal *
spki_principal_by_md5(struct spki_acl_db *db, const uint8_t *digest)
{
  struct spki_principal *s;

  for (s = db->first_principal; s; s = s->next)
    if ( (s->flags & SPKI_PRINCIPAL_MD5)
	 && !memcmp(s->hashes.md5, digest, sizeof(s->hashes.md5)))
      return s;

  return spki_principal_add_md5(db, digest);
}

struct spki_principal *
spki_principal_by_sha1(struct spki_acl_db *db, const uint8_t *digest)
{
  struct spki_principal *s;

  for (s = db->first_principal; s; s = s->next)
    if ( (s->flags & SPKI_PRINCIPAL_SHA1)
	 && !memcmp(s->hashes.sha1, digest, sizeof(s->hashes.sha1)))
      return s;
  
  return spki_principal_add_sha1(db, digest);
}

void
spki_principal_free_chain(struct spki_acl_db *db,
			  struct spki_principal *chain)
{
  while(chain)
    {
      struct spki_principal *next = chain->next;

      SPKI_FREE(db, chain->key);
      SPKI_FREE(db, chain);

      chain = next;
    }
}

const struct spki_principal *
spki_principal_normalize(const struct spki_principal *principal)
{
  assert(principal);
  while (principal->alias)
    principal = principal->alias;

  return principal;
}



void
spki_5_tuple_init(struct spki_5_tuple *tuple)
{
  /* NOTE: Created with no references */
  tuple->refs = 0;
  tuple->issuer = NULL;
  tuple->subject = NULL;
  tuple->flags = 0;
  tuple->tag = NULL;

  tuple->not_before = spki_date_since_ever;
  tuple->not_after = spki_date_for_ever;
}

/* FIXME: It's somewhat unfortunate to have both spki_tag_release and
 * spki_tag_free. The first function is a lower leve one. */
void
spki_tag_free(struct spki_acl_db *db,
	      struct spki_tag *tag)
{
  spki_tag_release(db->realloc_ctx, db->realloc, tag);
}

struct spki_5_tuple *
spki_5_tuple_cons_new(struct spki_acl_db *db,
		      struct spki_5_tuple_list **list)
{
  SPKI_NEW(db, struct spki_5_tuple, tuple);
  if (tuple)
    {
      SPKI_NEW(db, struct spki_5_tuple_list, cons);
      if (cons)
	{
	  spki_5_tuple_init(tuple);
	  tuple->refs++;
	  cons->car = tuple;
	  cons->cdr = *list;
	  *list = cons;
	  return tuple;
	}
      else
	SPKI_FREE(db, tuple);
    }
  return NULL;
}

void
spki_5_tuple_list_release(struct spki_acl_db *db,
			  struct spki_5_tuple_list *list)
{
  while (list)
    {
      struct spki_5_tuple_list *cdr = list->cdr;
      struct spki_5_tuple *car = list->car;

      assert(car->refs);

      if (!--car->refs)
	{
	  spki_tag_free(db, car->tag);
	  SPKI_FREE(db, car);
	}
      SPKI_FREE(db, list);

      list = cdr;
    }
}

struct spki_5_tuple_list *
spki_5_tuple_list_nappend(struct spki_5_tuple_list *a,
			  struct spki_5_tuple_list *b)
{  
  if (!a)
    return b;

  if (!b)
    return a;

  {
    struct spki_5_tuple_list *p;
    
    for (p = a; p->cdr; p = p->cdr)
      ;
    
    assert(p);
    assert(!p->cdr);
    
    p->cdr = b;
  }
  return a;
}

/* NOTE: More or less a copy of tag.c:spki_cons_nreverse */
struct spki_5_tuple_list *
spki_5_tuple_list_nreverse(struct spki_5_tuple_list *c)
{
  struct spki_5_tuple_list *head = NULL;
  
  while (c)
    {
      struct spki_5_tuple_list *next = c->cdr;
      
      /* Link current node at head */
      c->cdr = head;
      head = c;

      c = next;
    }

  return head;
}

/* Copies a list (if filter == NULL) or a sublist. */
/* FIXME: Use one function for plain copying, and another for
   destructive filtering. */
struct spki_5_tuple_list *
spki_5_tuple_list_filter(struct spki_acl_db *db,
			 struct spki_5_tuple_list *list,
			 void *ctx, spki_5_tuple_filter_func *filter)
{
  struct spki_5_tuple_list *head;
  struct spki_5_tuple_list *tail;
  
  for (head = tail = NULL; list; list = list->cdr)
    {
      if (!filter || filter(db, ctx, list->car))
	{
	  SPKI_NEW(db, struct spki_5_tuple_list, cons);
	  if (!cons)
	    {
	      spki_5_tuple_list_release(db, head);
	      return NULL;
	    }
	  list->car->refs++;
	  
	  cons->car = list->car;
	  cons->cdr = NULL;

	  if (tail)	    
	    tail->cdr = cons;
	  else
	    {
	      assert(!head);
	      head = cons;
	    }
	  tail = cons;
	}
    }
  return head;
}

const struct spki_5_tuple *
spki_5_tuple_by_subject_next(const struct spki_5_tuple_list **i,
			     const struct spki_principal *subject)
{
  const struct spki_5_tuple_list *p;
  subject = spki_principal_normalize(subject);

  assert(!subject->alias);
  
  for (p = *i ; p; p = p->cdr)
    {
      struct spki_5_tuple *tuple = p->car;
      assert(tuple->subject);
      
      if (spki_principal_normalize(tuple->subject) == subject)
	{
	  *i = p->cdr;
	  return tuple;
	}
    }
  return NULL;
}

/* ACL database */

const struct spki_5_tuple *
spki_acl_by_subject_first(struct spki_acl_db *db,
			  const struct spki_5_tuple_list **i,
			  const struct spki_principal *subject)
{
  *i = db->acls;
  return spki_5_tuple_by_subject_next(i, subject);
}

/* FIXME: Delete? Could just as well be a macro. */
const struct spki_5_tuple *
spki_acl_by_subject_next(const struct spki_5_tuple_list **i,
			 const struct spki_principal *subject)
{
  return spki_5_tuple_by_subject_next(i, subject);
}

const struct spki_5_tuple *
spki_5_tuple_by_authorization_next(const struct spki_5_tuple_list **i,
				   struct spki_tag *request)
{
  const struct spki_5_tuple_list *p;
  const struct spki_5_tuple *acl;
  
  for (p = *i; p; p = p->cdr)
    {
      acl = p->car;
      if (spki_tag_includes(acl->tag, request))
	{
	  *i = p->cdr;
	  return acl;
	}
    }
  return NULL;
}

const struct spki_5_tuple *
spki_acl_by_authorization_first(struct spki_acl_db *db,
				const struct spki_5_tuple_list **i,
				struct spki_tag *request)
{
  *i = db->acls;
  return spki_5_tuple_by_authorization_next(i, request);
}

const struct spki_5_tuple *
spki_acl_by_authorization_next(const struct spki_5_tuple_list **i,
			       struct spki_tag *request)
{
  return spki_5_tuple_by_authorization_next(i, request);
}

int
spki_acl_process(struct spki_acl_db *db,
		 struct spki_iterator *i)
{
  struct spki_5_tuple_list *acl = spki_parse_acl(db, i);
  if (!acl)
    return 0;

  db->acls = spki_5_tuple_list_nappend(db->acls, acl);
  return 1;
}

static unsigned
format_valid(const struct spki_5_tuple *tuple,
	     struct nettle_buffer *buffer)
{
  unsigned done = sexp_format(buffer, "%0l", "(5:valid");
  if (!done)
    return 0;

  if (tuple->flags & SPKI_NOT_BEFORE)
    {
      unsigned length = sexp_format(buffer, "(%0s%s)",
				    "not-before",
				    sizeof(tuple->not_before), tuple->not_before);
      if (!length)
	return 0;
      done += length;
    }

  if (tuple->flags & SPKI_NOT_AFTER)
    {
      unsigned length = sexp_format(buffer, "(%0s%s)",
				    "not-after",
				    sizeof(tuple->not_after), tuple->not_after);
      if (!length)
	return 0;
      done += length;
    }
  return sexp_format(buffer, "%l", 1, ")") ? done + 1 : 0;
}

/* Formats an acl from a sequence of 5 tuples. */
unsigned
spki_acl_format(const struct spki_5_tuple_list *list,
		struct nettle_buffer *buffer)
{
  unsigned done = sexp_format(buffer, "%(acl");
  if (!done)
    return 0;

  /* No version field */
  
  for (; list; list = list->cdr)
    {
      unsigned length;
      const struct spki_5_tuple *acl = list->car;
      
      assert(!acl->issuer);
      assert(acl->subject);
      
      length = sexp_format(buffer, "%(entry");
      if (length)
	done += length;
      else
	return 0;

      /* For now, always write the entire key, not a hash. */
      assert(acl->subject->key);

      length = sexp_format(buffer, "%l",
			   acl->subject->key_length, acl->subject->key);
      if (length)
	done += length;
      else
	return 0;

      if (acl->flags & SPKI_PROPAGATE)
	{
	  length = sexp_format(buffer, "(%0s)", "propagate");
	  if (length)
	    done += length;
	  else
	    return 0;
	}

      length = sexp_format(buffer, "%(tag");
      if (length)
	done += length;
      else
	return 0;
      
      length = spki_tag_format(acl->tag, buffer);
      if (length)
	done += length;
      else
	return 0;

      length = sexp_format(buffer, "%)");
      if (length)
	done += length;
      else
	return 0;
      
      if (acl->flags & (SPKI_NOT_BEFORE | SPKI_NOT_AFTER))
	{
	  length = format_valid(acl, buffer);
	  if (length)
	    done += length;
	  else
	    return 0;
	}

      if (sexp_format(buffer, "%)"))
	done++;
      else return 0;
    }
  return sexp_format(buffer, "%)") ? done + 1 : 0;
}


/* Certificates */

#define HASH_CHECK(hash, method, digest_length, data_length, data) 	\
do {									\
  struct method##_ctx ctx;						\
  uint8_t digest[digest_length];					\
  if ((hash)->length != digest_length)					\
    return 0;								\
  method##_init(&ctx);							\
  method##_update(&ctx, length, data);					\
  method##_digest(&ctx, sizeof(digest), digest);			\
									\
  if (memcmp(digest, (hash)->digest, digest_length))			\
    return 0;								\
} while (0)

int
spki_hash_verify(const struct spki_hash_value *hash,
		 unsigned length,
		 const uint8_t *data)
{
  switch(hash->type)
    {
    default:
      return 0;
    case SPKI_TYPE_MD5:
      HASH_CHECK(hash, md5, MD5_DIGEST_SIZE, length, data);
      break;
    case SPKI_TYPE_SHA1:
      HASH_CHECK(hash, sha1, SHA1_DIGEST_SIZE, length, data);
      break;
    }
  return 1;
}
#undef HASH_CHECK

/* Each certificate must be followed by a signature, of the form
 *
 * (signature <hash> <principal> <sig-val>)
 *
 * To verify it, we need to check three things:
 *
 *   1. That the <hash> matches the cetificate expression.
 *   2. That the <principal> equals the issuer of the certificate.
 *   3. That the <hash>, <principal> and <sig-val> forms a valid signature.
 *
 * Step 3 is the only cryptographic application, and we invoke a callback
 * to perform that operation.
 */

static int
parse_sequence(struct spki_acl_db *db,
	       struct spki_iterator *i,
	       struct spki_5_tuple_list **list,	       
	       const struct spki_principal **subject,
	       void *verify_ctx,
	       spki_verify_func *verify)
{
  /* When we process a certificate, we store the information needed
   * to verify the signature that follows it. If NULL, we have no data
   * that need verification. */
  const uint8_t *cert_to_verify = NULL;
  unsigned cert_length;
  struct spki_principal *issuer = NULL;
  
  *list = NULL;
  *subject = NULL;
  
  if (!spki_check_type(i, SPKI_TYPE_SEQUENCE))
    return 0;
  
  for (;;)
    {
      switch (i->type)
	{
	case SPKI_TYPE_END_OF_EXPR:
	  if (spki_parse_end(i) && *subject && !cert_to_verify)
	    {
	      /* FIXME: What's reasonable constness for
	       * spki_principal_normalize? Should we dodge by making
	       * it a makro? */
		 
	      *subject = spki_principal_normalize(*subject);
	      return 1;
	    }
	  
	  /* Fall through */
	default:
	fail:
	  spki_5_tuple_list_release(db, *list);
	  *list = NULL;
	  *subject = NULL;
	  return 0;
	  
	case SPKI_TYPE_CERT:
	  if (cert_to_verify)
	    /* Previous cert not yet verified. */
	    goto fail;
	  {
	    unsigned start = i->start;
	    struct spki_5_tuple *cert = spki_5_tuple_cons_new(db, list);
	    
	    if (!cert)
	      goto fail;

	    if (!spki_parse_cert(db, i, cert))
	      goto fail;

	    assert(cert->subject);
	    assert(cert->issuer);

	    if (verify)
	      {
		cert_to_verify = spki_parse_prevexpr(i, start, &cert_length);
		assert(cert_to_verify);
	      }
	    *subject = cert->subject;
	    issuer = cert->issuer;
	    break;
	  }
	case SPKI_TYPE_PUBLIC_KEY:
	  {
	    /* Just remember key. */
	    unsigned start = i->start;

	    unsigned key_length;
	    const uint8_t *key;

	    if (spki_parse_skip(i))
	      {
		key = spki_parse_prevexpr(i, start, &key_length);
		assert(key);
		*subject = spki_principal_by_key(db, key_length, key);
		if (!*subject)
		  goto fail;
	      }
	    break;
	  }
	case SPKI_TYPE_SIGNATURE:
	  /* NOTE: Allows spurious extra signatures */
	  if (cert_to_verify)
	    {
	      struct spki_hash_value hash;
	      struct spki_principal *principal;

	      assert(issuer);
	      assert(verify);
	      
	      if (spki_parse_type(i) == SPKI_TYPE_HASH
		  && spki_parse_hash(i, &hash)
		  && spki_hash_verify(&hash, cert_length, cert_to_verify)
		  && spki_parse_principal(db, i, &principal)
		  && principal == spki_principal_normalize(issuer)
		  && verify(verify_ctx, &hash, issuer, i))
		{
		  /* Valid signature */
		  cert_to_verify = NULL;
		  break;
		}
	      else
		goto fail;
	    }
	  else
	    /* Fall through */
	case SPKI_TYPE_DO:
	  /* Ignore */
	  spki_parse_skip(i);
	  break;
	}
    }
}

int
spki_parse_sequence(struct spki_acl_db *db,
		    struct spki_iterator *i,
		    struct spki_5_tuple_list **list,
		    const struct spki_principal **subject,
		    void *verify_ctx,
		    spki_verify_func *verify)
{
  assert(verify);
  return parse_sequence(db, i, list, subject, verify_ctx, verify);
}

int
spki_parse_sequence_no_signatures(struct spki_acl_db *db,
				  struct spki_iterator *i,
				  struct spki_5_tuple_list **list,
				  const struct spki_principal **subject)
{
  return parse_sequence(db, i, list, subject, NULL, NULL);
}


/* Dates */

/* MUST have length SPKI_DATE_SIZE */
const struct spki_date spki_date_since_ever =
  { "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
    "\x00\x00\x00\x00\x00\x00\x00\x00\x00" };

const struct spki_date spki_date_for_ever =
  { "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    "\xff\xff\xff\xff\xff\xff\xff\xff\xff" };


static void
write_decimal(unsigned length, uint8_t *buffer, unsigned x)
{
  const unsigned msd[5] = { 0, 1, 10, 100, 1000 };
  unsigned digit;
  
  assert(length <= 4);

  for (digit = msd[length]; digit; digit /= 10)
    {
      /* NOTE: Will generate a bogus digit if x is too large. */
      *buffer++ = '0' + x / digit;
      x %= digit;
    }
}

void
spki_date_from_time_t(struct spki_date *d, time_t t)
{
  struct tm *tm;
#if HAVE_GMTIME_R
  struct tm tm_storage;
  tm = gmtime_r(&t, &tm_storage);
#else
  tm = gmtime(&t);
#endif
  
  if (!tm)
    /* When can gmtime_r fail??? */
    abort();

  d->date[4] = d->date[7] = '-';
  d->date[10] = '_';
  d->date[13] = d->date[16] = ':';
  
  write_decimal(4, d->date,   1900 + tm->tm_year);
  write_decimal(2, d->date +  5, 1 + tm->tm_mon);
  write_decimal(2, d->date +  8,     tm->tm_mday);
  write_decimal(2, d->date + 11,     tm->tm_hour);
  write_decimal(2, d->date + 14,     tm->tm_min);
  write_decimal(2, d->date + 17,     tm->tm_sec);
}

/* Returns < 0, 0 or > 0 if if d < t, d == t or d > t */ 
int
spki_date_cmp_time_t(struct spki_date *d, time_t t)
{
  struct spki_date d2;
  spki_date_from_time_t(&d2, t);
  return memcmp(d, &d2, SPKI_DATE_SIZE);
}
  
