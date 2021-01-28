/* format.c
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include <nettle/bignum.h>

#include "format.h"

#include "list.h"
#include "lsh_string.h"
#include "werror.h"
#include "xalloc.h"

struct lsh_string *ssh_format(const char *format, ...)
{
  va_list args;
  uint32_t length;
  struct lsh_string *packet;

  va_start(args, format);
  length = ssh_vformat_length(format, args);
  va_end(args);

#if DEBUG_ALLOC
  packet = lsh_string_alloc_clue(length, format);
#else
  packet = lsh_string_alloc(length);
#endif
  
  va_start(args, format);
  ssh_vformat_write(format, packet, 0, args);
  va_end(args);

  return packet;
}

uint32_t ssh_format_length(const char *format, ...)
{
  va_list args;
  uint32_t length;

  va_start(args, format);
  length = ssh_vformat_length(format, args);
  va_end(args);

  return length;
}

void
ssh_format_write(const char *format, struct lsh_string *buffer, uint32_t start, ...)
{
  va_list args;
  
  va_start(args, start);
  ssh_vformat_write(format, buffer, start, args);
  va_end(args);
}
     
static int
write_decimal_length(struct lsh_string *buffer, uint32_t start, uint32_t n);

uint32_t ssh_vformat_length(const char *f, va_list args)
{
  uint32_t length = 0;

  while(*f)
    {
      if (*f == '%')
	{
	  int literal = 0;
	  int decimal = 0;
	  int unsigned_form = 0;
	  int hex = 0;
	  
	  while(*++f)
	    {
	      switch (*f)
		{
		case 'l':
		  literal = 1;
		  break;
		case 'd':
		  decimal = 1;
		  break;
		case 'f':
		  /* Do nothing */
		  break;
		case 'u':
		  unsigned_form = 1;
		  break;
		case 'x':
		  hex = 1;
		  break;
		default:
		  goto end_options;
		}
	    }
end_options:

	  if (literal && decimal)
	    fatal("Internal error!\n");
	  
	  switch(*f++)
	    {
	    case 'c':
	      (void) va_arg(args, int);
	      /* Fall through */
	    case '%':
	      length++;
	      break;

	    case 'i':
	      {
		uint32_t i = va_arg(args, uint32_t);
		if (decimal)
		  length += format_size_in_decimal(i);
		else
		  length += 4;
		break;
	      }

	    case 's':
	      {
		uint32_t l = va_arg(args, uint32_t); /* String length */ 
		(void) va_arg(args, const uint8_t *);    /* data */

		length += l;

		if (hex)
		  length += l;

		if (decimal)
		  length += format_size_in_decimal(l) + 1;
		else if (!literal)
		  length += 4;

		break;
	      }
	    case 'S':
	      {
		struct lsh_string *s = va_arg(args, struct lsh_string *);
		uint32_t l = lsh_string_length(s);
		length += l;

		if (hex)
		  length += l;
		
		if (decimal)
		  length += format_size_in_decimal(l) + 1;
		else if (!literal)
		  length += 4;
		
		break;
	      }
	    case 'z':
	      {
		unsigned l = strlen(va_arg(args, const char *));
		length += l;

		if (decimal)
		  length += format_size_in_decimal(l) + 1;
		
		else if (!literal)
		  length += 4;
		break;
	      }
	    case 'r':
	      {
		uint32_t l = va_arg(args, uint32_t); 
		length += l;
		(void) va_arg(args, uint8_t **);    /* pointer */

		if (decimal)
		  length += format_size_in_decimal(l) + 1;
		else if (!literal)
		  length += 4;

		break;
	      }
	    case 'a':
	      {
		int atom = va_arg(args, int);
		int l;
		
		assert(atom);

		l = get_atom_length(atom);
		length += l;

		if (decimal)
		  length += format_size_in_decimal(l) + 1;
		else if (!literal)
		  length += 4;

		break;
	      }
	    case 'A':
	      {
		struct int_list *l = va_arg(args, struct int_list *);
		uint32_t n, i;

		if (decimal)
		  fatal("ssh_format: Decimal lengths not supported for %%A\n");
		
		for(n = i =0; i < LIST_LENGTH(l); i++)
		  {
		    if (LIST(l)[i])
		      {
			n++;
			length += get_atom_length(LIST(l)[i]);
		      }
		  }
		if (n)
		  /* One ','-character less than the number of atoms */
		  length += (n-1);
		    
		if (!literal)
		  length += 4;

		break;
	      }
	    case 'n':
	      {
		MP_INT *n = va_arg(args, MP_INT*);

		/* Calculate length of written number */
		unsigned l;
		if (unsigned_form)
		  {
		    assert(mpz_sgn(n) >= 0);
		    l = nettle_mpz_sizeinbase_256_u(n);
		  }
		else
		  /* FIXME: Do we really need tohandle negative
		   * numbers in lsh? */
		  /* Unlike nettle's convention, zero is represented
		   * as an empty string. */
		  l = mpz_sgn(n) ? nettle_mpz_sizeinbase_256_s(n) : 0;

		length += l;

		/* Decimal not supported. */
		assert(!decimal);

		if (!literal)
		  length += 4;

		break;
	      }
	    default:
	      fatal("ssh_vformat_length: bad format string");
	      break;
	    }
	}
      else
	{
	  length++;
	  f++;
	}
    }
  return length;
}

