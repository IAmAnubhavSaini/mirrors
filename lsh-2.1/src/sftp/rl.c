/*
 * @(#) $Id: c87f4b88ad2bac87e845b8d55b80c5452d684026 $
 *
 * rl.c
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

int lsftp_rl_line_done = 0;
char* lsftp_rl_line = 0;

int sloppy_complete = 2;


#include "rl.h"

#ifdef WITH_READLINE
#ifdef HAVE_RL_FILENAME_COMPLETION_FUNCTION
# define RL_FILENAME_COMPLETION_FUNCTION rl_filename_completion_function
#else /* HAVE_RL_FILENAME_COMPLETION_FUNCTION */
# ifdef HAVE_FILENAME_COMPLETION_FUNCTION
#  define RL_FILENAME_COMPLETION_FUNCTION filename_completion_function
# else
#  if __GNUC__
#   warning "lsftp doesn't work with your readline library, please disable readline and contact the author."
#  endif
#  undef WITH_READLINE
# endif /* HAVE_FILENAME_COMPLETION_FUNCTION */
#endif /* HAVE_RL_FILENAME_COMPLETION_FUNCTION */
#endif

#ifdef WITH_READLINE
#ifdef HAVE_RL_COMPLETION_MATCHES
# define RL_COMPLETION_MATCHES rl_completion_matches
#else /* HAVE_RL_COMPLETION_MATCHES */
# ifdef HAVE_COMPLETION_MATCHES
#  define RL_COMPLETION_MATCHES completion_matches
# else
#  if __GNUC__
#   warning "lsftp doesn't work with your readline library, please disable readline and contact the author."
#  endif
#  undef WITH_READLINE
# endif /* HAVE_COMPLETION_MATCHES */
#endif /* HAVE_RL_COMPLETION_MATCHES */
#endif

#ifdef WITH_READLINE
#ifdef HAVE_RL_CHAR_IS_QUOTED_P
# define RL_CHAR_IS_QUOTED rl_char_is_quoted_p
#else /* HAVE_RL_CHAR_IS_QUOTED */
# ifdef HAVE_CHAR_IS_QUOTED
#  define RL_CHAR_IS_QUOTED char_is_quoted
# else
#  if __GNUC__
#   warning "lsftp doesn't work with your readline library, please disable readline and contact the author."
#  endif
#  undef WITH_READLINE
# endif /* HAVE_CHAR_IS_QUOTED */
#endif /* HAVE_RL_CHAR_IS_QUOTED */
#endif

#ifdef WITH_READLINE

#include "str_utils.h"

static int lsftp_rl_line_inited=0;

/* FIXME: review readline support, proper way to do it? */



void lsftp_rl_init()
{
  /* Basic readline initialization goes here */

# ifdef WITH_HISTORY

  char* histfname = lsftp_rl_history_fname();

  using_history(); /* Use history function */

  if( histfname )
    {
      read_history( histfname ); /* Try to load old history */
      free( histfname );
    }

# endif /* WITH_HISTORY */

  /* Allow conditional parsing of the ~/.inputrc file. */
  rl_readline_name = "lsftp";

  rl_completer_quote_characters = "\'\"";  /* Quotes are performed by ", ' and \ */
  rl_basic_word_break_characters =" \n\t"; /* Only whitespace break words */

  RL_CHAR_IS_QUOTED = char_quoted;
  rl_attempted_completion_function = (CPPFunction *)lsftp_rl_completion;


  interactive = 1; /* Set the interactive flag in the main program */
}

void lsftp_rl_exit()
{
  /* Called to finish business with readline, only saves history and 
   * makes sure we don't have no callback handler */

# ifdef WITH_HISTORY
  char* histfname = lsftp_rl_history_fname();

  using_history(); /* Use history function */

  if( histfname )
    {
      write_history( histfname ); /* Try to load old history */
      free( histfname );
    }

# endif /* WITH_HISTORY */
  rl_callback_handler_remove();

}



int char_quoted( char* text, int index )
{
  int i = 0;

  int literalq = 0;
  int nextliteral = 0;
  int normalq = 0;

  while( i < index )  /* Found the correct position yet? */
    {
      
      if( nextliteral )
	{
	  nextliteral = 0;
	  i++;
	}
      else
	{
	  if( literalq )                /* In a 'foo block? */
	    {
	      if( '\'' == text[i] )      /* Finishing bar' ? */
		{
		  literalq = 0;
		  i++;
		}
	    }
	  else
	    {
	      /* Neither nextliteral or literalq */
	      
	      switch( text[i] )
		{
		case '\'':
		  literalq = 1;
		  i++;
		  break;
		  
		case '\\':
		  nextliteral = 1;
		  i++;
		  break;
		  
		default: /* Not a quote */
		  i++;
		}
	    }
	}
    }
    

    return normalq || nextliteral || literalq;
}





