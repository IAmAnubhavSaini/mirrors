/* tag.c
 *
 * Operations on SPKI "tags", i.e. authorization descriptions. */

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

#include <nettle/buffer.h>
#include <nettle/sexp.h>

#include "tag.h"


/* Memory allocation */
#define MALLOC(ctx, realloc, size) realloc((ctx), NULL, (size))

/* Cast needed because realloc doesn't like const pointers. */
#define FREE(ctx, realloc, p) realloc((ctx), (void *) (p), 0)

#define NEW(ctx, realloc, type, var) \
type *var = MALLOC(ctx, realloc, sizeof(type))


/* Strings */
struct spki_string
{
  /* FIXME: Do we really need references here? It seems to be enough
   * to referencecount the tags. Then we don't need the string type at
   * all. */
  unsigned refs;
  unsigned length;
  const uint8_t *data;
};

static struct spki_string *
spki_string_new(void *ctx, nettle_realloc_func *realloc,
		unsigned length, const uint8_t *data)
{
  NEW(ctx, realloc, struct spki_string, s);
  uint8_t *p;
  
  if (!s)
    return NULL;

  p = MALLOC(ctx, realloc, length);
  if (!p)
    {
      FREE(ctx, realloc, s);
      return NULL;
    }
  
  memcpy(p, data, length);
  s->refs = 1;
  s->length = length;
  s->data = p;

  return s;
}

static void
spki_string_release(void *ctx, nettle_realloc_func *realloc,
		    struct spki_string *s)
{
  if (!s)
    return;

  if (--s->refs)
    return;

  FREE(ctx, realloc, s->data);
  FREE(ctx, realloc, s);
}

static struct spki_string *
spki_string_dup(struct spki_string *s)
{
  assert(s);
  s->refs++;
  return s;
}

static int
string_equal(struct spki_string *a, struct spki_string *b)
{
  if (a == b)
    return 1;

  if (!a || !b)
    return 0;

  return a->length == b->length
    && !memcmp(a->data, b->data, a->length);
}

/* Returns 1 if s starts with prefix */
static int
string_prefix(struct spki_string *prefix, struct spki_string *s)
{
  assert(prefix);
  assert(s);

  return prefix->length <= s->length
    && !memcmp(prefix->data, s->data, prefix->length);
}

/* Lists */
struct spki_cons
{
  struct spki_tag *car;
  struct spki_cons *cdr;
};

static void
spki_cons_release(void *ctx, nettle_realloc_func *realloc,
		  struct spki_cons *c)
{
  while (c)
    {
      struct spki_cons *cdr = c->cdr;
      spki_tag_release(ctx, realloc, c->car);
      FREE(ctx, realloc, c);
      c = cdr;
    }
}

/* Consumes the reference to CAR. Deallocates both CAR and CDR on
 * failure. */
static struct spki_cons *
spki_cons(void *ctx, nettle_realloc_func *realloc,
	  struct spki_tag *car, struct spki_cons *cdr)
{
  NEW(ctx, realloc, struct spki_cons, c);
  if (!c)
    {
      spki_tag_release(ctx, realloc, car);
      spki_cons_release(ctx, realloc, cdr);
      return NULL;
    }
  c->car = car;
  c->cdr = cdr;

  return c;
}

/* Reverses a list destructively. */
static struct spki_cons *
spki_cons_nreverse(struct spki_cons *c)
{
  struct spki_cons *head = NULL;
  
  while (c)
    {
      struct spki_cons *next = c->cdr;
      
      /* Link current node at head */
      c->cdr = head;
      head = c;

      c = next;
    }

  return head;
}      


/* Tags abstraction */
enum spki_tag_type
  {
    SPKI_TAG_ERROR = 0,
    /* Listed in the order we want to handle tags in
     * spki_tag_intersect. */
    SPKI_TAG_ANY,
    SPKI_TAG_SET,
    SPKI_TAG_LIST,
    SPKI_TAG_PREFIX,
    SPKI_TAG_RANGE,
    SPKI_TAG_ATOM,
  };

struct spki_tag
{
  enum spki_tag_type type;
  unsigned refs;
};

