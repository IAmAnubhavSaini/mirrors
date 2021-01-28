/*
 * @(#) $Id: dcbd6df3c0ceb3eeb5427adb8da85b568c376221 $
 *
 * dc.c - dircache functions
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

#include "dc.h"

static int dc_entries = 0;
static struct lsftp_dc_s* lsftp_dircache = 0;

int lsftp_dc_init( int new_dc_entries )
{
  int i;

  /* We flush the cache when resulting */

  if( lsftp_dircache )
    lsftp_dc_uninit();

  dc_entries = new_dc_entries;

  lsftp_dircache = malloc( sizeof( struct lsftp_dc_s ) * dc_entries );

  if( !lsftp_dircache ) /* malloc failed? */
    return -1;

  for( i = 0; i < dc_entries; i++ )
    {
      lsftp_dircache[i].name = 0;
      lsftp_dircache[i].hash = 0;
      lsftp_dircache[i].has_permissions = 0;
      lsftp_dircache[i].permissions = 0;
    }

  return 1;
}

void lsftp_dc_uninit()
{
  int i;

  if( !lsftp_dircache )
    return;

  for( i = 0; i < dc_entries; i++ )
    free( lsftp_dircache[i].name );

  free( lsftp_dircache );

  lsftp_dircache = 0;
  dc_entries = 0;
  
  return;
}

int
lsftp_dc_index(const char* name)
{
  int h;
  int i;

  if( !lsftp_dircache )
    return -1;

  h = lsftp_dc_hash( name );

  for( i = 0; i < dc_entries; i++ )
    if( lsftp_dircache[i].hash == h &&                /* Hash is correct? */
	!strcmp( lsftp_dircache[i].name, name )
	)
      return i;

  return -1;   /* No match */
}

static int
lsftp_dc_free_index(void)
{
  int i;

  if( !lsftp_dircache )
    return -1;
  
  for( i = 0; i < dc_entries; i++ )
    if( lsftp_dircache[i].hash == 0 &&
	lsftp_dircache[i].name == 0
	)
      return i;

  return -1;   /* No match */
}

static int
lsftp_dc_make_index(void)
{
  static int nextexpire = 0; /* FIXME: How choose which to expire? */
  int i = nextexpire++;

  if( !lsftp_dircache )
    return -1;

  if( nextexpire == dc_entries ) /* Filled? */
    nextexpire = 0;

  free( lsftp_dircache[i].name ); 

  lsftp_dircache[i].name = 0;
  lsftp_dircache[i].hash = 0;
  lsftp_dircache[i].has_permissions = 0;
  lsftp_dircache[i].permissions = 0;

  return i;
}







int lsftp_dc_notice( char* name, struct sftp_attrib* a )
{
  char* s;
  int i;

  if( !lsftp_dircache )
    return -1;

  s = strdup( name );
  
  if( !s ) /* Malloc failed? */
    return -1;

  i = lsftp_dc_free_index();

  if( -1 == i ) /* All slots taken? */
    i = lsftp_dc_make_index();

  lsftp_dircache[i].name = s;
  lsftp_dircache[i].hash = lsftp_dc_hash( s );
  lsftp_dircache[i].has_permissions
    = sftp_attrib_perms(a, &lsftp_dircache[i].permissions);
    
  return 0;
}

int lsftp_dc_remove( char* name )
{
  int i;

  if( !lsftp_dircache )
    return -1;
  
  i = lsftp_dc_index( name );

  if( -1 == i ) /* Not found? */
    return 0;

  free( lsftp_dircache[i].name );

  lsftp_dircache[i].name = 0;
  lsftp_dircache[i].hash = 0;
  lsftp_dircache[i].has_permissions = 0;
  lsftp_dircache[i].permissions = 0;
  
  return 1;
}

int
lsftp_dc_hash(const char* name UNUSED)
{
  /* FIXME: Calculate */

  return 5;
}

int
lsftp_dc_l_isdir(const char *name)
{
  /*
   * Check whatever a local name is a directory 
   *
   * Return 1 if it is, 0 if it isn't, -1 if we couldn't find out
   *
   */

  struct stat st;
  int failed;

  failed = stat( name, &st ); 

  if( failed )
    return -1; /* stat failed */
  
  return S_ISDIR( st.st_mode ); 
}

