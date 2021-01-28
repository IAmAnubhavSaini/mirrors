/* xalloc.h
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

#ifndef LSH_XALLOC_H_INCLUDED
#define LSH_XALLOC_H_INCLUDED

#include <stdlib.h>

#include "gc.h"

/* Allocation */

#if DEBUG_ALLOC
#define lsh_free debug_free
#define lsh_malloc debug_malloc

void *
debug_malloc(size_t real_size);

void
debug_free(const void *m);
#else /* !DEBUG_ALLOC */

/* ANSI-C free doesn't allow const pointers to be freed. (That is one
 * of the few things that C++ gets right). */
#define lsh_free(p) free((void *) (p))
#define lsh_malloc malloc

#endif /* !DEBUG_ALLOC */


struct lsh_object *
lsh_var_alloc(struct lsh_class *class,
	      size_t extra);

struct lsh_object *
lsh_object_alloc(struct lsh_class *class);

void
lsh_object_free(const struct lsh_object *o);

/* NOTE: This won't work for if there are strings or other instance
 * variables that can't be shared. */

struct lsh_object *
lsh_var_clone(struct lsh_object *o, size_t size);

struct lsh_object *
lsh_object_clone(struct lsh_object *o);

void *lsh_space_alloc(size_t size);
void lsh_space_free(const void *p);

#if DEBUG_ALLOC

struct lsh_object *lsh_object_check(struct lsh_class *class,
				    struct lsh_object *instance);
struct lsh_object *lsh_object_check_subtype(struct lsh_class *class,
					    struct lsh_object *instance);

#define CHECK_TYPE(c, i) \
  lsh_object_check(&CLASS(c), (struct lsh_object *) (i))
#define CHECK_SUBTYPE(c, i) \
  lsh_object_check_subtype(&CLASS(c), (struct lsh_object *) (i))

#define CAST(class, var, o) \
  struct class *(var) = (struct class *) CHECK_TYPE(class, o)

#define CAST_SUBTYPE(class, var, o) \
  struct class *(var) = (struct class *) CHECK_SUBTYPE(class, o)
   
#else   /* !DEBUG_ALLOC */

#define CHECK_TYPE(c, o) ((struct lsh_object *)(o))
#define CHECK_SUBTYPE(c, o) ((struct lsh_object *)(o))
     
#define CAST(class, var, o) \
   struct class *(var) = (struct class *) (o)

#define CAST_SUBTYPE(class, var, o) CAST(class, var, o)

     
#endif  /* !DEBUG_ALLOC */

#define NEW(class, var) \
  struct class *(var) = (struct class *) lsh_object_alloc(&CLASS(class))

#define NEW_VAR_OBJECT(class, var, size) \
  struct class *(var) = \
  (struct class *) lsh_var_alloc(&CLASS(class), size)

#define NEW_SPACE(x) ((x) = lsh_space_alloc(sizeof(*(x))))

#define CLONE(class, i) \
  ((struct class *) lsh_object_clone(CHECK_TYPE(class, (i))))

#define CLONE_VAR_OBJECT(class, i, size) \
  ((struct class *) lsh_var_clone(CHECK_TYPE(class, (i)), (size)))

#define CLONED(class, var, i) \
  struct class *(var) = CLONE(class, i)

#define CLONED_VAR_OBJECT(class, var, i, size) \
  struct class *(var) = CLONE_VAR_OBJECT(class, i, size)

#define KILL(x) gc_kill((struct lsh_object *) (x))

#endif /* LSH_XALLOC_H_INCLUDED */