static const struct spki_tag
spki_tag_any = { SPKI_TAG_ANY, 0 };

/* For SPKI_TAG_SET and SPKI_TAG_LIST */
struct spki_tag_list
{
  struct spki_tag super;
  struct spki_cons *children;
};

static struct spki_tag_list *
tag_list(struct spki_tag *tag)
{
  assert(tag->type == SPKI_TAG_LIST
	 || tag->type == SPKI_TAG_SET);

  return (struct spki_tag_list *) tag;
}

/* For SPKI_TAG_ATOM and SPKI_TAG_PREFIX */
struct spki_tag_atom
{
  struct spki_tag super;
  struct spki_string *display;
  struct spki_string *atom;
};

static struct spki_tag_atom *
tag_atom(struct spki_tag *tag)
{
  assert(tag->type == SPKI_TAG_ATOM
	 || tag->type == SPKI_TAG_PREFIX);

  return (struct spki_tag_atom *) tag;
}

enum spki_range_type
  {
    SPKI_RANGE_TYPE_ALPHA,
    SPKI_RANGE_TYPE_NUMERIC,
    SPKI_RANGE_TYPE_TIME,
    SPKI_RANGE_TYPE_BINARY,
    SPKI_RANGE_TYPE_DATE,
    /* Indicates if the upper or lower limit is inclusive. */
    SPKI_RANGE_GTE = 0x10,
    SPKI_RANGE_LTE = 0x20
  };

/* For SPKI_TAG_RANGE */
struct spki_tag_range
{
  struct spki_tag super;
  enum spki_range_type flags;
  
  struct spki_string *display;
  struct spki_string *lower;
  struct spki_string *upper;
};

static struct spki_tag_range *
tag_range(struct spki_tag *tag)
{
  assert(tag->type == SPKI_TAG_RANGE);

  return (struct spki_tag_range *) tag;
}

static void
spki_tag_init(struct spki_tag *tag,
	      enum spki_tag_type type)
{
  tag->type= type;
  tag->refs = 1;
}

static struct spki_tag *
spki_tag_dup(struct spki_tag *tag)
{
  assert(tag);
  if (tag != &spki_tag_any)
    tag->refs++;
  return tag;
}

static struct spki_tag *
spki_tag_atom_alloc(void *ctx, nettle_realloc_func *realloc,
		    enum spki_tag_type type,
		    struct sexp_iterator *i)
{
  struct spki_string *display;
  struct spki_string *atom;

  assert(i->type == SEXP_ATOM);
  assert(i->atom);

  if (i->display)
    {
      display = spki_string_new(ctx, realloc,
				i->display_length, i->display);

      if (!display)
	return NULL;
    }
  else
    display = NULL;
  
  atom = spki_string_new(ctx, realloc,
			 i->atom_length, i->atom);

  if (!atom)
    {
      spki_string_release(ctx, realloc, display);
      return NULL;
    }
  
  if (!sexp_iterator_next(i))
    {
      spki_string_release(ctx, realloc, display);
      spki_string_release(ctx, realloc, atom);
      return NULL;
    }
  
  {
    NEW(ctx, realloc, struct spki_tag_atom, tag);
    if (!tag)
      {
	spki_string_release(ctx, realloc, display);
	spki_string_release(ctx, realloc, atom);
	return NULL;
      }

    spki_tag_init(&tag->super, type);
    tag->display = display;
    tag->atom = atom;
    
    return &tag->super;
  }
}

static struct spki_tag *
spki_tag_list_alloc(void *ctx, nettle_realloc_func *realloc,
		    enum spki_tag_type type,
		    struct spki_cons *children)
{
  NEW(ctx, realloc, struct spki_tag_list, tag);

  assert(type == SPKI_TAG_SET || type == SPKI_TAG_LIST);
  
  if (!tag)
    return NULL;

  spki_tag_init(&tag->super, type);
  tag->children = children;

  return &tag->super;
}

static struct spki_tag *
spki_tag_range_alloc(void *ctx, nettle_realloc_func *realloc,
		     enum spki_range_type flags,
		     struct spki_string *display,
		     struct spki_string *lower,
		     struct spki_string *upper)
{
  NEW(ctx, realloc, struct spki_tag_range, tag);

