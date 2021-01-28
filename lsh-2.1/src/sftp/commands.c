/*
 * @(#) $Id: 059f3050a94b10875f55d32bbac5d16545528818 $
 *
 * commands.c
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

#include "commands.h"

int ls_longlist = 1;
int ls_all = 0;
int gp_cont = 1;


command commands[] = {
  { "cd", com_cd, "Change to directory DIR", 
    "Change to directory DIR, relative paths\n"
    "are handled by the other side. If DIR is"
    "the empty string, go to your home directory", REMOTEFILE, 1, 2 },

  { "rm", com_rm, "Delete file or directory ", 
    "rm is used to delete files or directories. To delete\n" 
    "directories, you need to specify the r option. Normally,\n"
    "the rm command doesn't glob (you have to give the exact\n"
    "of what you want to delete), but that can be enabled with\n"
    "the -g option.\n",
    REMOTEFILE, 1, 2 },

  { "delete", com_rm, "Synonym for `rm'", 
    "Please see the help for rm.", REMOTEFILE, 0, 2 },

  { "about", com_about, "Display information about lsftp", 
    "Displays some information about lsftp.", NOARG, 1, 1 },  

  { "help", com_help, "List available commands or help for command", 
    "help also displays more specific information for certain commands,\n"
    "as you have likely already discovered.", COMMAND, 1, 1 },

  { "longhelp", com_longhelp, "Give more help", 
    "If given a command, this is equal to help (help actually uses longhelp for this).\n"
    "used without arguments, longhelp displays short help texts for all commands.", COMMAND, 1, 2 },

  { "?", com_help, "Synonym for `help'", 
    "You can type help COMMAND, but you've already discovered that\n", 
    COMMAND, 0, 1 },

  { "list", com_ls, "Synonym for ls", 
    "Please see the help for ls.", REMOTEFILE, 0, 2 },

  { "ls", com_ls, "List files remotely", 
    "ls lists files or folder remotely. It takes options to modify\n"
    "it's behaviour (normally derived from the global options that\n"
    "you can modify with set). Options may be given after a -(minus)\n"
    "sign. The option l toggles whatever files will be displayed in long\n"
    "listing mode. The a option toggles whatever files with names starting\n"
    "with dots will be shown unless specifically asked for.\n"
    "\n"
    "The ls command accepts globs, put for efficiency everything to the\n"
    "right of the last slash (/) is considered a filename-matching pattern\n"
    "meaning ls fo* lists information about files or folders with names\n"
    "starting with fo. ls fo*/ on the other hand will list information\n"
    "about files in directories with names starting with fo (and give\n"
    "error messages if you have any files with names starting with fo).",
    REMOTEFILE, 1, 2 },


  { "dir", com_ls, "Synonym for `ls'", 
    "Please see the help for ls", REMOTEFILE, 0, 2 },

  { "pwd", com_pwd, "Print the current working directory", 
    "pwd longdoc", NOARG, 1, 2 },

  { "quit", com_quit, "Quit using lsftp",
    "Leave lsftp", NOARG, 1, 1 },

  { "exit", com_quit, "Synonym for `quit'",
    "Leave lsftp", NOARG, 0, 1 },

  { "set", com_set, "Show or set variables",
    "Allows you to show or set variables is lsftp, " 
    "useful for brining mayhem.", OTHERARG, 0, 1 },

  { "mail", com_mail, "Run mail",
    "Starts mail", OTHERARG, 1, 3 },

  { "umask", com_umask, "Show or set umask",
    "Shows or sets your umask", OTHERARG, 1, 1 },


  { "jobs", com_jobs, "Job info",
    "Show status for JOBID", JOBID, 0, 1 },

  { "open", com_open, "Open a new connection to a site",
    "Opens a connection to a site", OTHERARG, 0, 1 },
  { "close", com_close, "Close an existing connection",
    "Close kills an existing connection", NOARG, 0, 2 },



  { "rename", com_mv, "Synonym for `mv'",
    "rename longdoc", REMOTEFILE, 0, 2 },
  { "mv", com_mv, "Rename FILE to NEWNAME",
    "rename longdoc", REMOTEFILE, 1, 2 },


  { "ln", com_ln, "Symlink FILE",
    "ln longdoc", REMOTEFILE, 1, 2 },



  { "get", com_get, "Get FILE to local system",
    "Get longdoc", REMOTEFILE, 1, 1 },

  { "put", com_put, "Put FILE to remote system",
    "Put longdoc", LOCALFILE, 1, 2 },

  { "chown", com_chown, "Change owner of FILE to UID", 
    "chown UID FILE changes the owner of FILE to UID", 
    REMOTEFILE, 1, 3 },

  { "chgrp", com_chgrp, "Change group of FILE to GID", 
    "chgrp GID FILE changes the group of FILE to GID", 
    REMOTEFILE, 1, 3 },

  
  { "lcd", com_lcd, "LOCALLY Change to directory DIR",
    "lcd longdoc" , LOCALFILE, 1, 2 },

  { "lrm", com_lrm, "LOCALLY Delete FILE", 
    "lrm longdoc" , LOCALFILE, 1, 2 },

  { "ldelete", com_lrm, "Synonym for `lrm'", 
    "ldelete longdoc" , LOCALFILE, 0, 2 },


  { "mkdir", com_mkdir, "Makes directory DIR [DIR...]", 
    "Makes a new directory" , OTHERARG, 0, 2 },


  { "llist", com_lls, "List files in LOCAL [DIR]", 
    "llist longdoc" , LOCALFILE, 0, 3 },

  { "lls", com_lls, "List files in LOCAL [DIR]", 
    "lls" , LOCALFILE, 1, 3 },

  { "ldir", com_lls, "Synonym for `lls'", 
    "ldir longdoc" , LOCALFILE, 0, 3 },

  { "lpwd", com_lpwd, "Print the current (local) working directory", 
    "lpwd longdoc" , LOCALFILE, 1, 2 },


  /*  { "lmv", com_lmv, "LOCALLY Rename FILE to NEWNAME", 
    "lmv longdoc" , LOCALFILE, 1 },
    { "lrename", com_lmv, "Synonym for `lmv'", 
    "lrename longdoc" , LOCALFILE, 1 }, */
  
  { "!", com_escape, "Send COMMAND to /bin/sh", 
    "Executes system(\"COMMAND\");" , OTHERARG, 1, 1 },

  { NULL, NULL, NULL, NULL, 0, 0, 0 }
};

