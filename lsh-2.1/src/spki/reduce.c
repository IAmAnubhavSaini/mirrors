/* SPKI 5-tuple reduction */

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

#include "certificate.h"
#include "tag.h"


static void
validity_sanity_check(const struct spki_5_tuple *tuple)
{
  /* The flags *should* be redundant. */
  if (!(tuple->flags & SPKI_NOT_BEFORE))
    assert(!SPKI_DATE_CMP(tuple->not_before, spki_date_since_ever));

  if (!(tuple->flags & SPKI_NOT_AFTER))
    assert(!SPKI_DATE_CMP(tuple->not_after, spki_date_for_ever));
}

static int
validity_intersect(struct spki_5_tuple *result,
		   const struct spki_5_tuple *a,
		   const struct spki_5_tuple *b)
{
  validity_sanity_check(a);
  validity_sanity_check(b);

  if (SPKI_DATE_CMP(a->not_before, b->not_after) > 0)
    return 0;

  if (SPKI_DATE_CMP(b->not_before, a->not_after) > 0)
    return 0;
  
  if (result)
    {
      /* Compute and store resulting interval.
       *
       * We want the latest not-before date and the earliest not-after
       * date. */
      if (SPKI_DATE_CMP(a->not_before, b->not_before) > 0)
	result->not_before = a->not_before;
      else
	result->not_before = b->not_before;

      if (SPKI_DATE_CMP(a->not_after, b->not_after) < 0)
	result->not_after = a->not_after;
      else
	result->not_after = b->not_after;

      assert(! (result->flags & (SPKI_NOT_BEFORE | SPKI_NOT_AFTER)));
      
      result->flags |= (SPKI_NOT_BEFORE | SPKI_NOT_AFTER)
	& (a->flags | b->flags);

      validity_sanity_check(result);
    }
  return 1;
}

/* Uses the given list to reduce the given certificate. Resulting
 * reduced certificates are consed onto the list.
 *
 * Returns 0 on out of memory. */
static int
reduce_with_list(struct spki_acl_db *db,
		 struct spki_5_tuple_list **list,
		 const struct spki_5_tuple *tuple)
{
  const struct spki_5_tuple *known;
  const struct spki_5_tuple_list *i;
  assert(tuple->issuer);
  
  for (i = *list;
       (known = spki_5_tuple_by_subject_next(&i, tuple->issuer)); )
    if (known->flags & SPKI_PROPAGATE
	&& validity_intersect(NULL, known, tuple))
      {
	struct spki_tag *tag = spki_tag_intersect(db->realloc_ctx, db->realloc,
						  known->tag, tuple->tag);
	if (tag)
	  {
	    struct spki_5_tuple *result = spki_5_tuple_cons_new(db, list);
	    if (!result)
	      return 0;

	    result->issuer = known->issuer;
	    result->subject = tuple->subject;
	    result->tag = tag;

	    if (!validity_intersect(result, known, tuple))
	      abort();

	    if (tuple->flags & SPKI_PROPAGATE)
	      result->flags |= SPKI_PROPAGATE;
	  }
      }
  return 1;
}

/* Returns a list of reduced 5 tuples.
 *
 * FIXME: How to distinguish out of memory from other failing
 * reductions? */

struct spki_5_tuple_list *
spki_5_tuple_reduce(struct spki_acl_db *db,
		    struct spki_5_tuple_list *sequence)
{
  struct spki_5_tuple_list *result;

  for (result = spki_5_tuple_list_filter(db, db->acls, NULL, NULL);
       sequence; sequence = sequence->cdr)
    {
      assert(sequence->car->issuer);

      if (!reduce_with_list(db, &result, sequence->car))
	{
	  spki_5_tuple_list_release(db, result);
	  return NULL;
	}
    }
  return result;
}