  if (tag)
    {
      spki_tag_init(&tag->super, SPKI_TAG_RANGE);
      tag->flags = flags;
      tag->display = display;
      tag->lower = lower;
      tag->upper = upper;
    }

  return &tag->super;
}

void
spki_tag_release(void *ctx, nettle_realloc_func *realloc,
		 struct spki_tag *tag)
{
  if (!tag || tag == &spki_tag_any)
    return;

  assert(tag->refs);

  if (--tag->refs)
    return;

  switch (tag->type)
    {
    case SPKI_TAG_ATOM:
    case SPKI_TAG_PREFIX:
      {
	struct spki_tag_atom *self = tag_atom(tag);

	spki_string_release(ctx, realloc, self->display);
	spki_string_release(ctx, realloc, self->atom);

	break;
      }
    case SPKI_TAG_LIST:
    case SPKI_TAG_SET:
      {
	struct spki_tag_list *self = tag_list(tag);
	spki_cons_release(ctx, realloc, self->children);

	break;
      }
    case SPKI_TAG_RANGE:
      {
	struct spki_tag_range *self = tag_range(tag);
	
	spki_string_release(ctx, realloc, self->lower);
	spki_string_release(ctx, realloc, self->upper);
      }
    default:
      abort();
    }

  FREE(ctx, realloc, tag);
}

/* Normalizes set expressions,
 *
 * (* set (* set a b) c) --> (* set a b c)
 *
 * (* set a)             --> a
 *
 * Requires that the children elements passed in are already
 * normalized.
 */

/* FIXME: A destructive function could be more efficient */
static struct spki_tag *
spki_tag_set_new(void *ctx, nettle_realloc_func *realloc,
		 struct spki_cons *c)
{
  struct spki_cons *subsets = NULL;
  struct spki_tag *tag;

  if (c && !c->cdr)
    return spki_tag_dup(c->car);
  
  for (; c; c = c->cdr)
    {
      if (c->car->type != SPKI_TAG_SET)
	{
	  subsets = spki_cons(ctx, realloc, spki_tag_dup(c->car), subsets);
	  if (!subsets)
	    return NULL;
	}
      else
	{
	  struct spki_tag_list *set = tag_list(c->car);
	  struct spki_cons *p;
	  
	  for (p = set->children; p; p = p->cdr)
	    {
	      /* Inner sets must be normalized. */
	      assert (p->car->type != SPKI_TAG_SET);
	      subsets = spki_cons(ctx, realloc, spki_tag_dup(p->car), subsets);
	      if (!subsets)
		return NULL;
	    }
	}
    }
  tag = spki_tag_list_alloc(ctx, realloc, SPKI_TAG_SET,
			    subsets);
  if (tag)
    return tag;

  spki_cons_release(ctx, realloc, subsets);
  return NULL;
}


/* Converting a tag into internal form */

static enum spki_tag_type
spki_tag_classify(struct sexp_iterator *i)
{
  switch (i->type)
    {
    default:
      abort();
      
    case SEXP_END:
      return 0;
      
    case SEXP_ATOM:
      return SPKI_TAG_ATOM;
      
    case SEXP_LIST:
      if (!sexp_iterator_enter_list(i)
	  || i->type != SEXP_ATOM)
	return 0;

      if (!i->display
	  && i->atom_length == 1 && i->atom[0] == '*')
	{
	  enum spki_tag_type type;
	  
	  if (!sexp_iterator_next(i))
	    return 0;

	  if (i->type == SEXP_END)
	    return sexp_iterator_exit_list(i) ? SPKI_TAG_ANY : 0;

	  if (i->type != SEXP_ATOM || i->display)
	    return 0;

#define CASE(x, t)				\
case sizeof("" x) - 1:				\
  if (!memcmp(i->atom, x, sizeof("" x) - 1)) 	\
    { type = t; break; }			\
  return 0

	  switch (i->atom_length)
	    {
	    default:
	      return 0;
	      
	    CASE("set", SPKI_TAG_SET);
	    CASE("range", SPKI_TAG_RANGE);
	    CASE("prefix", SPKI_TAG_PREFIX);
	    }

	  return sexp_iterator_next(i) ? type : 0;
	}
      else
	return SPKI_TAG_LIST;
    }
}