static int
com_disconnected(void)
{
  printf( "This command doesn't work in disconnected mode!\n" );
  return -1;
}

static int
com_connected(void)
{
  printf( "This command doesn't work in connected mode!\n" );
  return -1;
}


int       
com_help(const char *arg, const char *command UNUSED)
{ 
  const char *s;
  
  s = lsftp_s_skip(arg," \n\t\r"); /* Skip any initial whitespace */
  
  if( !s || ! (*s) ) /* End of string? */
    {
      int row = 0;
      char* cmdname;
      int i=0;
      
      /* No command given, show the short description */

      while( ( cmdname = commands[i++].name) )
	{
          printf( "%-10s",      /* Print each command */
		  cmdname
		  );
	  row++;

	  if( 8 == row )
	    {
	      printf("\n");
	      row = 0;
	    }
	}

      if( row ) /* Anything printed on this line? */
	printf("\n");

      printf("\nFor more help, try longhelp or help <COMMAND>\n");
      return 0;
    }
  else /* Command given, show long help */
    return com_longhelp( arg, command );

}


int
com_longhelp(const char *arg, const char *command UNUSED) 
{
  const char *s;
  char* tmp;
  char* cmdname;
  int i=0;
  
  s = lsftp_s_skip(arg," \n\t\r"); /* Skip any initial whitespace */
  
  if( !s || ! (*s) ) /* End of string? */
    {
      /* No command given, show the short description */

      while( ( cmdname = commands[i++].name) )
	printf("%-10s -- %s\n",
	       cmdname, 
	       commands[i-1].doc
	       ); 
      /* Print command name (10 columns) -- documentation */
        
      return 0;
    }


  while(1)
    {
      int known=0;

      i=0;
      /* Get the first word from the line (the command) */
      s = lsftp_s_strtok(s," \n\t\r", &tmp ); 

  
      if( !tmp ) /* No more? */
	return 0;

      while( ( cmdname = commands[i++].name) )
	if ( ! strcmp(cmdname, tmp) )          /* We've got the command? */
	  {
	    known=1;
	    printf(
		   "%-10s -- %s\n\n%s\n", 
		   commands[i-1].name, 
		   commands[i-1].doc,
		   commands[i-1].longdoc
		   );
	    /* Print documentation */
	  }
  
      if( !known ) /* If no command matched, say so */
	printf("Unknown command %s\n", tmp);
      
      free( tmp );
    }

  return 0; /* Placeholder to stop complaints from compilers*/

}

int
com_mail(const char *arg UNUSED, const char *command) 
{
  return system(command);
}

int
com_umask(const char *arg, const char *command UNUSED) 
{
  char* tmp;
  int given = 0;

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      int newmask = 0;

      given++; /* User gave a new mask */

      /* FIXME: Should probably explicitly state signed/unsigned in all declarations. */

      if( isdigit( (unsigned char) tmp[0] ) )
	{
	  newmask = atol( tmp );
	  umask( newmask );
	}
      else
	printf( "Sorry, only numeric masks allowed\n" );

      free( tmp );
    }

  if( !given )
    {
      /* FIXME: Better way to check? */

      int oldmask = umask( 0 ); /* Get old and reset it */
      umask( oldmask );

      printf( "Your current umask is %o.\n", oldmask );
    }

  /* FIXME: What does return value mean? */
  return 0;
}

int
com_escape(const char *arg, const char *command UNUSED) 
{
  int ret;
  printf( "Launching %s\n", arg );
  ret = system( arg );
  printf( "Returned from system call %s\n", arg );

  return ret;
}

int
com_quit(const char *arg UNUSED, const char *command UNUSED) 
{
  if( lsftp_connected() )   /* If connected */
    com_close( "", "INTERNAL CLOSE COMMAND" ); /* Close connection */

  printf( "Bye\n" );

  mainloop=0;
  return 0;
}

int
com_about(const char *arg UNUSED, const char *command UNUSED)
{
  printf("%s %s\n", PACKAGE, VERSION);
  printf("Copyright (C) 2001, Pontus Sköld and various contributors\n\n");

  printf("This program is free software, you may distribute it under the\n");
  printf("terms of the GNU Genereal Public License. \n\n");
  
  printf("This program is distributed in the hope that it will be useful,\n");
  printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
  printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n");
  printf("General Public License for more details.\n");

  return 0;
}


int
com_close(const char *arg UNUSED, const char *command UNUSED)
{
  /* Close any existing connections */

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  return lsftp_close_connection();
}


int
com_open(const char *arg, const char *command UNUSED)
{
  char** myargv;
  char** freeargv;
  char *s;
  char* tmp; /* Used for arguments */

  int argcount = 0;
  int maxargs;

  if( lsftp_connected() )   /* Bail out *if* connected */
    return com_connected();

  if( arg )                        /* Any arguments given? */
    maxargs = strlen( arg ) + 2;  /* We will have at most one pointer per charater in arg */
  else
    maxargs = 2;

  myargv = malloc( maxargs * sizeof( char* ) ); 

  if( myargv )     /* Malloc OK? */
    {
      myargv[argcount++] = "Internal argv[0]"; 

      while( argcount < maxargs &&  /* Still room in our array */
	     arg && 
	     *arg && 
	     ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	     )
	myargv[argcount++] = tmp;

      if( argcount < maxargs )      /* Exited not because the array was full */
	myargv[argcount] = 0;
      else                          /* Array was full, incredible but true */
	{
	  myargv[--argcount] = 0;
	  printf( "Warning, too many arguments given\n" );
	}

      lsftp_open_connection( myargv, argcount );

      freeargv = myargv;

      freeargv++; /* Skip first entry, we may not free that one */

      while( (s = *(freeargv++) ) )
	/* Free doesn't take const pointers */
	free( s );
      
      free( myargv );

      lsftp_handshake();
      lsftp_sftp_init();
      
      lsftp_do_cd( "" );                   /* Change to our home directory */
    }
  else
    {
      perror( "malloc failed" );
    }

  return 0;
}



