/* lsh-pam-checkpw.c
 *
 * This is a helper program to verify a name and password using
 * PAM. */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001, 2002 Pontus Sköld, Niels Möller
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#if HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <sysexits.h>
#include <security/pam_appl.h>

#define PWD_MAXLEN 1024
#define SERVICE_NAME "lshd"
#define TIMEOUT 600 

static int
conv(int num_msg,
     const struct pam_message** msg,
     struct pam_response** resp,
     void* appdata_ptr)
{
  int i;
  struct pam_response* reply;
  
  if (num_msg <= 0)
    return PAM_CONV_ERR;

  /* Get memory for response array */  
  reply = calloc( num_msg, sizeof( struct pam_response ) );

  if( !reply ) /* Got no memory? */
    return PAM_CONV_ERR;

  /* Hrrm, we could just reply with the password to all messages,
   * would that be better? (the upside would be that we would have a
   * higher hitrate, the downside would be that we might risk that
   * some module logs the password. For the moment, I think responding
   * to the questions with echo off is sufficient and safe enough.
   */

  for( i = 0; i < num_msg; i++ )
    {          
      switch( msg[i]->msg_style )
	{
	case PAM_PROMPT_ECHO_OFF:      /* Assume prompt off => wants password */
	  reply[i].resp_retcode = 0;  /* Default return code */
	  reply[i].resp = 0;
	  
	  if( appdata_ptr )
	      reply[i].resp = strdup( appdata_ptr );

	  break;
	  
	default:
	  
	  /* Information message, leave empty response */
	  reply[i].resp = 0;
	  reply[i].resp_retcode = 0;
	}
    }

  *resp = reply; /* Send back our answers */

  return PAM_SUCCESS;
}

int
main(int argc, char** argv)
{
  struct pam_conv pconv;
  pam_handle_t* pamh;
  char pass[PWD_MAXLEN];
  int authenticated = 0;
  int status = 0;
  char* username;

  int retval;

#ifdef HAVE_ALARM
  alarm( TIMEOUT ); /* Exit after TIMEOUT seconds */
#endif

  if( argc < 2 )
    {
      printf( "usage: %s <USERNAME to check>\nPassword on stdin\n", argv[0] );
      return EX_USAGE;
    }


  username = argv[1]; /* Username to authenticate */

  retval = read( 0, pass, PWD_MAXLEN-1 ); /* Get password */

  if( -1 == retval ) /* Failure while reading? */
    {
      perror( "failed to read password" );
      return EX_IOERR;
    }

  pass[retval] = 0; /* Terminate string just read */

  pconv.conv = conv;
  pconv.appdata_ptr = pass;

  /* Start PAM */
  retval = pam_start( SERVICE_NAME, username, &pconv, &pamh );

  if( PAM_SUCCESS  != retval)
    {
      perror( "pam_start failed" );
      return EX_OSERR;
    }

  /* Authenticate user */

  retval = pam_authenticate( pamh, 
			     PAM_DISALLOW_NULL_AUTHTOK | PAM_SILENT 
			     );

  switch (retval)
    {
    case PAM_SUCCESS:
      /* User successfully authenticated? */
      authenticated = 1;
      break;
      
    case PAM_USER_UNKNOWN:
    case PAM_AUTH_ERR:
      authenticated = 0;
      break;
      
    default:
      fprintf( stderr,
	       "%s (error number was %d)\n",
	       pam_strerror( pamh, retval ),
	       retval
	       );
      return EX_OSERR;
    }

  /* Say goodbye */

  /* FIXME: Second parameter to pam_end should be? */  
  retval = pam_end( pamh, status );

  if( PAM_SUCCESS != retval )
    { 
      perror( "pam_end failed" );
      return EX_OSERR;
    }

  return !authenticated;
}