static struct spki_cons *
spki_tag_compile_list(void *ctx, nettle_realloc_func *realloc,
		      struct sexp_iterator *i);

struct spki_tag *
spki_tag_compile(void *ctx, nettle_realloc_func *realloc,
		 struct sexp_iterator *i)
{
  enum spki_tag_type type = spki_tag_classify(i);
  
  switch (type)
    {
    default:
      return NULL;

    case SPKI_TAG_ATOM:
      return spki_tag_atom_alloc(ctx, realloc,
				 SPKI_TAG_ATOM, i);

    case SPKI_TAG_SET:
      {
	struct spki_tag *tag;
	struct spki_cons *children;
	
	/* Empty sets are allowed, but not empty lists. */
	if (i->type == SEXP_END)
	  return spki_tag_set_new(ctx, realloc, NULL);

	children = spki_tag_compile_list(ctx, realloc, i);

	if (!children)
	  return NULL;

	tag = spki_tag_set_new(ctx, realloc, children);
	spki_cons_release(ctx, realloc, children);

	return tag;
      }

    case SPKI_TAG_LIST:
      {
	struct spki_tag *tag;
	
	struct spki_cons *children
	  = spki_tag_compile_list(ctx, realloc, i);

	if (!children)
	  return NULL;
	
	tag = spki_tag_list_alloc(ctx, realloc, type,
				  spki_cons_nreverse(children));

	if (tag)
	  return tag;

	spki_cons_release(ctx, realloc, children);
	return NULL;
      }
      
    case SPKI_TAG_ANY:
      /* Cast to non-const, anybody that tries to modify it should
       * crash. */
      return (struct spki_tag *) &spki_tag_any;
      
    case SPKI_TAG_PREFIX:
      {
	struct spki_tag *tag = spki_tag_atom_alloc(ctx, realloc,
						   SPKI_TAG_PREFIX,
						   i);
	if (!tag)
	  return NULL;

	if (i->type == SEXP_END && sexp_iterator_exit_list(i))
	  return tag;

	spki_tag_release(ctx, realloc, tag);
	return NULL;
      }

    case SPKI_TAG_RANGE:
      /* Not yet implemented. */
      return NULL;
    }

}

/* NOTE: Conses the list up in reverse order. */
static struct spki_cons *
spki_tag_compile_list(void *ctx, nettle_realloc_func *realloc,
		      struct sexp_iterator *i)
{
  struct spki_cons *c = NULL;

  while (i->type != SEXP_END)
    {
      struct spki_tag *tag = spki_tag_compile(ctx, realloc, i);
      struct spki_cons *n;
      
      if (!tag)
	{
	  spki_cons_release(ctx, realloc, c);
	  return NULL;
	}
      n = spki_cons(ctx, realloc, tag, c);
      if (!n)
	/* spki_cons has already released both tag and c */
	return NULL;
	
      c = n;
    }

  if (!sexp_iterator_exit_list(i))
    {
      spki_cons_release(ctx, realloc, c);
      return NULL;
    }
  return c;
}

struct spki_tag *
spki_tag_from_sexp(void *ctx, nettle_realloc_func *realloc,
		   unsigned length,
		   const uint8_t *expr)
{
  struct sexp_iterator i;
  struct spki_tag *tag;
  
  if (!sexp_iterator_first(&i, length, expr))
    return NULL;

  if ((tag = spki_tag_compile(ctx, realloc, &i))
      && i.type == SEXP_END)
    return tag;

  spki_tag_release(ctx, realloc, tag);
  return NULL;
}


/* Tag operations */

static int
atom_prefix(struct spki_tag_atom *a, struct spki_tag_atom *b)
{
  assert(a->super.type == SPKI_TAG_PREFIX);
  assert(b->super.type == SPKI_TAG_ATOM);

  return string_equal(a->display, b->display)
    && string_prefix(a->atom, b->atom);
}

