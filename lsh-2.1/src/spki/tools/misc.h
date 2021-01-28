/* misc.h */

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

#ifndef SPKI_TOOLS_IO_H_INCLUDED
#define SPKI_TOOLS_IO_H_INCLUDED

#include <nettle/nettle-meta.h>

#include <stdio.h>

void *
xalloc(size_t size);

void
die(const char *format, ...);

uint8_t *
hash_file(const struct nettle_hash *hash, FILE *f);

/* If size is > 0, read at most that many bytes. If size == 0,
 * read until EOF. Allocates the buffer dynamically. */
char *
read_file(FILE *f, unsigned size, unsigned *length);

char *
read_file_by_name(const char *name, unsigned size, unsigned *length);

int
write_file(FILE *f, unsigned size, const char *buffer);

#endif /* SPKI_TOOLS_IO_H_INCLUDED */