int
com_cd(const char *arg, const char *command UNUSED) 
{
  char* tmp;

  int didcd = 0;

  int fail = 0;


  while( !fail &&
	 !didcd &&
	 arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      const char **mglob;
      const char *ptr = 0;
      char* tmp2 = lsftp_qualify_path( tmp );

      free( tmp );

      if( !tmp2 )
	return -1;

      mglob = lsftp_dc_r_startglob( tmp2, 0, 1 );
      free( tmp2 );

      if( mglob ) /* Glob returned allright? */
	ptr = *mglob;
      
      if( ptr )
	{
	  if( lsftp_dc_r_isdir( ptr ) < 1 )
	    printf( "Error: %s: Not a directory!\n", ptr );
	  else
	    lsftp_do_cd( ptr );
	  
	  didcd = 1;
	}

      lsftp_dc_endglob( mglob );
    }

  if( !didcd )
    return lsftp_do_cd( "" );

  /* FIXME: What does return value mean? */
  return 0;
}


int com_mkdir(const char *arg, const char *command) 
{
  char* tmp;

  int id;
  int mode = 0755;
  int fail = 0;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( !fail &&
	 arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      id = lsftp_do_mkdir( tmp, mode, command ); /* FIXME: Default mode? */
      fail = ( -1 == lsftp_await_command( id ) );

      free( tmp );
    }

  return 0;
}

static int
rm_file_or_folder(const char *path, const char *command, int recursive )
{
  int isdir = lsftp_dc_r_isdir(path);
  char *sub = 0;
  const char **subglob = 0;
  const char **orgglob = 0;
  const char *ptr;
  int err = 0;
  int id;

  if( !isdir ) /* Not a directory? */
    {
      id = lsftp_do_rm( path, command ); /* Delete it */
      
      if( id > 0 )
	{
	  lsftp_await_command( id );
	  return 0;
	}

      return id;
    }

  if( -1 == isdir ) /* Status checking failed? */
    {
      printf( "Error: can't determine status of %s.\n", path );
      return -1;
    }

  if(                     /* path is a directory, but user don't want to recurse */
     !recursive 
     )
    {
      printf( "Error: %s is a directory, try -r.\n", path );
      
      return -1;
    }
  
  sub = lsftp_concat( path, "/.*" );

  if( !sub )
    {
      perror( "concat failed" );
      return -1;
    }

  subglob = lsftp_dc_r_startglob( sub, 0, 0 );
  orgglob = subglob;

  if( subglob )
    while( !err && 
	   ( ptr = *(subglob++) ) ) 
      if(
	 !(                                         /* Not . or .. */
	   !strcmp( filename_part( ptr ), "." ) ||
	   !strcmp( filename_part( ptr ), ".." ) 
	   )
	 )
	err = rm_file_or_folder( ptr, command, recursive );
  
  lsftp_dc_endglob( orgglob );
  free( sub );
  
  if( err ) /* Bail out if failed */
    return -1; 
  
  /* Now do all files not starting with a period */
  
  sub = lsftp_concat( path, "/*" );
  
  if( !sub )
    {
      perror( "concat failed" );
      return -1;
    }
  
  subglob = lsftp_dc_r_startglob( sub, 0, 0 );
  orgglob = subglob;

  if( subglob ) /* Glob returned allright */
    while( !err && 
	   ( (ptr = *subglob++) ) ) 
      err = rm_file_or_folder( ptr, command, recursive );
  
  lsftp_dc_endglob( orgglob );
  free( sub );
  
  if( err ) /* Bail out if failed */
    return -1; 
  
  id = lsftp_do_rmdir( path, command );        /* Delete it */
  
  if( id > 0 )
    {
      lsftp_await_command( id );
      return 0;
     }
  else
    return id;
  
  return 0;
}


int
com_rm(const char *arg, const char *command) 
{
  char* tmp;

  int recurse = 0;
  int globbing = 0;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( lsftp_get_opts( "rg", tmp )  )
	switch( lsftp_get_opts( "rg", tmp ) )
	  {
	  case 3: /* Both set? */
	    recurse = !recurse;
	    globbing = !globbing;
	    break;
	  case 2: /* -r */
	    recurse = !recurse;
	    break;
	  case 1: /* -g */
	    globbing = !globbing;
	    break;
	  default:
	    printf( "Unknown option(s) %s\n", tmp );
	  }
      else 
	{
	  if( !globbing )
	    rm_file_or_folder( tmp, command, recurse );
	  else      /* Globbing desired */
	    {
	      const char **mglob;
	      const char **orgglob;
	      const char *ptr;
	    
	      char* tmp2 = lsftp_qualify_path( tmp );

	      if( !tmp2 )
		return -1;


	      mglob = lsftp_dc_r_startglob( tmp2, 0, 1 );
	      free( tmp2 );
	      orgglob = mglob;

	      if( mglob ) /* Glob returned allright? */		
		while( (ptr = *mglob++) ) 
		  rm_file_or_folder( ptr, command, recurse );
	    
	      lsftp_dc_endglob( orgglob );
	    }
	}

      free( tmp );
    }


  return 0;
}

int
com_ls(const char *arg, const char *command) 
{
  char* tmp;

  int didls = 0;
  int longlist = ls_longlist;
  int all = ls_all;
  int id;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( lsftp_get_opts( "la", tmp )  ) 
	switch( lsftp_get_opts( "la", tmp ) )
	  {
	  case 3: /* Both set? */
	    longlist = !longlist;
	    all = !all;
	    break;
	  case 2: /* -l */
	    longlist = !longlist;
	    break;
	  case 1: /* -a */
	    all = !all;
	    break;
	  default:
	    printf( "Unknown option(s) %s\n", tmp );
	  }
      else
	{
	  id = lsftp_do_ls( tmp, command, longlist, all ); /* ls does globbing by itself */
	  didls++;
	  
	  lsftp_await_command( id );
	} 

      free( tmp );
    }
  
  if( !didls ) /* No argument given? */
    {
      id = lsftp_do_ls( "", command, longlist, all );
      lsftp_await_command( id );
    }
  
/*    printf( "Returned from ls\n" ); */
  return 0;
}

