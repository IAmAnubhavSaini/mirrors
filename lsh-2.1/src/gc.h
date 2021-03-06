/* gc.h
 *
 * Simple mark&sweep garbage collector.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels M�ller
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

#ifndef LSH_GC_H_INCLUDED
#define LSH_GC_H_INCLUDED

#include "lsh.h"

void gc_register(struct lsh_object *o);
void gc_kill(struct lsh_object *o);

void gc_global(struct resource *o);

void gc(void);

void gc_final(void);

struct lsh_object *
gc_iterate_objects(const struct lsh_class *class, struct lsh_object *o);

#endif /* LSH_GC_H_INCLUDED */
