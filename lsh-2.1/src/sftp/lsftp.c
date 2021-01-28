/*
 * @(#) $Id: c8d7f984fd7cb45db90db89d1d3a54bd39df988d $
 *
 * lsftp.c
 */

/* lsftp, an implementation of the sftp protocol
 *
 * Copyright (C) 2001, Pontus Sköld
 * Portions of this code originately from the readline manual
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

#include "lsftp.h"

#include "werror.h"

int mainloop;    /* "Continue run" flag */
int interactive; /* Interactive mode flag */

const char *werror_program_name = "lsftp";



static int
lsftp_num_commands(int argc, char **argv)
{
  /* 
   * Returns how many trailing parameters to skip when
   * starting the SecSH client
   *
   */

  int i = argc - 1;     /* First element to investigate (argv[argc] should be NULL) */
  
  while( i > 0 )
    if( strcmp( LSFTP_COMMAND_MARKER, argv[i] ) ) /* Not a marker? */
      i--;
    else
      break;

  if( i )                      /* Found a marker? */
    return argc - i;           /* The number of parameters not to pass to secsh is argc - i */

  return 0;
}

static void
lsftp_interactive_mainloop(void)
{
  char* current_line;
  int fail = 0;

  mainloop=1;
  while( mainloop && !fail )
    {
      fd_set rfds;
      fd_set wfds;
      
      struct timeval tv;
      int retval;

      lsftp_rl_init_get_line( "command> " );

      while( !lsftp_rl_line_done )
	{
	  int lsh_w = lsftp_fd_write_net();
	  int lsh_r = lsftp_fd_read_net();
	  int try_write = 0;
	  int n;

	  FD_ZERO( &rfds );
	  FD_ZERO( &wfds );

	  /* Watch stdin (fd 0) to see when it has input. */
	  FD_SET( 0, &rfds );

	  if( lsftp_connected() )   /* Only if connected */
	    {

	      /* Anything from lsh would also be interesting */
	      FD_SET( lsh_r, &rfds );
	      
	      /* As would (pherhaps) if we can write to lsh */
	      
	      if( lsftp_want_to_write() )
		{
		  FD_SET( lsh_w, &wfds );
		  try_write = 1;
		}
	      
	      tv.tv_sec = 10;
	      tv.tv_usec = 0;
	      
	      if( 
		 try_write &&
		 lsh_w > lsh_r 
		 ) /* Calculate the n argument to select */
		n = lsh_w + 1;
	      else
		n = lsh_r + 1;
	    }
	  else
	    n = 1;


	  retval = select(n, &rfds, &wfds, NULL, &tv);

	  if( FD_ISSET( 0, &rfds ) )
	    lsftp_rl_check_input();

	  if( lsftp_connected() )
	    if( FD_ISSET( lsh_r, &rfds ) || 
		( 
		 try_write &&
		 FD_ISSET( lsh_w, &wfds )
		 )
		)
	      if( -1 == lsftp_callback() )
		fail++;
	}
      
      current_line = lsftp_rl_line;

      handle_command( current_line );

      free( current_line );
    }

}

static void
lsftp_noninteractive_mainloop(int startat, char **argv)
{
  int done = 0;
  int fail = 0;

  mainloop = 1;

  while( !done && !fail )
    {
      while( !fail && lsftp_active_cbs() )
	if( -1 == lsftp_callback() )            /* Checks itself with select */
	  {
	    printf( "Failure!\n");
	    fail++;
	  }

      if( argv[startat] && mainloop )            /* More commands? (no quit) */
	handle_command( argv[startat++] );         /* Do it */
      else
	done = 1;                                  /* Finito */
          
    }
}




int main(int argc, char** argv, char** envp)
{
  int i;

  do_gnu_stuff( argv ); /* Check for GNU thingies and act accordingly */

  lsftp_rl_init(); /* Safe even if built for noninteractive use only */
  mgetenv_init( envp ); /* Tell mgetenv about environment parameters */

  i = lsftp_num_commands( argc, argv );

  if( i > 1 )                      /* Anything after the last marker? */
    interactive = 0;               /* Non interactive mode */

  if( interactive )
    lsftp_welcome();

  lsftp_sftp_init();

  if( 1 < argc - i ) /* Only connect if given arguments */
  {
    lsftp_open_connection( argv, argc - i );
    lsftp_handshake();

    lsftp_do_cd( "" );                   /* Change to our home directory */
  }

  lsftp_dc_init( 1024 );                 /* Set up the directory cache */

  if( interactive )
    lsftp_interactive_mainloop();
  else
    if( i )
      lsftp_noninteractive_mainloop( argc - i + 1, argv );
    else
      printf( "%s seems to be built for noninteractive mode only,\n" 
	      "but you gave no command.\n", PACKAGE);

  lsftp_rl_exit();
  lsftp_close_connection();
  return 0;
}