int
com_set(const char *arg, const char *command UNUSED) 
{
  char* tmp;

  int show = 1;

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( !strncmp( "sloppy_complete", tmp, strlen( "sloppy_complete" ) ) )
	{
	  char c = tmp[ strlen( "sloppy_complete" ) ];

	  if( !c )
	    printf( "sloppy_complete: %d\n", sloppy_complete );
	  else 
	    if( '=' == c )
	      sloppy_complete = atoi( tmp + strlen( "sloppy_complete=" ) ); 
	    else
	      printf( "I don't understand what you want to do, try sloppy_complete=2\n" );
	  
	  show = 0;
	}

      if( !strncmp( "buggy_server_treshold", tmp, strlen( "buggy_server_treshold" ) ) )
	{
	  char c = tmp[ strlen( "buggy_server_treshold" ) ];

	  if( !c )
	    printf( "buggy_server_treshold: %d\n", buggy_server_treshold );
	  else 
	    if( '=' == c )
	      buggy_server_treshold = atoi( tmp + 
					    strlen( "buggy_server_treshold=" ) ); 
	    else
	      printf( "I don't understand what you want to do, "
		      " try buggy_server_treshold=2\n" );
	  
	  show = 0;
	}

      if( !strncmp( "ls_all", tmp, strlen( "ls_all" ) ) )
	{
	  char c = tmp[ strlen( "ls_all" ) ];

	  if( !c )
	    printf( "ls_all: %d\n", ls_all );
	  else 
	    if( '=' == c )
	      ls_all = atoi( tmp + strlen( "ls_all=" ) ); 
	    else
	      printf( "I don't understand what you want to do, try ls_all=2\n" );

	  show = 0;
	}

      if( !strncmp( "ls_longlist", tmp, strlen( "ls_longlist" ) ) )
	{
	  char c = tmp[ strlen( "ls_longlist" ) ];

	  if( !c )
	    printf( "ls_longlist: %d\n", ls_longlist );
	  else 
	    if( '=' == c )
	      ls_longlist = atoi( tmp + strlen( "ls_longlist=" ) ); 
	    else
	      printf( "I don't understand what you want to do, try ls_longlist=2\n" );

	  show = 0;
	}

      if( !strncmp( "gp_cont", tmp, strlen( "gp_cont" ) ) )
	{
	  char c = tmp[ strlen( "gp_cont" ) ];

	  if( !c )
	    printf( "gp_cont: %d\n", gp_cont );
	  else 
	    if( '=' == c )
	      gp_cont = atoi( tmp + strlen( "gp_cont=" ) ); 
	    else
	      printf( "I don't understand what you want to do, try gp_cont=2\n" );

	  show = 0;
	}

      if( show ) /* Obviously something was given, but we didn't recognise it */
	printf( "Unknown command or variable: %s\n", tmp );

      free( tmp );
    }
  
  if( show ) /* No argument given? */
    {
      printf( "sloppy_complete: %d\n", sloppy_complete );
      printf( "ls_all: %d\n", ls_all );
      printf( "ls_longlist: %d\n", ls_longlist );
      printf( "gp_cont: %d\n", gp_cont );
      printf( "buggy_server_treshold: %d\n", buggy_server_treshold );
    }
  
  return 0;
}

static int
get_file_or_folder(const char *arg, const char *command,
		   int cont, const char *destname )
{
  /* FIXME: Split into smaller functions get_file and get_folder used
   * by this function. */

  const char **dirinfo = NULL;
  const char **curdirinfo;

  char *localwd;
  char *remotewd;

  const char* realdest;
  char *tmp;

  struct sftp_attrib attrib;
  int id;
  int mode = 0700;
  int mask;
  int isdir = lsftp_dc_r_isdir( arg );
  int ret;

  if( -1 == isdir ) /* Unable to determine dir or not? */
    {
      if( !strncmp( filename_part( arg ), "*", 2 ) ||
	  !strncmp( filename_part( arg ), ".*", 3 )
	  )  /* Empty directory will make globbing fail => * (or .*) */
	return 0;
      
      printf( "Unable to determine status of %s\n", arg );
      return -1;
    }

  if( !isdir )  /* Not a directory? */
    {
      int id;
      int ret = -1;
      
      if( !destname )
	id = lsftp_do_get( filename_part( arg ), arg, command, cont ); 
      else
	id = lsftp_do_get( destname, arg, command, cont ); 

      if( id > 0) /* Not a failure? */
	ret = lsftp_await_command( id );

      return ret;
    }

  /* Obviously a directory */
  
  /* FIXME: What do we expect filename_part( "foobar/" ) to return? */
  
  id = lsftp_do_stat( arg, &attrib );
  
  if( id > 0) /* Not a failure? */
    {
      lsftp_await_command( id );

      if (attrib.flags & SSH_FILEXFER_ATTR_PERMISSIONS)
	mode = attrib.permissions;
      else
	/* FIXME: Default mode */
	mode = 0700;       
    }

  if( destname )              /* If given, use it */
    realdest = destname;
  else
    realdest = filename_part( arg ); /* Destination */

  if( !realdest )              /* Just to be safe */
    return -1;

  ret = lsftp_dc_l_isdir( realdest );

  if( !ret )       /* Exists, but not a dir? */
    {
      printf( 
	     "Error, %s exists but is not a directory\n", 
	      realdest
	     );
      return -1;
    }
  
  if( -1 == ret ) /* stat failed (probably because no such dir) */
  {
    mask = umask( 0 );
    umask( mask );


    if( mkdir( realdest, (mode & ~mask) | 0700 ) )  /* Make sure we have proper access */
      {
	/* Problem while creating the directory? */
	
	perror( "mkdir failed" );
	return -1;
      }
  }
  
  ret = lsftp_dc_l_isdir( realdest ); /* Check status */
  
  if( 1 != ret ) /* If not directory, bail out */
    {
      printf( "Unable to create directory %s\n", realdest );
      return ret;
    }
  
  localwd = canonicalize_file_name( realdest );

  if( !destname ) /* No destination given => likely first call,
		   * make sure arg is an absolute path.
		   */
    {
      id = lsftp_do_realpath( arg, &remotewd );
      
      if( id > 0) /* Not a failure? */
	{
	  lsftp_await_command( id );      
	}
    }
  else
    {
      /* destname is given, trust arg is an absoulte path */

      remotewd = strdup( arg );
    }


  if( !localwd )
    {
      printf( "Error: failed to determine local cwd\n" );
      return -1;
    }
  else if( !remotewd )
    {
      printf( "Error: failed to determine remote cwd\n" );
      return -1;
    }

  /* Place trailing slashes after both paths */

  tmp = lsftp_concat( localwd, "/" );
  free( localwd );

  if( !tmp )
    {
      perror( "Concat failed\n" );
      return -1;
    }

  localwd = tmp;

  tmp = lsftp_concat( remotewd, "/" );

  if( !tmp )
    {
      perror( "Concat failed\n" );
      return -1;
    }

  free( remotewd );
  remotewd = tmp;
  
  /* Do globbing */

  tmp = lsftp_concat( remotewd, "*" );
  
  if( tmp )
    dirinfo = lsftp_dc_r_startglob( tmp, 0, 1 );
  else
    return -1; /*Bail out on error */

  free(  tmp );
  
  tmp = lsftp_concat( remotewd, ".*" );
  if( tmp )
    dirinfo = lsftp_dc_r_contglob( tmp, dirinfo, 1 );
  else
    return -1; /*Bail out on error */
  
  free( tmp );

  if( dirinfo )
    {
      curdirinfo = dirinfo;
      
      while( *curdirinfo )
	{
	  const char *curentry = lsftp_skip_common( *curdirinfo, remotewd );   
	  
	  if( !(                                  /* Not . or .. */
		!strcmp( curentry, "." ) ||
		!strcmp( curentry, ".." ) 
		)
	      )
	    {
	      char *src;
	      char *dst;

	      src = lsftp_concat( remotewd, curentry );	      
	      dst = lsftp_concat( localwd, curentry );

	      if( src && dst )
		{
		  get_file_or_folder( src, command, cont, dst ); 
		  
		  free( src );
		  free( dst );
		}
	      else
		perror( "Concat failed" );	      
	    }
	  
	  curdirinfo++;  /* Next entry */
	}
      
      lsftp_dc_endglob( dirinfo );
    }
  
  free(  remotewd ); /* Free memory used by to hold remote wd */
  free(  localwd ); /* Free memory used by to hold remote wd */

  return 0;

}

