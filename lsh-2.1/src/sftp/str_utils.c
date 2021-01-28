/*
 * @(#) $Id: 88c86704a9d98d44dccdc28e977e661707f707a8 $
 *
 * str_utils.c
 */

/* lsftp, an implementation of the sftp protocol
 *
 * Copyright (C) 2001, Pontus Sköld
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

#include <string.h>
#include <stdlib.h>

#include "str_utils.h"

char* lsftp_quote( const char* s )
{
  /* Maximum size needed = len(s)*2+1 */

  int i = 0;
  int j = 0;

  char* news = malloc( strlen(s) * 2 + 1 );

  char* protectchars = "\\[](){} \t\r\n";
  int toprotect = strlen( protectchars );

  /* Malloc failed? */
  
  if( !news ) 
    return 0;

  while( s[i] )
    {
      int safe = 1; /* May go unquoted */
      int k;

      for( k = 0; k < toprotect; k++ )
	if( s[i] == protectchars[k] )
	  safe = 0;
      
      if( safe ) /* Unqouted */
	news[j++] = s[i++];
      else
	{
	  news[j++] = '\\';
	  news[j++] = s[i++];
	}
    }

  news[j] = 0;

  return news;
}

/* Not used anywhere */
#if 0
char *
lsftp_unqoute( char* s )
{
  char* news = strdup( s );
  int i = 0;
  int j = 0;

  int literalq = 0;
  int nextliteral = 0;

  if( 0 == news ) /* strdup failed */
    return NULL;

  while( s[i] )
    {
      int copy = 0;

      if( nextliteral )
	{
	  copy = 1;
	  nextliteral = 0;
	}
      else
	{
	  if( literalq )
	    {
	      if( '\'' == s[i] )
		{
		  literalq = 0;
		  i++;
		}
	      else
		copy = 1;
	    }
	  else
	    {
	      /* Neither nextliteral or literalq */
	      
	      switch( s[i] )
		{
		case '\'':
		  literalq = 1;
		  i++;
		  break;

		case '\\':
		  nextliteral = 1;
		  i++;
		  break;

		default: /* Not a quote - copy */
		  copy = 1;
		}
	    }
	}

      copy = 1;
      
      if( copy )
	news[j++] = s[i++];
    }
      
  news[j] = 0;
  
  return news; 
}
#endif

const char *
lsftp_s_strtok(const char* s, const char* sep,
	       char **store )
{
  /* TODO: should we do this different to support char-types with more than eight bits? */

  const char* tend;
  char* scopy;

  *store = NULL;

  if( !s )   /* NULL-address given? */
    return NULL;

  /* Skip any starting separators */

  s = lsftp_s_skip( s, sep );


  if( !s || !(*s) ) /* No string or ended without a token (only contained separators) */
	return NULL;

  tend = lsftp_s_skipn( s, sep );    /* Find the first separator in our new s (or it's end) */
 
  scopy = strdup( s ); /* Put the token in store */

  if( !scopy )
    return NULL;

  scopy[tend-s] = 0;                /* Make sure it is null-terminated at the right place */

  *store = scopy;
  return tend;                 /* Return a pointer to where we haven't yet looked */
    
}

const char *
lsftp_s_skip(const char *s, const char *sep)
{
  /* Return a pointer into s, skipping by any charaters in sep */

  int i;
  int numsep;
  int cont = 1;

  if( !s ) /* If s is NULL, return NULL */
    return NULL;

  numsep = strlen( sep ); /* Number of tokens to compare with */

  /* While there's any left of the string 
   * (and we're still finding separators) 
   */

  while( cont && *s )
    {
      cont=0;

      for( i=0; i<numsep; i++) 
	if( *s == sep[i] ) /* Current char == separator */
	  cont=1;
      s++;
    }


  if( !cont )
    return s-1; /* If we didn't stop because s terminated, we've gone one step to far */
  
  return NULL; /* We didn't find any non separator */
}

const char *
lsftp_s_skipn(const char *s, const char *sep )
{
  /* Negated version of lsftp_s_skip, 
   * skip by s until we find a charater 
   * in sep 
   */

  int i;
  int numsep;
  int cont=1;

  int literalq = 0;
  int nextliteral = 0;
  int normalq = 0;

  numsep = strlen( sep ); /* Number of tokens to compare with */

  while( cont && *s ) 

/* While there's any left of the string 
 *  (and we're still not finding separators) 
 */
    {
      int check = 0;

      cont=1;

      if( !nextliteral && !normalq && !literalq )  /* Unquoted char? */
	check = 1;

      if( !nextliteral)
	switch( *s )
	  {
	  case '\\':
	    nextliteral = 1;
	    check = 0;
	    break;
	    
	  case '\'':
	    literalq = !literalq;
	    check = 0;
	    break;
	    
	  case '\"':
	    normalq = !normalq;
	    check = 0;
	    break;
	    
	  default:
	    ;
	  }
      else
	{
	  /* nextliteral was true? */
	  nextliteral = 0;
	  check = 0;
	}

      if( check )
	for( i=0; i<numsep; i++)
	  if( *s == sep[i] )
	    cont=0;
      
      s++;
    }

/* If we didn't stop because s terminated, we've gone one step to far */

  if( !cont )
    return s-1; 

  return s;
}