static int
atom_equal(struct spki_tag_atom *a, struct spki_tag_atom *b)
{
  assert(a->super.type == SPKI_TAG_ATOM);
  assert(b->super.type == SPKI_TAG_ATOM);

  return string_equal(a->display, b->display)
    && string_equal(a->atom, b->atom);
}

static int
set_includes(struct spki_cons *set,
	     struct spki_tag *request)
{
  /* The request is included if it's including in any of
   * the delegations in the set. */

  if (request->type == SPKI_TAG_SET)
    {
      /* Check that each of the subsets is included in some of the
       * delegated subsets. This a reasonable approximation, but will
       * result in some false negatives.  */
      struct spki_cons *c;

      for (c = tag_list(request)->children; c; c = c->cdr)
	{
	  if (!set_includes(set, c->car))
	    return 0;
	}

      return 1;
    }

  for (; set; set = set->cdr)
    if (spki_tag_includes(set->car, request))
      return 1;

  return 0;
}

static int
list_includes(struct spki_cons *list,
	      struct spki_tag *request)
{
  /* There may be fewer elements in the delegation list than in the
   * request list. A delegation list implicitly includes any number of
   * (*) forms at the end needed to match all elements in the request
   * form.
   *
   * For example, the delegation (tag (ftp /home/nisse)) includes the
   * request (tag (ftp /home/nisse write)) */

  struct spki_cons *c;
  if (request->type != SPKI_TAG_LIST)
    return 0;
  
  for (c = tag_list(request)->children;
       c && list;
       list = list->cdr, c = c->cdr)
    {
      if (!spki_tag_includes(list->car, c->car))
	return 0;
    }

  /* If we have matched all elements in the delegation list, the
   * request is granted. */
  return (list == NULL);
}

/* Returns true if the requested authorization is included in the
 * delegated one. */
int
spki_tag_includes(struct spki_tag *delegated,
		  struct spki_tag *request)
{
  switch (delegated->type)
    {
    default:
      return 0;

    case SPKI_TAG_ATOM:
      return request->type == SPKI_TAG_ATOM
	&& atom_equal(tag_atom(delegated), tag_atom(request));

    case SPKI_TAG_PREFIX:
      /* Request must have the same display type, and include
       * the delegation as a prefix. */
      return (request->type == SPKI_TAG_ATOM
	      || request->type == SPKI_TAG_PREFIX)
	&& atom_prefix(tag_atom(delegated), tag_atom(request));	
	  
    case SPKI_TAG_LIST:
      return list_includes(tag_list(delegated)->children, request);
      
    case SPKI_TAG_ANY:
      return 1;

    case SPKI_TAG_SET:
      return set_includes(tag_list(delegated)->children, request);

      /* Other star forms not yet implemented. */
    }
}


/* Intersecting tags. */

static struct spki_tag *
set_intersect(void *ctx, nettle_realloc_func *realloc,
	      struct spki_cons *set,
	      struct spki_tag *b)
{
  struct spki_cons *head = NULL;
  
  if (b->type == SPKI_TAG_SET)
    {
      struct spki_cons *children = tag_list(b)->children;
      struct spki_cons *p;

      for (; set; set = set->cdr)
	for (p = children; p; p = p->cdr)
	  {
	    struct spki_tag *tag
	      = spki_tag_intersect(ctx, realloc, set->car, p->car);
	    if (tag)
	      if (!(head = spki_cons(ctx, realloc, tag, head)))
		return NULL;
	  }
    }

  if (!head)
    return NULL;

  return spki_tag_set_new(ctx, realloc, head);
}

static struct spki_tag *
list_intersect(void *ctx, nettle_realloc_func *realloc,
	       struct spki_cons *a,
	       struct spki_cons *b)
{
  struct spki_cons *head = NULL;
  
  for ( ; a && b; a = a->cdr, b = b->cdr)
    {
      struct spki_tag *tag
	= spki_tag_intersect(ctx, realloc, a->car, b->car);
      if (tag)
	if (! (head = spki_cons(ctx, realloc, tag, head)))
	  return NULL;
    }

  assert(head);
  
  /* Add the tail */
  if (!a)
    a = b;