int
com_get(const char *arg, const char *command) 
{
  int cont = gp_cont;
  char* tmp;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( lsftp_get_opts( "c", tmp )  ) 
	switch( lsftp_get_opts( "c", tmp ) )
	  {
	  case 1:
	    cont = !cont;
	    break;
	  default:
	    printf( "Unknown option(s) %s\n", tmp );
	  }
      else
	{
	  const char **glob;
	  const char **orgglob;
	  const char *ptr;
	  
	  char* tmp2 = lsftp_qualify_path( tmp );
	  
	  if( !tmp2 )
	    return -1;
	  
	  glob = lsftp_dc_r_startglob( tmp2, 0, 1 );
	  free( tmp2 );
	  
	  orgglob = glob;
	  
	  while( (ptr = *glob++) ) 
	    get_file_or_folder( ptr, command, cont, 0 );
	  
	  lsftp_dc_endglob( orgglob );
	}
      
      free( tmp );
    }
  return 0;
}

static int
put_file_or_folder(const char *arg, const char *command,
		   int cont, const char *destname )
{

  /* FIXME: Split into smaller functions put_file and put_folder used
   * by this function. */

  const char **dirinfo = NULL;
  const char **curdirinfo;

  char *localwd;
  char *remotewd;

  const char* realdest;
  char *tmp;
 
  struct stat st;
  int id;
  int mode = 0700;
  int isdir = lsftp_dc_l_isdir( arg );
  int err;
  int ret;

  if( -1 == isdir ) /* Unable to determine dir or not? */
    {
      if( !strncmp( filename_part( arg ), "*", 2 ) )
	/* Empty directory will make globbing fail => * */
	return 0;
      
      printf( "Unable to determine status of %s\n", arg );
      return -1;
    }

  if( !isdir )  /* Not a directory? */
    {
      int id;
      int ret = -1;
      
      if( !destname )
	id = lsftp_do_put( arg, filename_part( arg ), command, cont ); 
      else
	id = lsftp_do_put( arg, destname, command, cont ); 

      if( id > 0) /* Not a failure? */
	ret = lsftp_await_command( id );

      return ret;
    }

  /* Obviously a directory */
  
  /* FIXME: What do we expect filename_part( "foobar/" ) to return? */
  
  err = stat( arg, &st );

  if( -1 != err )      /* Not a failure? */
    mode = st.st_mode; /* Assign same modes as we have */
  else
    mode = 0700;       /* FIXME: Default mode */

  if( destname )              /* If given, use it */
    realdest = destname;
  else
    realdest = filename_part( arg ); /* Destination */

  if( !realdest )    /* Just to be safe, bail out on error */
    return -1;

  ret = lsftp_dc_r_isdir( realdest );
  
  if( !ret )       /* Exists, but not a dir? */
    {
      printf( 
	     "Error, %s exists but is not a directory\n", 
	     realdest
	     );
      return -1;
    }
  
  if( -1 == ret ) /* stat failed (probably because no such dir) */
  {
    id = lsftp_do_mkdir( realdest, mode, command ); /* Make directory */
    
    if( id > 0) /* Not a failure? */
      ret = lsftp_await_command( id );

    ret = lsftp_dc_r_isdir( realdest ); /* Check status */

    if( 1 != ret ) /* If not directory, bail out */
      {
	printf( "Unable to create directory %s\n", realdest );
	return ret;
      }

  }

  localwd = canonicalize_file_name( arg );

  if( !destname ) /* No destination given => Make path absolute */
    {
 
      id = lsftp_do_realpath( filename_part( arg ), &remotewd );
  
      if( id > 0) /* Not a failure? */
	{
	  lsftp_await_command( id );      
	}
    }
  else
    {
      /* destname is given, trust it is an absoulte path */

      remotewd = strdup( destname );
    }


  if( !localwd )
    {
      printf( "Error: failed to determine local cwd\n" );
      return -1;
    }
  else if( !remotewd )
    {
      printf( "Error: failed to determine remote cwd\n" );
      return -1;

    }


  /* Place trailing slashes after both paths */

  tmp = lsftp_concat( localwd, "/" );
  free( tmp );

  if( !tmp )
    {
      perror( "Concat failed\n" );
      return -1;
    }

  localwd = tmp;

  tmp = lsftp_concat( remotewd, "/" );

  if( !tmp )
    {
      perror( "Concat failed\n" );
      return -1;
    }

  free( remotewd );
  remotewd = tmp;
  
  /* Do globbing */

  tmp = lsftp_concat( localwd, "*" );
  
  if( tmp )
    dirinfo = lsftp_dc_l_startglob( tmp, 1 );
  else
    return -1; /*Bail out on error */

  free( tmp );
  
  tmp = lsftp_concat( localwd, ".*" );

  if( tmp )
    dirinfo = lsftp_dc_l_contglob( tmp, dirinfo, 1 );
  else
    return -1; /*Bail out on error */

  free( tmp );

  if( dirinfo )
    {
      curdirinfo = dirinfo;
      
      while( *curdirinfo )
	{
	  const char *curentry = lsftp_skip_common( *curdirinfo, localwd );   
	  
	  if( !(                                  /* Not . or .. */
		!strcmp( curentry, "." ) ||
		!strcmp( curentry, ".." ) 
		)
	      )
	    {
	      char *src;
	      char *dst;
	      
	      src = lsftp_concat( localwd, curentry );
	      dst = lsftp_concat( remotewd, curentry );

	      if( src && dst )
		{

		  put_file_or_folder( src, command, cont, dst ); 
		  
		  free( src );
		  free( dst );
		}
	      else
		perror( "Concat failed" );	      
	    }
	  
	  curdirinfo++;  /* Next entry */
	}
      
      lsftp_dc_endglob( dirinfo );
    }
  
  free( remotewd ); /* Free memory used by to hold remote wd */
  free( localwd ); /* Free memory used by to hold remote wd */

  return 0;
}    



