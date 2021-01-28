/* certificate.h */

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

#ifndef LIBSPKI_CERTIFICATE_H_INCLUDED
#define LIBSPKI_CERTIFICATE_H_INCLUDED


#include <nettle/md5.h>
#include <nettle/sha.h>
#include <nettle/realloc.h>
#include <nettle/buffer.h>

/* This should be the only file ever including spki-types.h
 * directly. */
#include "spki-types.h"

/* Move this to a separate file? */
struct spki_type_name
{
  unsigned length;
  const uint8_t *name;
};

extern const struct spki_type_name
spki_type_names[];


#include <time.h>

/* Real declarations in parse.h */
struct spki_iterator;
struct spki_hash_value;

/* Real declaration in tag.c */
struct spki_tag;

/* Forward declaration */
struct spki_acl_db;
  
struct spki_hashes
{
  /* Include the flags in this struct? */
  uint8_t md5[MD5_DIGEST_SIZE];
  uint8_t sha1[SHA1_DIGEST_SIZE];
};

enum spki_principal_flags
  {
    SPKI_PRINCIPAL_MD5 = 1,
    SPKI_PRINCIPAL_SHA1 = 2
  };

struct spki_principal
{
  /* Principals linked into a list. */
  struct spki_principal *next;
  
  /* An s-expression */
  unsigned key_length;
  /* NULL if only hash is known */
  uint8_t *key;

  /* A flag is set iff the corresponding hash value is known. */
  enum spki_principal_flags flags;
  struct spki_hashes hashes;

  /* If the lookup code sees hashes and keys in an unfortunate order,
   * it may create several principal structs that represent the same
   * key. In this case, we install an alias pointer when the mistake
   * is discovered. This means that one should call
   * spki_principal_normalize before comparing two principals. */
  
  struct spki_principal *alias;
  
  /* Information needed to verify signatures for this key. For now,
   * details are up to the application. */
  void *verifier;
};

enum spki_5_tuple_flags
{
  SPKI_PROPAGATE = 1,
  /* These redundant flags are kept for convenience. */
  SPKI_NOT_BEFORE = 2,
  SPKI_NOT_AFTER = 4,
};

/* Dates are represented as 19-character strings of the form
 * "1997-07-26_23:15:10". Note that dates can be compared by
 * memcmp. */

#define SPKI_DATE_SIZE 19

struct spki_date {
  uint8_t date[SPKI_DATE_SIZE];
};

extern const struct spki_date spki_date_since_ever;
extern const struct spki_date spki_date_for_ever;

#define SPKI_DATE_CMP(a,b) memcmp((a).date, (b).date, SPKI_DATE_SIZE)

void
spki_date_from_time_t(struct spki_date *d, time_t t);

/* Return value < 0, == 0 or > 0 if d < t, d == t or d > t */ 
int
spki_date_cmp_time_t(struct spki_date *d, time_t t);

/* Lists of 5-tuples is a fundamental type. We use referens counts and
 * cons-cells to keep track of them. */
struct spki_5_tuple
{
  /* This is usually the number of lists that the 5-tuple is on. */
  unsigned refs;
  
  /* NULL for ACL:s */
  struct spki_principal *issuer;
  
  /* For now, support only subjects that are principals (i.e. no
   * names) */
  struct spki_principal *subject;
  enum spki_5_tuple_flags flags;

  /* 00...00 if there's no not-before date */
  struct spki_date not_before;
  /* ff...ff if there's no not-after date */
  struct spki_date not_after;

  /* Tag in internal representation. */
  struct spki_tag *tag;
};

/* Internal function??? */
void
spki_5_tuple_init(struct spki_5_tuple *tuple);

struct spki_5_tuple_list
{
  struct spki_5_tuple *car;
  struct spki_5_tuple_list *cdr;
};

/* Allocates a new tuple, initializes it and conses it onto the list. */
struct spki_5_tuple *
spki_5_tuple_cons_new(struct spki_acl_db *db,
		      struct spki_5_tuple_list **list);

void
spki_5_tuple_list_release(struct spki_acl_db *db,
			  struct spki_5_tuple_list *list);

struct spki_5_tuple_list *
spki_5_tuple_list_nappend(struct spki_5_tuple_list *a,
			  struct spki_5_tuple_list *b);

struct spki_5_tuple_list *
spki_5_tuple_list_nreverse(struct spki_5_tuple_list *l);

