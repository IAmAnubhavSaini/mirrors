/* sexp.h
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2002 Niels Möller
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

#ifndef LSH_SEXP_H_INCLUDED
#define LSH_SEXP_H_INCLUDED

#include "lsh.h"

/* Forward declaration, real declaration in nettle/sexp.h. */
struct sexp_iterator;

int
lsh_sexp_to_atom(struct sexp_iterator *i);

int
lsh_sexp_to_uint32(struct sexp_iterator *i, uint32_t *x);

int
lsh_sexp_get_type(struct sexp_iterator *i);

struct lsh_string *
lsh_sexp_to_string(struct sexp_iterator *i, struct lsh_string **display);

struct lsh_string *
lsh_sexp_copy(struct sexp_iterator *i);

#endif /* LSH_SEXP_H_INCLUDED */
