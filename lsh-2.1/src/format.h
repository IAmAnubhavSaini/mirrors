/* format.h
 *
 * Create a packet from a format string and arguments.
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

#ifndef LSH_FORMAT_H_INCLUDED
#define LSH_FORMAT_H_INCLUDED

#include <stdarg.h>

#include "atoms.h"

/* Format strings can contain the following %-specifications:
 *
 * %%  Insert a %-sign
 *
 * %c  Insert an 8-bit character
 *
 * %i  Insert a 32-bit integer, in network byte order
 *
 * %s  Insert a string, given by a length and a pointer.
 *
 * %S  Insert a string, given as a struct lsh_string pointer.
 *
 * %z  Insert a string, using a null-terminated argument.
 *
 * %r  Reserves space in the string, first argument is the length, and
 *     the start position is stored into the second argument, a uint32_t *.
 *
 * %a  Insert a string containing one atom.
 *
 * %A  Insert a string containing a list of atoms. The input is an
 *     int_list object. Zero elements are allowed and ignored.
 *
 * %X  Insert a string containing a list of atoms. The corresponding
 *     argument sublist should be terminated with a zero. (Not used)
 *
 * %n  Insert a string containing a bignum.
 *
 * There are also some valid modifiers:
 *
 * "l" (as in literal). It is applicable to the s, a, A, n and r
 * specifiers, and outputs strings *without* a length field.
 *
 * "d" (as in decimal). For integers, convert the integer to decimal
 * digits. For strings, format the input string using sexp syntax;
 * i.e. prefixed with the length in decimal.
 *
 * "x" (as in heXadecimal). For strings, format each character as two
 * hexadecimal digits. Does not currently mean any thing for numbers.
 * Note that this modifier is orthogonal to the decimal modifier.
 * 
 * "f" (as in free). Frees the input string after it has been copied.
 * Applicable to %S only.
 *
 * "u" (as in unsigned). Used with bignums, to use unsigned-only
 * number format. */

struct lsh_string *ssh_format(const char *format, ...);
uint32_t ssh_format_length(const char *format, ...);
void ssh_format_write(const char *format,
		      struct lsh_string *buffer, uint32_t pos, ...);

uint32_t ssh_vformat_length(const char *format, va_list args);
void ssh_vformat_write(const char *format,
		       struct lsh_string *buffer, uint32_t pos, va_list args);

void
format_hex_string(struct lsh_string *buffer, uint32_t pos,
		  uint32_t length, const uint8_t *data);

     
/* Short cuts */
#define lsh_string_dup(s) (ssh_format("%lS", (s)))

#define make_string(s) (ssh_format("%lz", (s)))

unsigned format_size_in_decimal(uint32_t n);
void
format_decimal(struct lsh_string *buffer, uint32_t pos,
	       uint32_t length, uint32_t n);

/* FIXME: These functions don't really belong here */


struct lsh_string *
lsh_string_colonize(const struct lsh_string *s, int every, int freeflag);

struct lsh_string *
lsh_string_bubblebabble(const struct lsh_string *s, int freeflag);

#endif /* LSH_FORMAT_H_INCLUDED */