int com_put(const char *arg, const char *command) 
{
  char* tmp;
  int cont = gp_cont;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( lsftp_get_opts( "c", tmp )  ) 
	switch( lsftp_get_opts( "c", tmp ) )
	  {
	  case 1:
	    cont = !cont;
	    break;
	  default:
	    printf( "Unknown option(s) %s\n", tmp );
	  }
      else
	{
	  const char **glob;
	  const char **orgglob;
	  const char *ptr;
	  
	  glob = lsftp_dc_l_startglob( tmp, 1 );
	  orgglob = glob;
	  
	  while( (ptr = *glob++) ) 
	    put_file_or_folder( ptr, command, cont, 0 );
	  
	  lsftp_dc_endglob( orgglob );
	}
      
      free( tmp );
    }
  
  return 0;
}


int com_chown(const char *arg, const char *command) 
{
  char* tmp;
  int gotuid = 0;
  int enough_parameters = 0;
  long newuid = 0;
  struct sftp_attrib attrib;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( !gotuid ) 
	{
	  if( isdigit( (unsigned char) tmp[0] ) ) /* Numeric GID? FIXME: Better check? */
	    newuid = atol( tmp );
	  else
	    {
	      struct passwd* pwd;

	      printf( "Warning: performing LOCAL lookup of user %s\n", tmp ); /* Warn user */
	    
	      pwd = getpwnam( tmp );

	      if( pwd ) /* OK? */
		newuid = pwd->pw_uid; /* get uid */
	      else
		{
		  printf( "Lookup failed!\n" );
		  return -1; /* Signal failure */
		}
	    }

	  gotuid = 1;
	}
      else
	{
	  int id;
	
	  const char **glob;
	  const char **orgglob;
	  const char *ptr;

	  enough_parameters++;
	
	  glob = lsftp_dc_l_startglob( tmp, 1 );
	  orgglob = glob;
	
	  while( (ptr = *glob++) )
	    {
	      id = lsftp_do_stat( ptr, &attrib );
	      if( id > 0) /* Not a failure? */
		{
		  lsftp_await_command( id );

		  if (! (attrib.flags & SSH_FILEXFER_ATTR_UIDGID))
		    printf("Remote system doesn't support uids?\n");
		  
		  lsftp_do_chown(ptr, newuid, attrib.gid, command );
		}
	    }
	
	  lsftp_dc_endglob( orgglob );
	}
	
      free( tmp );
    }

  if( !enough_parameters )
    {
      printf( "chown needs at least two parameters\n" );
      return -1;
    }

  return 0;
}


int com_chgrp(const char *arg, const char *command) 
{
  char* tmp;
  int gotgid = 0;
  long newgid = 0;
  int enough_parameters = 0;
  struct sftp_attrib attrib;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      if( !gotgid )  /* First time? */
	{
	  if( isdigit( (unsigned char) tmp[0] ) ) /* Numeric GID? FIXME: Better check? */
	    newgid = atol( tmp );
	  else
	    {
	      struct group* gr;

	      printf("Warning: performing LOCAL lookup of group %s\n", tmp ); /* Warn user */
	    
	      gr = getgrnam( tmp );

	      if( gr ) /* OK? */
		newgid = gr->gr_gid; /* Get gid */
	      else
		{
		  printf("Lookup failed!\n");
		  return -1; /* Signal failure */
		}
	    }

	  gotgid = 1;
	}
      else
	{
	  int id;

	  enough_parameters++;
	  id = lsftp_do_stat( tmp, &attrib );

	  if( id > 0) /* Not a failure? */
	    {
	      lsftp_await_command( id );

	      if (! (attrib.flags & SSH_FILEXFER_ATTR_UIDGID))
		printf("Remote system doesn't support gids?\n");
	      lsftp_do_chown( tmp, attrib.uid, newgid, command );
	    }
	}

      free( tmp );
    }

  if( !enough_parameters )
    {
      printf( "chgrp needs at least two parameters\n" );
      return -1;
    }

  return 0;
}

int
com_jobs(const char *arg, const char *command UNUSED)
{
  int listjobs = 0;
  char* tmp;

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      int jobid;

      jobid = atoi( tmp );
      lsftp_cb_status( jobid ); 
      listjobs++;

      free( tmp );
    }

  if( !listjobs )
    lsftp_cb_list();

  return 0;
}


int
com_pwd(const char *arg UNUSED, const char *command UNUSED) 
{
  char* pwd;

  pwd = lsftp_pwd();

  if( pwd )
    printf( "%s\n", pwd );
  else
    printf( "Current working directory not available\n" );

  return 0;
}

