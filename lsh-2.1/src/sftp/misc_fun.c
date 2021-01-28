/*
 * @(#) $Id: e5dc8a8ab0e2ab957a7ccf1a338cc96b6792c961 $
 *
 * misc_fun.c
 */

/* lsftp, an implementation of the sftp protocol
 *
 * Copyright (C) 2001 Pontus Sköld
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
# include "config.h"
#endif

#include <stdlib.h>

#include "misc_fun.h"

#ifndef HAVE_GETENV
char** environ;
#endif

char* mgetenv( char* var )
{
#ifdef HAVE_GETENV
  /* If the system provides a getenv function, use it */
  return getenv(var);
#else
  char* env;
  char** lenviron;
  int varlen;
  
  lenviron = environ;
  varlen = strlen(var);
  
  while( ( env = *lenviron++) ) /* Get next enviroment variable */
    if( !( strncmp( var, env, varlen ) ))
      /* Found it? (ignores A==B and AX=B) */
      return lsftp_s_skipn( env, "=" )+1; 
  /* We return whatever is after the equal sign */
  
  return NULL;
#endif /* HAVE_GETENV */
}

void mgetenv_init( char** envp )
{
#ifndef HAVE_GETENV
  environ = envp;
#endif
}

char* massage_fname(char* fname, int local)
{
  return fname;
}


const char *
filename_part(const char *s)
{
  int i = strlen(s);
  
  while( i >= 0 && s[--i] != '/' )         /* Search backwards for a directory separator, stop at the start of the string */
    ;
    
  if( !s[i+1] )                            /* String ended with slash? */
    while( i >= 0 && s[--i] != '/' );      /* Redo it */

  return s + i + 1;
}


char* lsftp_concat( const char* s1, const char* s2 )
{
  int i1,i2;
  char* ret;

  i1 = strlen( s1 );
  i2 = strlen( s2 );

  ret = malloc( i1 + i2 + 1 ); /* Get mem */
  
  if( !ret )
    return 0;

  strncpy( ret, s1, i1 );
  strncpy( ret+i1, s2, i2 );

  ret[i1+i2] = 0; /* Make sure it's null terminated */

  return ret;
}


const char *
lsftp_skip_common(const char *s1, const char *s2)
{
  /* Returns the adress of the first charater in s1 not in s2 */
  int i = 0;

  while( s1[i] && s2[i] && (s1[i] == s2[i]) ) /* Neither string has terminated */
    i++;

  return s1+i;
}


int lsftp_get_opts( char* options, char* string )
{
  /* Search string for charaters in options and return a bitmap of which options were found
   */

  int set = 0;
  int last;
  int sindex = 1;
  char c;

  if( !options || !options[0] || !string || '-' != string[0] ) /* Nothing to do? */
    return 0;

  last = strlen( options ) - 1; /* Index of last charater in options */
  
  while( ( c = string[sindex++] ) ) /* Get next (or terminate if finished) */
    {
      int i = 0;

      while( 0 <= last - i )
	if( c == options[last - i++] ) /* Search options from right to left for c */
	  set |= ( 1 << (i-1) ); /* Set corresponding bit in set */
    }

  return set;

}