void
ssh_vformat_write(const char *f, struct lsh_string *buffer, uint32_t pos, va_list args)
{
  while(*f)
    {
      if (*f == '%')
	{
	  int literal = 0;
	  int do_free = 0;
	  int decimal = 0;
	  int hex = 0;
	  int unsigned_form = 0;

	  uint32_t size;
	  const uint8_t *data;
	  
	  while(*++f)
	    {
	      switch (*f)
		{
		case 'l':
		  literal = 1;
		  break;
		case 'd':
		  decimal = 1;
		  break;
		case 'f':
		  do_free = 1;
		  break;
		case 'u':
		  unsigned_form = 1;
		  break;
		case 'x':
		  hex = 1;
		  break;
		default:
		  goto end_options;
		}
	    }
	end_options:
		  
	  if (literal && decimal)
	    fatal("Internal error!\n");

	  switch(*f++)
	    {
	    case 'c':
	      lsh_string_putc(buffer, pos++, va_arg(args, int));
	      break;

	    case '%':
	      lsh_string_putc(buffer, pos++, '%');
	      break;

	    case 'i':
	      {
		uint32_t i = va_arg(args, uint32_t);
		if (decimal)
		  {
		    unsigned length = format_size_in_decimal(i);
		    format_decimal(buffer, pos, length, i);
		    pos += length;
		  }
		else
		  {
		    lsh_string_write_uint32(buffer, pos, i);
		    pos += 4;
		  }
		break;
	      }

	    case 'z':
	      data = va_arg(args, const char *);
	      size = strlen(data);

	      goto do_string;
	      
	    case 's':
	      size = va_arg(args, uint32_t);
	      data = va_arg(args, const uint8_t *);
	      
	    do_string:
	      {
		uint32_t length = hex ? (2*size) : size;

		assert(!do_free);

		if (decimal)
		  pos += write_decimal_length(buffer, pos, length);
		else if (!literal)
		  {
		    lsh_string_write_uint32(buffer, pos, length);
		    pos += 4;
		  }

		if (hex)
		  format_hex_string(buffer, pos, size, data);
		else
		  lsh_string_write(buffer, pos, size, data);

		pos += length;
		break;
	      }
	      
	    case 'S':
	      {
		struct lsh_string *s = va_arg(args, struct lsh_string *);
		uint32_t length;
		
		size = lsh_string_length(s);
		data = lsh_string_data(s);

		/* FIXME: Some duplication */
		length = hex ? (2*size) : size;

		if (decimal)
		  pos += write_decimal_length(buffer, pos, length);
		else if (!literal)
		  {
		    lsh_string_write_uint32(buffer, pos, length);
		    pos += 4;
		  }

		if (hex)
		  format_hex_string(buffer, pos, size, data);
		else
		  lsh_string_write(buffer, pos, size, data);
		
		pos += length;

		if (do_free)
		  lsh_string_free(s);
		
		break;
	      }

	    case 'r':
	      {
		uint32_t length = va_arg(args, uint32_t);
		uint32_t *p = va_arg(args, uint32_t *);

		if (decimal)
		  pos += write_decimal_length(buffer, pos, length);
		else if (!literal)
		  {
		    lsh_string_write_uint32(buffer, pos, length);
		    pos += 4;
		  }

		if (p)
		  *p = pos;
		pos += length;

		break;
	      }
	    
	    case 'a':
	      {
		uint32_t length;
		int atom = va_arg(args, int);
		
		assert(atom);

		length = get_atom_length(atom);

		if (decimal)
		  pos += write_decimal_length(buffer, pos, length);
		else if (!literal)
		  {
		    lsh_string_write_uint32(buffer, pos, length);
		    pos += 4;
		  }

		lsh_string_write(buffer, pos, length, get_atom_name(atom));
		pos += length;

		break;
	      }
	    case 'A':
	      {
		struct int_list *l = va_arg(args, struct int_list *);
		uint32_t start = pos; /* Where to store the length */
		uint32_t n, i;
		
		if (decimal)
		  fatal("ssh_format: Decimal lengths not supported for %%A\n");

		if (!literal)
		  pos += 4;
		
		for(n = i = 0; i < LIST_LENGTH(l); i++)
		  {
		    if (LIST(l)[i])
		      {
			uint32_t length = get_atom_length(LIST(l)[i]);
			
			if (n)
			  /* Not the first atom */
			  lsh_string_putc(buffer, pos++, ',');

			lsh_string_write(buffer, pos, length, get_atom_name(LIST(l)[i]));
			pos += length;

			n++;
		      }
		  }

		if (!literal)
		  {
		    uint32_t total = pos - start - 4;
		    lsh_string_write_uint32(buffer, start, total);
		  }
		break;
	      }
	    case 'n':
	      {
		MP_INT *n = va_arg(args, MP_INT *);
		uint32_t length;

		/* Decimal not supported */
		assert(!decimal);
		
		if (unsigned_form)
		  {
		    assert(mpz_sgn(n) >= 0);

		    length = nettle_mpz_sizeinbase_256_u(n);
		  }
		else
		  length = mpz_sgn(n) ? nettle_mpz_sizeinbase_256_s(n) : 0;

		if (!literal)
		  {
		    lsh_string_write_uint32(buffer, pos, length);
		    pos += 4;
		  }		
		lsh_string_write_bignum(buffer, pos, length, n);
		pos += length;

		break;
	      }
	    default:
	      fatal("ssh_vformat_write: bad format string");
	      break;
	    }
	}
      else
	lsh_string_putc(buffer, pos++, *f++);
    }
}