char** lsftp_rl_completion(char* text, int start, int end)
{
  char** matches=NULL;
  int s;

  /* If this word is at the start of the line, then it is a command to
   * complete. 
   */

  rl_completion_append_character = ' '; /* Trailing space after word */

  s = lsftp_s_skip( rl_line_buffer, " \n\t\r" ) - rl_line_buffer;

  if ( start == s ) /* The first word should be a command, even with spaces in front */
    {
      matches = RL_COMPLETION_MATCHES( 
				      text, 
				      lsftp_rl_command_generator 
				      );  
    }
  else
    {
      /* We're not completing the command */
      char* tmp;
      int i=0;
      char* cmdname;

      /* Get the first word from the line (the command) */

      lsftp_s_strtok( rl_line_buffer+s," \n\t\r", &tmp );

      while( (cmdname = commands[i].name) )
	{
	  if ( ! strncmp( cmdname, tmp, commands[i++].uniquelen ) )       /* We've got the command? */
	    switch ( commands[i-1].arg_type )
	      {
		
	      case COMMAND: /* Command takes a command as argument? */
		matches = RL_COMPLETION_MATCHES( 
						text, 
						lsftp_rl_command_generator
						);
		break;
		
	      case REMOTEFILE:  /* Remote file or dir as argument? */
		matches = RL_COMPLETION_MATCHES( 
						text, 
						lsftp_rl_remotefile_generator
						);
		break;
		
	      case LOCALFILE:
		matches = RL_COMPLETION_MATCHES(
						text, 
						RL_FILENAME_COMPLETION_FUNCTION
						);
		break;
		
	      default:
	      case NOARG:  /* Doesn't take any argument? */
		matches = RL_COMPLETION_MATCHES(
						text, 
						lsftp_rl_no_generator 
						); 
		break;
	      }
	  
	 
	}

      free( tmp );

      if( !matches ) /* No matching command? */
	  {
	    /* No command yet given - complete command */
	    matches = RL_COMPLETION_MATCHES( 
					  text, 
					  lsftp_rl_command_generator 
					  );  
	  }
    }

  return (matches);
}



/* Generator function for command completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
/* FIXME: type? */
char* lsftp_rl_command_generator (char* text, int state)
{
  static int list_index, len;
  char* name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state)
    {
      list_index = 0;
      len = strlen (text);
    }

  /* Return the next name which partially matches from the command list. */
  while ((name = commands[list_index].name))
    {
      list_index++;

      if (strncmp (name, text, len) == 0)
        return strdup(name);
    }

  rl_attempted_completion_over = 1; /* Don't do local filename completion */

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}



/* Generator function for remote completion.  STATE lets us know whether
   to start from scratch; without any state (i.e. STATE == 0), then we
   start at the top of the list. */