int
lsftp_dc_r_isdir(const char* name)
{
  /*
   * Check whatever a remote name is a directory 
   *
   * Return 1 if it is, 0 if it isn't, -1 if we couldn't find out
   *
   */

  int i;
  int id;
  struct sftp_attrib attrib;

/*    printf( "Checking directory status of %s: ", name ); */

  if( lsftp_dircache )
    {
      i = lsftp_dc_index( name );
      
      if( -1 != i &&                            /* Found it? */
	  lsftp_dircache[i].has_permissions &&
	  !(lsftp_dircache[i].permissions & S_IFLNK)
	  )
	/*      { */
	/*        printf( "%d (from cache), posixperms are %d\n",  S_ISDIR( lsftp_dircache[i].posixperms ),  */
	/*  	      lsftp_dircache[i].posixperms ); */
	return S_ISDIR(lsftp_dircache[i].permissions);
      /*      } */
      
    }
  
  /* Not found in cache (or link), we need to stat */
  
  id = lsftp_do_stat( name, &attrib );
  
  if( id > 0) /* Not a failure? */
    {
      lsftp_await_command( id );

      /* FIXME: A mode_t can't be compared to -1. */
      if(attrib.flags & SSH_FILEXFER_ATTR_PERMISSIONS)
	/* We've got permissions? */
	return S_ISDIR(attrib.permissions);
    }
  
/*    printf( "%d (failed) \n",  -1 ); */

  return -1; /* Failed, report it */

}

int
lsftp_dc_glob_matches(const char *fname, const char *glob, int period)
{
  /*
   * Return 0 if filename doesn't match glob, 1 if it matches.
   *
   */

#ifdef HAVE_FNMATCH

  if( !glob || !fname )
    return 0;

  if( !period )
    return !fnmatch( glob, fname,  FNM_PATHNAME | FNM_PERIOD);
  else
    return !fnmatch( glob, fname,  FNM_PATHNAME );

#else
  /* We have our own fnmatch function if needed */

  int i = 0;
  int j = 0;
  int match = 1;
  int lastslash = -1;

  if( !glob || !fname )
    return 0;

/*    printf( "Trying to match fname %s with glob %s (period %d), return ", */
/*  	  fname, */
/*  	  glob, */
/*  	  period	   */
/*  	  ); */

  while( fname[i] && glob[j] )
    {
      switch( glob[j] )
	{
	case '*':
	  if( !period && 
	      '.' == fname[i] &&
	      i == ( lastslash + 1) 
	      ) /* Don't match first . */
	    match = 0;

	  if( !glob[j+1] ) /* End of pattern? Break */
	    {
	      printf( "%d\n", match );
	      return match;
	    }
	  else
	    {
	      /* More */
	    }
	  break;
	  
	case '?':
	  i++;
	  j++;
	  break;

	case '[':
	  {
	    char f = fname[i++];
	    int inverted = 0;
	    int openmatch = 0;
	    
	    char startrange;
	    char endrange;

	    j++;                      /* Compensate for the beginning [ */

	    if(                     /* Starts with ! or ^, when invert */
	       '!' == glob[j]  && 
	       '^' == glob[j]  
	       )
	      {
		inverted++;
		j++;
	      }
	    else if( ']' == glob[j] ) /* First char means */
	      {
		if( ']' == f)
		  openmatch++;
		j++;
	      }

	    while( glob[j] && 
		   ']' != glob[j] 
		   )
	      {
		char g = glob[j++];
		
		if( !inverted )
		  {
		  if( g == f ) /* Match? */ 
		    openmatch++;

		  if( '-' == glob[j] && glob[j+1] ) /* Range? */
		    {
		      startrange = g;
		      endrange = glob[j+1];
		      
		      if( f >= startrange &&  /* FIXME: How to check with local charsets ? */
			  f <= endrange 
			  )
			openmatch++;

		      j += 2; /* Move forward */
		    }
		  }
		else
		  { 
		    /* Inverted goes here */
		    
		    if( '-' == glob[j] && glob[j+1] ) /* Range? */
		      {
			startrange = g;
			endrange = glob[j+1];
			
			if( f < startrange ||  /* FIXME: How to check with local charsets ? */
			    f > endrange
			    )
			  openmatch++;

			j += 2; /* Move forward */
		      }
		    else 
		      if( g != f )
			openmatch++;
		  }
		
	      }

	    if( !glob[j++] ) /* End of glob without terminated [ - error (skip closing ] too) */
	      match = 0;

	    if( !openmatch ) /* Matching failed */
	      match = 0;
	  }
	  break;





	default: /* This is the case for normal charaters */

	  if( '/' ==fname[i] ) /* Remember position of rightmost slash */
	    lastslash = i;

	  if( glob[j] != fname[i] )
	    match = 0;
	  
	  i++;
	  j++;

	}
    }

  if( fname[i] || fname[j] ) /* One finished but not the other */
    match = 0;

  printf( "%d\n", match );
  return match;

#endif /* HAVE_FNMATCH */

}