unsigned
format_size_in_decimal(uint32_t n)
{
  int i;
  int e;
  
  /* Table of 10^(2^n) */
  static const uint32_t powers[] = { 10UL, 100UL, 10000UL, 100000000UL };

#define SIZE (sizeof(powers) / sizeof(powers[0])) 

  /* Determine the smallest e such that n < 10^e */
  for (i = SIZE - 1 , e = 0; i >= 0; i--)
    {
      if (n >= powers[i])
	{
	  e += 1UL << i;
	  n /= powers[i];
	}
    }

#undef SIZE
  
  return e+1;
}


void
format_hex_string(struct lsh_string *buffer, uint32_t pos,
		  uint32_t length, const uint8_t *data)
{
  static const uint8_t hexchars[16] = "0123456789abcdef";
  uint32_t i;

  for (i = 0; i < length; i++)
    {
      lsh_string_putc(buffer, pos++, hexchars[ (data[i] & 0xf0) >> 4 ]);
      lsh_string_putc(buffer, pos++, hexchars[ data[i] & 0x0f]);
    }
}

void
format_decimal(struct lsh_string *buffer, uint32_t start,
	       uint32_t length, uint32_t n)
{
  unsigned i;
  
  for (i = 0; i<length; i++)
    {
      lsh_string_putc(buffer, start + length - i - 1, '0' + n % 10);
      n /= 10;
    }
}

static int
write_decimal_length(struct lsh_string *buffer, uint32_t start, uint32_t n)
{
  int length = format_size_in_decimal(n);

  format_decimal(buffer, start, length, n);
  lsh_string_putc(buffer, start + length, ':');

  return length + 1;
}



