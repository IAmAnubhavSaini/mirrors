/* atoms.h
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

#ifndef LSH_ATOMS_H_INCLUDED
#define LSH_ATOMS_H_INCLUDED

/* FIXME: Need this be a separate file? */
#include "lsh.h"

#include "atoms_defines.h"

/* Atoms are represented as plain (small) ints. Zero is used for all
 * atoms we don't know about. */

uint32_t get_atom_length(int atom);
const uint8_t *get_atom_name(int atom);
int lookup_atom(uint32_t length, const uint8_t *name);

#endif /* LSH_ATOMS_H_INCLUDED */