int
com_mv(const char *arg, const char *command) 
{
  char* tmp;
  char* dst = NULL;
  const char* orgarg = arg;
  int i = 0;
  int j = 0;
  const char** glob;
  const char** orgglob;
  const char* ptr;  
  const char* lastptr = NULL;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      char* tmp2 = lsftp_qualify_path( tmp );
      free( tmp );

      if( !tmp2 )
	return -1;

      glob = lsftp_dc_r_startglob( tmp2, 0, 1 );
      free( tmp2 );

      orgglob = glob;
      lastptr = NULL;

      while( (ptr = *glob++) ) 
	{
	  lastptr = ptr;
	  i++;
	}

      if( lastptr ) /* Anything matched? */
	{
	  
	  if( dst )
	    free( dst );
	  
	  dst = strdup( lastptr ); /* dst should be a copy of the last argument */	 
	}
      
      lsftp_dc_endglob( orgglob );
    }
  
  if( i <= 1 ) /* To few arguments */
    {
      printf( "mv needs at least two parameters\n" );
      return -1;
    }
  
  if( !dst ) /* Found no destination? */
    return -1;
  
  arg = orgarg;

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      char* tmp2 = lsftp_qualify_path( tmp );

      free( tmp );

      if( !tmp2 )
	return -1;

      glob = lsftp_dc_r_startglob( tmp2, 0, 1 );
      free( tmp2 );

      orgglob = glob;
      
      while( (ptr = *glob++) ) 
	{
	  j++;
	  if( j != i ) /* Not the last argument? */
	    {
	      /* More than one source and one destination => we should append src to dst/.
	       * Same goes if dst is a directory */

	      if( 2 != i ||
		  ( lsftp_dc_r_isdir( dst ) > 0 )
		  ) 
		{
		  char *tmp1 = lsftp_concat( dst, "/" );
		  char *tmp2 = NULL;

		  if( tmp1 )
		    tmp2 = lsftp_concat( tmp1, filename_part( ptr ) );          /* And make the new path for ptr (dst/ptr or so) */
		  
		  if( tmp2 )		    
		    lsftp_do_mv( ptr, tmp2, command );
		  else
		    perror( "problem while working with strings" );
	
		  free( tmp1 );
		  free( tmp2 );
		}
	      else
		lsftp_do_mv( ptr, dst, command );
	    }
	}

      lsftp_dc_endglob( orgglob );
      }

  free( dst );

  return 0;
}

int
com_ln(const char *arg, const char *command) 
{
  char* link = NULL;
  char* tmp;

  const char *orgarg = arg;
  int i = 0;
  int j = 0;
  const char **glob;
  const char **orgglob;
  const char *ptr;  
  const char *lastptr;

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      char* tmp2 = lsftp_qualify_path( tmp );

      free( tmp );

      if( !tmp2 )
	return -1;

      glob = lsftp_dc_r_startglob( tmp2, 0, 1 );
      free( tmp2 );

      orgglob = glob;

      if( !glob )
	return -1;

      lastptr = NULL;

      while( (ptr = *glob++) ) 
	{
	  lastptr = ptr;
	  i++;
	}

      if( lastptr )
	{
	  if( link )
	    free( link );

	  link = strdup( lastptr);
	}

      lsftp_dc_endglob( orgglob );
    }
  
  if( i < 1 ) /* To few arguments */
    {
      printf( "ln needs at least one parameters\n" );
      return -1;
    }
  
  if( 1 == i ) /* Exactly one argument? */
    {
      /* User hopefully wants to create a symlink to that argument in the current directory 
       *
       * So linkpath will be the the filename part of the argument,
       * and linkpath the full argument.
       * 
       */

      lsftp_do_ln( filename_part( link ), link, command );
      free( link );
      return 0;
   }

  /* Many arguments 
   *
   * The user wants to make links 
   *
   */
  
  arg = orgarg;

  while( arg && 
	 *arg && 
	 ( arg = lsftp_s_strtok( arg," \n\t\r", &tmp ) ) 
	 )
    {
      glob = lsftp_dc_r_startglob( tmp, 0, 1 );
      orgglob = glob;
      
      while( (ptr = *glob++) ) 
	{
	  j++;
	  if( j != i ) /* Not the last argument? */
	    {
	      if( 1 == lsftp_dc_r_isdir( link ) ) /* Link is a directory?  */
		{
		  char *tmp1 = lsftp_concat( link, "/" );
		  char *tmp2 = NULL;

		  if( tmp1 ) /* And make the new path for ptr (link/ptr or so) */
		    tmp2 = lsftp_concat( tmp1, filename_part( ptr ) ); 
		  
		  if( tmp2 )		    
		    lsftp_do_ln( link, tmp2, command );
		  else
		    perror( "problem while working with strings" );
	
		  free( tmp1 );
		  free( tmp2 );
		}
	      else
		lsftp_do_ln( link, ptr, command );
	    }
	}

      lsftp_dc_endglob( orgglob );

      free( tmp );
    }


  return 0;
}

int
com_lcd(const char *arg, const char *command UNUSED) 
{
  char* tmp;
  const char **gldata = NULL;
  int ret;
  

  arg = lsftp_s_strtok( arg, " \n\t\r", &tmp );  /* Get argument */    
   
  if( tmp )
    gldata = lsftp_dc_l_contglob( tmp, gldata, 1 );
  
  free( tmp );

  if( !gldata ) /* glob failed? */
    return -1;

  switch( lsftp_dc_numglob( gldata )  )
    {
    case 0:        /* No path given, go home */
      ret = chdir("~");
      break;
	
    case 1:
      ret = chdir( gldata[0] );
      break;
      
    default:
	printf( "lcd: To many arguments (expected one, got %d)\n", 
		lsftp_dc_numglob( gldata )
		);
    }
  
  lsftp_dc_endglob( gldata );
  
  return 0;
}

int
com_lrm(const char *arg UNUSED, const char *command UNUSED) 
{
  printf("lrm\n");

  if( !lsftp_connected() )   /* Bail out if not connected */
    return com_disconnected();

  return 0;
}

/* FIXME: Use idcache.c? */
static const char *
uidstring( int uid )
{
  static char uids[10];
  struct passwd* p = getpwuid( uid );

  if( p ) /* Negative result */
    return p->pw_name;

  snprintf( uids, sizeof(uids), "%d", uid );
  uids[sizeof(uids)-1] = 0;
  
  return uids;
}

static const char *
gidstring( int gid )
{
  static char gids[10];
  struct group* g = getgrgid( gid );

  if( g )
    return g->gr_name;

  snprintf( gids, sizeof(gids), "%d", gid );
  gids[sizeof(gids)-1] = 0;

  return gids;
}