struct lsh_string *
lsh_string_colonize(const struct lsh_string *s, int every, int freeflag)
{
  uint32_t i = 0;
  uint32_t j = 0;

  struct lsh_string *packet;
  const uint8_t *data;
  uint32_t length;
  uint32_t size;
  int colons;

  /* No of colonds depens on length, 0..every => 0, 
   * every..2*every => 1 */
  length = lsh_string_length(s);
  data = lsh_string_data(s);
  
  colons = length ? (length - 1) / every : 0;
  size = length + colons;

  packet = lsh_string_alloc(size);

  for (; i<length; i++)
    {
      if (i && !(i%every))  /* Every nth position except at the beginning */
	lsh_string_putc(packet, j++, ':');

      lsh_string_putc(packet, j++, data[i]);
    }

  assert(j == size);

  if (freeflag) /* Throw away the source string? */
    lsh_string_free( s );

  return packet;
}

static uint8_t 
lsh_string_bubblebabble_c(const struct lsh_string *s, uint32_t i)
{ 
  /* Recursive, should only be used for small strings */

  uint8_t c;
  uint32_t j;
  uint32_t k;
  uint32_t length = lsh_string_length(s);
  const uint8_t *data = lsh_string_data(s);
  assert( 0 != i);

  if (1==i)
    return 1;

  j = i*2-3-1;
  k = i*2-2-1;

  assert( j < length && k < length );

  c = lsh_string_bubblebabble_c( s, i-1 );
 
  return (5*c + (data[j]*7+data[k])) % 36;
}

struct lsh_string *
lsh_string_bubblebabble(const struct lsh_string *s, int freeflag)
{
  /* Implements the Bubble Babble Binary Data Encoding by Huima as
   * posted to the secsh list in August 2001 by Lehtinen.*/

  uint32_t length = lsh_string_length(s);
  uint32_t i = 0;
  uint32_t babblelen = 2 + 6*(length/2) + 3;
  struct lsh_string *p = lsh_string_alloc( babblelen );
  
  uint32_t r = 0;
  const uint8_t *q = lsh_string_data(s);

  uint8_t a;
  uint8_t b;
  uint8_t c;
  uint8_t d;
  uint8_t e;

  char vowels[6] = { 'a', 'e', 'i', 'o', 'u', 'y' };

  char cons[17] = { 'b', 'c', 'd', 'f', 'g', 'h', 'k',  'l', 'm',
		    'n', 'p', 'r', 's', 't', 'v', 'z', 'x' }; 

  lsh_string_putc(p, r++, 'x');
  
  while( i < length/2 )
    {
      assert( i*2+1 < length );

      a = (((q[i*2] >> 6) & 3) + lsh_string_bubblebabble_c( s, i+1 )) % 6;
      b = (q[i*2] >> 2) & 15;
      c = ((q[i*2] & 3) + lsh_string_bubblebabble_c( s, i+1 )/6 ) % 6;
      d = (q[i*2+1] >> 4) & 15; 
      e = (q[i*2+1]) & 15;

      lsh_string_putc(p, r++, vowels[a]);
      lsh_string_putc(p, r++, cons[b]);
      lsh_string_putc(p, r++, vowels[c]);
      lsh_string_putc(p, r++, cons[d]);
      lsh_string_putc(p, r++, '-');
      lsh_string_putc(p, r++, cons[e]);

      i++;
    }

  if( length % 2 ) /* Odd length? */
    {
      a = (((q[length-1] >> 6) & 3) + lsh_string_bubblebabble_c( s, i+1 )) % 6;
      b = (q[length-1] >> 2) & 15;
      c = ((q[length-1] & 3) + lsh_string_bubblebabble_c( s, i+1 )/6 ) % 6;
    }
  else
    {
      a = lsh_string_bubblebabble_c( s, i+1 ) % 6;
      b = 16;
      c = lsh_string_bubblebabble_c( s, i+1 ) / 6;
    }

  lsh_string_putc(p, r++, vowels[a]);
  lsh_string_putc(p, r++, cons[b]);
  lsh_string_putc(p, r++, vowels[c]);
  
  lsh_string_putc(p, r++, 'x');
  
  assert(r == lsh_string_length(p));
  
  if( freeflag )
    lsh_string_free( s );

  return p;
}
