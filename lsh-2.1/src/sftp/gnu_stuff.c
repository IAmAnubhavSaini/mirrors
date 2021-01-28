/*
 * @(#) $Id: ab7b8c2d0687059726d6602beaa710e374a019a6 $
 *
 * gnu_stuff.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "gnu_stuff.h"

void lsftp_welcome()
{
  printf( "Welcome to %s %s by Pontus Sköld, Niels Möller and various.\n", PACKAGE, VERSION );
  printf( "This program is free software, for more information\n" );
  printf( "please see the file COPYING or type about, you may\n" );
  printf( "also use help or ? to get help.\n\n");
  printf( "Trying to connect, please stand by.\n");

}

void help_option()
{
  printf( "Usage: lsftp [ <transport options> ] \n" );
  printf( "or:    lsftp [ <transport options> ] -- [ COMMAND ] [ COMMAND ] \n" );
  
  printf( "For information on the commands of lsftp, you may try the built in help\n" );
  printf( "by running lsftp -- help (please note the space between -- and help).\n" );
  printf( "\n");
  printf( "INTERACTIVE MODE:\n" );
  printf( "\n");
  printf( "To enter interactive mode, just start lsftp. A command prompt will appear.\n");
  printf( "To try the built in help system, type help (or ?) followed by return.\n");
  printf( "\n");
  printf( "NON-INTERACTIVE MODE:\n" );
  printf( "\n");
  printf( "Non interactive mode can be accessed by having -- anywhere in front of\n" );
  printf( "the last token of the commandline (you need to have something after it)\n");

  printf( "\n");
  printf( "In order NOT to pass into non interactive mode (e.g. if you want to pass -- \n");
  printf( "to your secsh-client), put -- with nothing following last on the command line.\n");
  printf( "\n");
  printf( "For noninteractive usage, each token is handled as a complete line, so to \n");
  printf( "go to the directory bar and put the file foo, you'd write something like\n");
  printf( "\n");
  printf( "lsftp host -- 'cd bar' 'put foo' \n");
  printf( "\n");
  printf( "TROUBLESHOOTING CONNECTIONS:\n" );
  printf( "\n");
  printf( "All lsftp really cares about is to find something to speak sftp with,\n");
  printf( "normally, it does with by requesting a subsystem on a remote host, but it\n");
  printf( "may as well start a local program or request the start of a sftp-server\n");
  printf( "remotely with a shell request\n");
  printf( "\n");
  printf( "The command actually executed is\n" );
  printf( "\n");
  printf( "$LSFTP_RSH $LSFTP_BEFORE_ARGS <anything on the command line> $LSFTP_AFTER_ARGS\n");
  printf( "\n");
  printf( "Report bugs to <bug-lsh@gnu.org>.\n" );
  printf( "\n");

  exit(0); /* Exit successfully */
}

void version_option()
{
  printf("%s %s\n", PACKAGE, VERSION);
  printf("Copyright (C) Pontus Sköld, Niels Möller and various contributors\n\n");

  printf("This program is free software, you may distribute it under the\n");
  printf("terms of the GNU Genereal Public License. \n\n");
  
  printf("This program is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
  printf("General Public License for more details.\n");

  exit(0); /* Exit successfully */
}

void do_gnu_stuff( const char** argv )
{
  const char** current = argv;

  while( *current ) /* More arguments? */
    {
      if( !strcmp( *current, "--version" ) )
	version_option();

      if( !strcmp( *current, "--help" ) )
	help_option();

      current++;
    }
}


#ifndef HAVE_CANONICALIZE_FILE_NAME

/* Compability function, taken from a parted patch */

char*
canonicalize_file_name (const char* name)
{
  char* buf;
  int size;
  char* result;
  
#ifdef PATH_MAX
  size = PATH_MAX;
#else
  /* Bigger is better; realpath has no way todo bounds checking.  */
  size = 4096;
#endif

  /* Just in case realpath does not NULL terminate the string
   * or it just fits in SIZE without a NULL terminator.  */
  buf = calloc( size + 1, 1 );
 
  if( !buf ) 
    {
      errno = ENOMEM;
      return NULL;
    }

  result = realpath(name, buf);

  if (! result)
    free (buf);

  return result;
}
#endif /* !HAVE_CANONICALIZE_FILE_NAME */