  for ( ; a; a = a->cdr)
    if (! (head = spki_cons(ctx, realloc, spki_tag_dup(a->car), head)))
      return NULL;

  return spki_tag_list_alloc(ctx, realloc,
			     SPKI_TAG_LIST, spki_cons_nreverse(head));
}

static struct spki_tag *
prefix_intersect(struct spki_tag_atom *prefix,
		 struct spki_tag *b)
{
  return ( (b->type == SPKI_TAG_ATOM
	    || b->type == SPKI_TAG_PREFIX)
	   && atom_prefix(prefix, tag_atom(b)))
    ? spki_tag_dup(b) : NULL;
}

struct spki_tag *
spki_tag_intersect(void *ctx, nettle_realloc_func *realloc,
		   struct spki_tag *a,
		   struct spki_tag *b)
{
  /* We want a to have the "widest" type, corresponding to the
   * smallest numerical value of the type. */
  if (a->type > b->type)
    {
      struct spki_tag *t = a;
      a = b;
      b = t;
    }

  switch (a->type)
    {
    default:
      abort();
      
    case SPKI_TAG_ANY:
      return spki_tag_dup(b);
      
    case SPKI_TAG_SET:
      return set_intersect(ctx, realloc,
			   tag_list(a)->children, b);

    case SPKI_TAG_LIST:
      /* A list can never match a string, range or prefix */
      if (b->type != SPKI_TAG_LIST)
	return NULL;
      return list_intersect(ctx, realloc,
			    tag_list(a)->children, tag_list(b)->children);

    case SPKI_TAG_PREFIX:
      return prefix_intersect(tag_atom(a), b);

    case SPKI_TAG_RANGE:
      /* FIXME: Not implemented. */
      return NULL;

    case SPKI_TAG_ATOM:
      assert(b->type == SPKI_TAG_ATOM);
      if (atom_equal(tag_atom(a), tag_atom(b)))
	return spki_tag_dup(b);

      return NULL;
    }
}


/* Formatting a tag as an S-expression. */

static unsigned
list_format(struct spki_cons *c, struct nettle_buffer *buffer)
{
  unsigned done = 0;
  for ( ; c; c = c->cdr)
    {
      unsigned length = spki_tag_format(c->car, buffer);
      if (!length)
	return 0;
      
      done += length;
    }
  
  return sexp_format(buffer, "%)") ? done + 1 : 0;
}

#define L(x) (sizeof(x "") - 1), x

unsigned
spki_tag_format(struct spki_tag *tag, struct nettle_buffer *buffer)
{
  switch(tag->type)
    {
    default:
      abort();
      
    case SPKI_TAG_ANY:
      return sexp_format(buffer, "%l", L("(1:*)"));

    case SPKI_TAG_SET:
      {
	struct spki_tag_list *self = tag_list(tag);
	unsigned length;
	unsigned prefix;

	prefix = sexp_format(buffer, "%l", L("(1:*3:set"));
	if (!prefix)
	  return 0;

	length = list_format(self->children, buffer);

	return length ? length + prefix : 0;
      }

    case SPKI_TAG_LIST:
      {
	struct spki_tag_list *self = tag_list(tag);
	unsigned length;

	if (!sexp_format(buffer, "%("))
	  return 0;

	assert(self->children);
	
	length = list_format(self->children, buffer);

	return length ? length + 1 : 0;
      }

    case SPKI_TAG_PREFIX:
      {
	struct spki_tag_atom *self = tag_atom(tag);
	return sexp_format(buffer, "(%l%t%s)", L("1:*6:prefix"),
			   self->display ? self->display->length : 0,
			   self->display ? self->display->data : NULL,
			   self->atom->length, self->atom->data);
      }
      
    case SPKI_TAG_RANGE:
      /* Not implemented. */
      abort();
      
    case SPKI_TAG_ATOM:
      {
	struct spki_tag_atom *self = tag_atom(tag);
	return sexp_format(buffer, "%t%s",
			   self->display ? self->display->length : 0,
			   self->display ? self->display->data : NULL,
			   self->atom->length, self->atom->data);
      }
    }
}