static const char **
lsftp_dc_r_sloppy_glob(const char* glob, int nocheck UNUSED)
{
  const char **retval;
  const char **mem;
  int count = 0;
  int i;

  /* We'll have at most dc_entries matches */

  if( !lsftp_dircache )
    return 0;

  mem = malloc( sizeof( char* ) * ( dc_entries + 1 ) );

  if( !mem )  /* malloc failed? Bail out */
    {
      perror( "malloc failed" );
      return 0;
    }

  for( i = 0; i < dc_entries; i++ )
    {
      if( lsftp_dircache[i].name )
	if( 
	   lsftp_dc_glob_matches( 
				 lsftp_dircache[i].name, 
				 glob,
				 0
				 ) 
	   )
	  mem[count++] = strdup( lsftp_dircache[i].name );
    }

  mem[count++] = 0;

  retval = realloc( mem, count * sizeof( char* ) );

  if( !retval ) /*  realloc failed? mem is unchanged, return it */
    return mem;

  return retval;
}

static char *
lsftp_dc_path_no_glob(const char *glob)
{
  /* Returns the part of a string leading up to the glob */

  char* tmp = strdup( glob );
  int i;
  int j;
  int quoted = 0;
  int done = 0;

  if( !tmp ) /* Failed? */
    return 0;

  i = 0; 
  j = 0;

  while( !done && tmp[i] )
    switch( tmp[i++] )
      {
      case '*': /* A globchar? */
      case '?':
      case '[':
	if( !quoted )
	  done++;
	break;

      case '\\':
	if( tmp[i] )
	  i++;    /* Next charater (to handle) is disregarded, unless it's a terminator */
	break;

      case '/':   /* Path separator */
	j = i;    /* j is first char after separator */
	break;

      case '\'':   /* Quote char? */
	quoted = !quoted;    /* Toggle quoted flag */
	break;

      default:
	break;
      }
  
  tmp[j] = 0; /* Terminate string after last / */

  return tmp;
}


static char *
lsftp_dc_path_first_glob(const char* glob)
{
  /* Returns the first string up to and including the first glob */

  char* tmp = strdup( glob );

  int i;
  int j;
  int quoted = 0;
  int done = 0;
  int found = 0;

  if( !tmp ) /* Failed? */
    return 0;

  i = 0; 
  j = 0;

  while( !done && tmp[i] )
    switch( tmp[i++] )
      {
      case '*': /* A globchar? */
      case '?':
      case '[':
	if( !quoted )
	  found++;
	break;

      case '\\':
	if( tmp[i] )
	  i++;    /* Next charater (to handle) is disregarded, unless it's a terminator */
	break;

      case '/':   /* Path separator */
	j = i-1;    /* Points to separator */

	if( found )
	  done++;
	break;

      case '\'':   /* Quote char? */
	quoted = !quoted;    /* Toggle quoted flag */
	break;

      default:
	break;
      }
  
  if( done ) /* Found a slash after a globchar? */
    tmp[j] = 0; /* Terminate string after last / */
                /* If not, either we have no glob, or only at the rightmost level. Both are ok */ 
  return tmp;
}