typedef int
spki_5_tuple_filter_func(struct spki_acl_db *db,
			 void *ctx, struct spki_5_tuple *tuple);

/* Copies a list (if filter == NULL) or a sublist. */
struct spki_5_tuple_list *
spki_5_tuple_list_filter(struct spki_acl_db *db,
			 struct spki_5_tuple_list *list,
			 void *ctx, spki_5_tuple_filter_func *filter);

const struct spki_5_tuple *
spki_5_tuple_by_subject_next(const struct spki_5_tuple_list **i,
			     const struct spki_principal *subject);

const struct spki_5_tuple *
spki_5_tuple_by_authorization_next(const struct spki_5_tuple_list **i,
				   struct spki_tag *request);


struct spki_acl_db
{
  /* For custom memory allocation. */

  void *realloc_ctx;
  nettle_realloc_func *realloc;

  struct spki_principal *first_principal;
  struct spki_5_tuple_list *acls;
};

void
spki_acl_init(struct spki_acl_db *db);

void
spki_acl_clear(struct spki_acl_db *db);


/* Looks up a principal by key or by hash, and creates new principals
 * when needed. */

struct spki_principal *
spki_principal_by_key(struct spki_acl_db *db,
		      unsigned key_length, const uint8_t *key);

struct spki_principal *
spki_principal_by_md5(struct spki_acl_db *db, const uint8_t *digest);

struct spki_principal *
spki_principal_by_sha1(struct spki_acl_db *db, const uint8_t *digest);

void
spki_principal_free_chain(struct spki_acl_db *db,
			  struct spki_principal *chain);

const struct spki_principal *
spki_principal_normalize(const struct spki_principal *principal);


/* Handling the acl database */

const struct spki_5_tuple *
spki_acl_by_subject_first(struct spki_acl_db *db,
			  const struct spki_5_tuple_list **i,
			  const struct spki_principal *subject);

const struct spki_5_tuple *
spki_acl_by_subject_next(const struct spki_5_tuple_list **i,
			 const struct spki_principal *principal);

const struct spki_5_tuple *
spki_acl_by_authorization_first(struct spki_acl_db *db,
				const struct spki_5_tuple_list **i,
				struct spki_tag *authorization);

const struct spki_5_tuple *
spki_acl_by_authorization_next(const struct spki_5_tuple_list **i,
			       struct spki_tag *authorization);

int
spki_acl_process(struct spki_acl_db *db,
		 struct spki_iterator *i);

unsigned
spki_acl_format(const struct spki_5_tuple_list *list,
		struct nettle_buffer *buffer);


/* Signature verification */

int
spki_hash_verify(const struct spki_hash_value *hash,
		 unsigned length,
		 const uint8_t *data);

typedef int
spki_verify_func(void *ctx,
		 const struct spki_hash_value *hash,
		 struct spki_principal *principal,
		 struct spki_iterator *signature);

spki_verify_func spki_verify;


/* Certificates */

void
spki_tag_free(struct spki_acl_db *db,
	      struct spki_tag *tag);

void
spki_5_tuple_list_release(struct spki_acl_db *db,
			  struct spki_5_tuple_list *list);

int
spki_parse_sequence_no_signatures(struct spki_acl_db *db,
				  struct spki_iterator *i,
				  struct spki_5_tuple_list **list,
				  const struct spki_principal **subject);

int
spki_parse_sequence(struct spki_acl_db *db,
		    struct spki_iterator *i,
		    struct spki_5_tuple_list **list,
		    const struct spki_principal **subject,
		    void *verify_ctx,
		    spki_verify_func *verify);

struct spki_5_tuple_list *
spki_5_tuple_reduce(struct spki_acl_db *db,
		    struct spki_5_tuple_list *sequence);



/* Other more or less internal functions. */

#define SPKI_MALLOC(db, size) ((db)->realloc((db)->realloc_ctx, NULL, (size)))
#define SPKI_FREE(db, p) ((db)->realloc((db)->realloc_ctx, (p), 0))

#define SPKI_NEW(db, type, var) type *var = SPKI_MALLOC((db), sizeof(type))

uint8_t *
spki_dup(struct spki_acl_db *db,
	 unsigned length, const uint8_t *data);

#endif /* LIBSPKI_CERTIFICATE_H_INCLUDED */