char* lsftp_rl_remotefile_generator( char* text, int state )
{
  static int list_index;
  char* name;
  static const char** glob;
  static int unqualify = 0;

  /* If this is a new word to complete, initialize now. */

  if( !state )
    {
      unqualify = 0;

      name = lsftp_concat( text, "*" ); /* Make name contain text* */ 
      
      if( !lsftp_path_is_absolute( text ) ) /* Relative path? */
	{
	  char* tmp = lsftp_qualify_path( name ); /* Make absoulte */

	  if( !tmp )
	    return 0;

	  free( name );
	  name = tmp;
	  unqualify = 1;
	}

      list_index = 0;
      glob = 0;

      switch( sloppy_complete )
	{
	case 0: /* No sloppy completion */

	  if( name ) /* concat OK? */
	    /* Do non-sloppy globbing */
	    glob = lsftp_dc_r_startglob( name, 0, 0 ); 

	  break;

	case 1: /* Sloppy completion */

	  if( name ) /* concat OK? */
	    /* Do sloppy globbing */
	    glob = lsftp_dc_r_startglob( name, 1, 0 ); 

	  break;

	default:
	case 2: /* Try sloppy completion */
	  {
	    int matches = 0;

	    if( name ) /* concat OK? */
	      /* Do sloppy globbing */
	      glob = lsftp_dc_r_startglob( name, 1, 0 ); 

	    matches = lsftp_dc_numglob( glob );

	    if( !matches ) /* Glob gave no matches? */
	      {
		lsftp_dc_endglob( glob ); /* Free old glob */
		/* Do non-sloppy globbing */
		glob = lsftp_dc_r_startglob( name, 0, 0 ); 
	      }
	  }
	}

      
      free( name );

    }

  /* Return the next match */

  if( glob[list_index] ) /* Not the end? */
    {
      char* tmp;
      char* tmp2;
      char* nfree = strdup( glob[list_index++] );
      const char* name = nfree; 

      if( unqualify )
	name = lsftp_unqualify_path( nfree );

      if( !lsftp_dc_r_isdir( name ) )
	{
	  char* s = lsftp_quote( name );
	  free( nfree );
	  return s;
	}

      /* Directory - add trailing slash */

      rl_completion_append_character = 0; /* No trailing space */

      tmp = lsftp_concat( name, "/" ); 
      
      free( nfree );

      if( !tmp ) /* concat failed? */ 
	return 0;
      
      tmp2 = lsftp_quote( tmp );
      free( tmp );
      
      return tmp2;
    }

  rl_attempted_completion_over = 1; /* Don't do local filename completion */

  lsftp_dc_endglob( glob ); /* Free memory used by glob */

  list_index = 0;
  glob = 0;

  /* If no names matched, then return NULL. */
  return ((char *)NULL);
}




/* Generator function for NOARG completion. 
   no arguments => nothing matches */ 

/* FIXME: Type? */ 
char* lsftp_rl_no_generator (char* text, int state)
{
  if(!state) /* On the first call, return the empty string */
    return strdup("");

  rl_attempted_completion_over = 1;

  return ((char *)NULL);
}



void lsftp_rl_unuse_line(char* line)
{
  /* A line has been malloced, so we free it */

  free(line);
}

void lsftp_rl_init_get_line(char* prompt)
{
  if( ! lsftp_rl_line_inited )
    {
      rl_callback_handler_install (prompt, &lsftp_rl_lhandler);
      
      lsftp_rl_line_inited=1; /* We've started */
      lsftp_rl_line_done=0;
    }
}


void lsftp_rl_check_input()
{
  rl_callback_read_char();
}

char* lsftp_rl_get_line()
{
  lsftp_rl_init_get_line(""); 

  while(! lsftp_rl_line_done)
    {
      lsftp_rl_check_input();
      
    }

  return lsftp_rl_line;
}

void lsftp_rl_lhandler(char* line)
{
  rl_callback_handler_remove();
  lsftp_rl_line_done=1;
  lsftp_rl_line_inited=0; 
  lsftp_rl_line=line;

# ifdef WITH_HISTORY
  if ( line && *line) /* Got a line and it's not empty? */
    add_history(line); /* Save to history */
# endif

}

char* lsftp_rl_history_fname()
{
  char* username;
  char* fname;

  if( mgetenv( LSFTP_HISTNAME_VAR ) )           /* If the filename is given */
    return strdup( mgetenv( LSFTP_HISTNAME_VAR ) );

  if( mgetenv( "HOME" ) )   /* HOME is set? */
    {
      int homelen = strlen( mgetenv( "HOME" ) );
      int len = homelen + strlen( LSFTP_HIST_FILENAME ) + 1; /* We'll have $HOME/.lsftp */

      fname = malloc( len );

      if( !fname ) /* Failed to allocate memory? */
	return 0;

      strncpy( fname, mgetenv( "HOME" ), homelen ); /* Copy $HOME */
      strncpy( fname + homelen, LSFTP_HIST_FILENAME, len - homelen ); /* Copy filename part */
      fname[len-1] = 0;                /* Make sure it's terminated */
      
      return fname;
    }
  
  return 0;
}


#else /* ifdef WITH_READLINE */

/* Readline not found */

inline void
lsftp_rl_init_get_line(char* prompt UNUSED)
{
  /* Do nothing */
}

char *
lsftp_rl_get_line(void )
{
     /*

      */
  return 0;
}

void
lsftp_rl_check_input(void)
{
}

void
lsftp_rl_unuse_line(char* line)
{
  free(line);
}

void
lsftp_rl_init(void)
{
  interactive=0;
}

void
lsftp_rl_exit(void)
{
}

char*
lsftp_rl_history_fname(void)
{
  return 0;
}

#endif /* ifdef WITH_READLINE */