const char **
lsftp_dc_r_startglob(const char *glob, int sloppy, int nocheck )
{
  const char **mem = NULL;
  const char **orgmem;
  const char* tmp;
  char* mglob;
  const char *restglob;
  const char **retval;
  const char **globbed;
  int count = 0;
  int i;
  int id;
  int deeper = 0;
  int size;

  /* FIXME: Do we allow for both qualified and unqualified paths to be globbed? */

  if( sloppy )
    return lsftp_dc_r_sloppy_glob( glob, nocheck );

  tmp = lsftp_dc_path_no_glob( glob ); /* Get the /a/ part of /a/b??/c/d/[a-f]/e? */
  mglob = lsftp_dc_path_first_glob( glob ); /* Get the part including the first glob, /a/b?? */
			  
  if( !tmp || !mglob ) /* failed? */
    return 0;

  restglob = lsftp_skip_common( glob, mglob );  /* Whatever is to the right of the first glob,
						 * ie /c/d/[a-f]/e? in /a/b??/c/d/[a-f]/e? */
  
  if( strlen( restglob ) ) /* There is more levels than this one? */
    deeper++;

  if( !lsftp_dc_r_isdir( tmp ) ) /* Is it a directory? */
    {
      /* Attempted globbing /a/b/[foo]* where /a/b is not a directory */

      mem = malloc( 2*sizeof( char* ) );

      if( !mem )
	{
	  perror( "malloc failed" );
	  return 0;
	}

      orgmem = mem;

      if( nocheck )
	*mem++ = strdup( glob );

      *mem++ = 0;

      return orgmem;	
    }

  id = lsftp_internal_ls( tmp, "Internal command", &mem );

  free( (void *) tmp );

  if( id > 0) /* Not a failure? */
    lsftp_await_command( id );      

  if( !mem ) /* ls failed? */
    return 0;

  orgmem = mem;     /* Save away */

  size = lsftp_dc_numglob( mem );

  retval = malloc( sizeof( char* ) * ( size + 1 ) ); /* Allocate memory for maximum number of slots needed (including terminating null) */

  i = 0;
  if( !retval ) /* malloc failed? */
    {
      while( mem[i] )  /* free mem used by ls before bailing out */
	free( (void *) mem[i++] );
      
      free( orgmem );

      return 0;
    }

  globbed = retval; /* Make a working copy pointer to our memory */

  while( mem[i] )
    if( 
       lsftp_dc_glob_matches( 
			     mem[i], 
			     mglob,
			     0
			     ) 
       )
      /* At least, we have a partial match */
      if( !deeper ) /* No more levels => full match */
	{
	  const char *tmp;

	  tmp = strdup( mem[i] );

	  if( tmp )
	    {
	      globbed[count++] = tmp; /* Match - use it */
	      free( (void *) mem[i++] );
	    }
	  else
	    globbed[count++] = mem[i++]; /* Match - use it */
	}
      else
	{
	  char* newglob = lsftp_concat( mem[i++], restglob );
	  const char **more = NULL;
	  const char **moretmp = NULL;
	  
	  if( newglob )
	    {
	      int reccount = 0;

 	      more = lsftp_dc_r_startglob( newglob, 0, 0 ); /* Recurse */ 

	      reccount = lsftp_dc_numglob( more );

	      if( reccount ) /* Any matches? */
		{
		  size += reccount;
		  moretmp = realloc( globbed, size * sizeof( char* ) );

		  if( moretmp )
		    {
		      globbed = moretmp;
		      retval = moretmp;
		      
		      moretmp = more;
		      
		      while( *moretmp )
			globbed[count++] = *moretmp++; /* Match - use it */		      
		    }
		  
		  /* If it failed globbed is left unharmed */

		  if( !reccount || !moretmp )
		    /* realloc returned zero or reccount was zero */
		    {
		      /* Throw away what we got */
		      moretmp = more;
		      
		      while( (tmp = *moretmp++) )
			free( (void *) tmp );
		    }

		  free( more );
		}
	    }
	}
    else
      free( (void *) mem[i++] );       /* Not a match - free it */

  free( orgmem ); /* We're done with the ls-thing */
      
  if( !count && nocheck) /* No match at all? */
    globbed[count++] = strdup( glob ); /* When leave the glob */
    
  globbed[count++] = 0;
  free( mglob );

  globbed = realloc( retval, count * sizeof( char* ) );

  if( !globbed ) /*  realloc failed? mem is unchanged, return it */
    return retval;

  return globbed;
}