/* FIXME: Use filemode.c? */
int
com_lls(const char *arg, const char *command UNUSED) 
{
  char* tmp;
  const char **gldata = 0;
  struct stat st;
  int i = 0;

  int longlist = ls_longlist;
  int all = ls_all;

  int args = 0;
  int err;

  while( 
	( arg =
	  lsftp_s_strtok( arg, " \n\t\r", &tmp )
	  )
	) /* Get argument */    
    {
      if( lsftp_get_opts( "la", tmp )  ) 
	switch( lsftp_get_opts( "la", tmp ) )
	  {
	  case 3: /* Both set? */
	    longlist = !longlist;
	    all = !all;
	    break;
	  case 2: /* -l */
	    longlist = !longlist;
	    break;
	  case 1: /* -a */
	    all = !all;
	    break;
	  default:
	    printf( "Unknown option(s) %s\n", tmp );
	  }
      else
	{
	  gldata = lsftp_dc_l_contglob( tmp, gldata, 1 );
	  args++;
	}

      free( tmp );
    }

  if( !args )
    {
       gldata = lsftp_dc_l_contglob( ".*", gldata, 1 );
       gldata = lsftp_dc_l_contglob( "*", gldata, 1 );
    }

  if( gldata )

    while( gldata[i] )
      {
	err = stat( gldata[i], &st );

	if( err )
	  printf( "%s: not found\n", gldata[i] );
	else if( all || ( gldata[i][0] != '.' ) )
	  {
	    /* Show all files or not staring with . */
	    if( ! longlist )
	      printf( "%s\n", gldata[i] );
	    else
	      {
		int j;
		char modestring[11];

		for( j=0; j < 11; j++ )
		  modestring[j] = '-';
		modestring[10] = 0;

		if( S_ISDIR( st.st_mode ) )
		  modestring[0] = 'd';
		else if( S_ISBLK( st.st_mode ) )
		  modestring[0] = 'b';
		else if( S_ISCHR( st.st_mode ) )
		  modestring[0] = 'c';
		else if( S_ISSOCK( st.st_mode ) )
		  modestring[0] = 's';
		else if( S_ISFIFO( st.st_mode ) )
		  modestring[0] = 'p';

		if( st.st_mode & S_IRUSR )
		  modestring[1] = 'r';
		if( st.st_mode & S_IWUSR )
		  modestring[2] = 'w';
		if( st.st_mode & S_IXUSR )
		  modestring[3] = 'x';

		if( st.st_mode & S_IRGRP )
		  modestring[4] = 'r';
		if( st.st_mode & S_IWGRP )
		  modestring[5] = 'w';
		if( st.st_mode & S_IXGRP )
		  modestring[6] = 'x';

		if( st.st_mode & S_IROTH )
		  modestring[7] = 'r';
		if( st.st_mode & S_IWOTH )
		  modestring[8] = 'w';
		if( st.st_mode & S_IXOTH )
		  modestring[9] = 'x';

		if( st.st_mode & S_ISUID )
		  {
		    if( st.st_mode & S_IXUSR )
		      modestring[3] = 's';
		    else
		      modestring[3] = 'S';
		  }
		
		if( st.st_mode & S_ISGID )
		  {
		    if( st.st_mode & S_IXGRP )
		      modestring[6] = 's';
		    else
		      modestring[6] = 'S';
		  }
		
		if( st.st_mode & S_ISVTX )
		  {
		    if( st.st_mode & S_IXOTH )
		      modestring[6] = 't';
		    else
		      modestring[6] = 'T';
		  }
		/* FIXME: Doesn't handle off_t larger than long */
		/* FIXME: st_nlink is a long on Solaris */
		printf( 
		       "%s %4ld %-8s %-8s %8ld ", 
		       modestring, 
		       (long) st.st_nlink, 
		       uidstring( st.st_uid ), 
		       gidstring( st.st_gid ),
		       (long) st.st_size 
		       );
		
		printf( "%s\n", gldata[i] );

	      }
	  }
	i++;
      }
  
  lsftp_dc_endglob( gldata );
  
  return 0;
}

int
com_lpwd(const char *arg UNUSED, const char *command UNUSED) 
{
  char* pwd;
  char* ret;
  int size;

#ifdef PATH_MAX
  size = PATH_MAX;
#else
  size = 8192;
#endif

  /* FIXME: Detect and use if getcwd can allocate the buffer by itself */

  pwd = malloc( size+1 );

  if( !pwd )
    return -1;

  ret = getcwd( pwd, size ); /* Get where we are now */
  pwd[ size ] = 0; /* Make certain it is NULL-terminated*/

  if( !ret )            /* Failed? */
    {
      perror( "getcwd failed" );
      free( pwd );
      return -1;
    }

  printf( "%s\n",pwd ); /*  Print it */
  free( pwd );
  return 0;      
  
}

int
com_lmv(const char *arg UNUSED, const char *command UNUSED) 
{
  printf("lmv\n");

  return 0;
}


int
handle_command(const unsigned char *s)
{
  const char *entered_cmd = s;
  char* tmp;
  char *cmdname;
  int i=0;
    
  s = lsftp_s_skip(s," \n\t\r"); /* Skip any initial whitespace */
  
  if( !s || !(*s) ) /* s is NULL or contains only whitespace */
    /* No command */
    return  NOCOMMAND;

  /* Special cases for ! and ? commands - allow arguments without whitespace separator */

  if( s && s[0] == '!' && isalnum( s[1] ) ) /* Commands is !x... ? */
    return com_escape( s+1, entered_cmd );

  if( s && s[0] == '?' && isalnum( s[1] ) ) /* Commands is ?x... ? */
    return com_longhelp( s+1, entered_cmd );

  /* Get the first word from the line (the command) */

  s = lsftp_s_strtok(s," \n\t\r", &tmp );
  
  if(!s) /* s equals null ? */
    return NOCOMMAND;

  while( 
	( cmdname = commands[i].name ) 
	)
    {
      int tmplen = strlen( tmp );
      int neededlen = commands[i++].uniquelen;
      
      if( neededlen <= tmplen  &&  /* Sufficiently long string? */
	  !strncmp( cmdname, tmp, tmplen )  /* Matching command? */                
	  )          
	{
	  free( tmp );

	  return commands[i-1].func( 
				    lsftp_s_skip(s," \n\t\r"), 
				    entered_cmd 
				    );
	}
    }

  
  printf("Unknown command %s\n", tmp);
  
  free( tmp );
  return -UNKNOWNCOMMAND;

}