const char **
lsftp_dc_l_startglob(const char *name, int nocheck )
{
  glob_t gl;
  size_t i; /* Use the same type as gl.pathc */
  const char** retval;

  gl.gl_pathc=0; /* Set a sensible value in case no parameters were passed */

  i = glob( name,
	    ( nocheck ? GLOB_NOCHECK : 0 ), /* return dir/, donotexist */
	    NULL, &gl
	    );

  if( i )         /* glob failed - return empty list */
    return 0;
    
  retval = malloc( (gl.gl_pathc + 1)  * sizeof( char* ) );

  if( !retval ) /* malloc failed? */
    return 0;

  for( i = 0; i < gl.gl_pathc; i++ )
    retval[i] = strdup( gl.gl_pathv[i] ); /* Copy all the strings */ 
                                          
  retval[i] = 0; /* Terminate with a null. 
		  * If malloc fails for strdup, 
		  * it returns a NULL, so it terminates
		  * prematurely 
		  */

  globfree( &gl );

  return retval;
}

const char **
lsftp_dc_l_contglob(const char *name, const char **globdata, int nocheck)
{
  const char** curglob;
  const char** retval;
  
  int i,k;

  if( ! globdata )         /* No globdata given? Well, start anew */
    return lsftp_dc_l_startglob( name, nocheck );

  curglob = lsftp_dc_l_startglob( name, nocheck );

  if( !curglob ) /* glob failed? */
    return globdata; /* Do as little harm as possible */

  i = lsftp_dc_numglob( curglob );
  k = lsftp_dc_numglob( globdata );

  if( !i ) /* Empty curglob? */
    return globdata;
  
  if( !k ) /* globdata is empty? */
    return curglob;

  retval = realloc( 
		   globdata, 
		   (i+k+1) * sizeof( char* ) 
		   ); /* We need more memory to house both */

  if( !retval ) /* Failed to allocate memory for new block? */
    return globdata;  /* Do as little harm as possible */

  /*
   * curglob contains i-1 paths and globdata contains k-1. Now, retval[k] 
   * is the first entry to be filled from curglob.
   *
   */

  i=0;

  /* Copy (also copies terminating NULL) */
  
  while( ( retval[k++] = curglob[i++] ) )
    ;
  
  free( curglob ); /* Free the memory curglob (holding pointers) */

  return retval;
}


const char **
lsftp_dc_r_contglob(const char *name, const char **globdata, int nocheck)
{
  const char **curglob;
  const char **retval;
  
  int i,k;

  if( ! globdata )         /* No globdata given? Well, start anew */
    return lsftp_dc_r_startglob( name, 0, nocheck );

  curglob = lsftp_dc_r_startglob( name, 0, nocheck );

  if( !curglob ) /* glob failed? */
    return globdata; /* Do as little harm as possible */

  i = lsftp_dc_numglob( curglob );
  k = lsftp_dc_numglob( globdata );

  if( !i ) /* Empty curglob? */
    return globdata;
  
  if( !k ) /* globdata is empty? */
    return curglob;

  retval = realloc( 
		   globdata, 
		   (i+k+1) * sizeof( char* ) 
		   ); /* We need more memory to house both */

  if( !retval ) /* Failed to allocate memory for new block? */
    return globdata;  /* Do as little harm as possible */

  /*
   * curglob contains i-1 paths and globdata contains k-1. Now, retval[k] 
   * is the first entry to be filled from curglob.
   *
   */

  i=0;

  /* Copy (also copies terminating NULL) */
  
  while( ( retval[k++] = curglob[i++] ) )
    ;
  
  free( curglob ); /* Free the memory curglob (holding pointers) */

  return retval;
}

int
lsftp_dc_numglob(const  char **globdata)
{
  int i = 0;

  if( ! globdata)
    return 0;

  while( globdata[i++] )
    ;

  return i-1;
}

void
lsftp_dc_endglob(const char **globdata)
{
  const char *curmem;
  const char **curptr = globdata;

  if( ! globdata )
    return;

  while( (curmem = *curptr++) )
    free( (void *) curmem );

  free( globdata );
}
