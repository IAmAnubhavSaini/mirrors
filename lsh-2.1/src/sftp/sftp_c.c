/*
 * @(#) $Id: d7736f2d4ec0a583b2c81dd877bcdcdc8cce555a $
 *
 * sftp_c.c - sftp client protocol functions.
 *
 * Portions of code taken from the sftp test client from 
 * the sftp server of lsh by Niels Möller and myself.
 *
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

#include "sftp_c.h"

void
sftp_attrib_from_stat(const struct stat *st, struct sftp_attrib* a)
{
  a->permissions = st->st_mode;
  a->uid = st->st_uid;
  a->gid = st->st_gid;
  a->atime = st->st_atime;
  a->mtime = st->st_mtime;
  a->size = st->st_size;
  a->flags = ( 
	      SSH_FILEXFER_ATTR_SIZE ||
	      SSH_FILEXFER_ATTR_UIDGID ||
	      SSH_FILEXFER_ATTR_PERMISSIONS || 
	      SSH_FILEXFER_ATTR_ACMODTIME
	      );
}

int
sftp_attrib_perms(const struct sftp_attrib *a,
		  mode_t *mode)
{
  if (a->flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      *mode = a->permissions;
      return 1;
    }
  
  return 0;
}

void
sftp_null_state(struct sftp_callback *s)
{
  s->nextfun = 0;
  s->id = 0;
  s->fd = 0;
  s->last = SFTP_HANDLE_LAST_UNKNOWN;
  s->filepos = 0;
  s->filesize = 0;
  s->op_id = 0;
  s->retval = 0;
  s->handle = 0;
  s->handlelen = 0;
  s->localerr = 0;
  s->localerrno = 0;

  sftp_null_mem(&s->mem);

  s->bad_status = 0;
  s->last_bad = SFTP_HANDLE_LAST_UNKNOWN;

  sftp_clear_attrib( &s->attrib );
}

void
sftp_alloc_mem(struct sftp_mem *s, int desired_size)
{
  /* Reserve a memory block*/
  s->at = malloc( desired_size );

  assert( s->at != 0 ); /* Make sure we got some memory */

  s->used = 0;
  s->size = desired_size;
}

int
sftp_resize_mem(struct sftp_mem *mem, int newsize)
{
  if( !newsize ) /* Want to create an empty block? */
    {
      free( mem-> at ); /* Free used memory */

      mem->size = 0; /* Set everything to zero */
      mem->used = 0;
      mem->at = 0;
    }
  else
    {
      char *newat=realloc( mem->at, newsize );
  
      if ( newat ) /* realloc successful? */
	{
	  mem->at = newat;
	  mem->size = newsize;
	  return 0;
	}
    }

  return -1; /* realloc failed, leave mem unchanged */
}



int sftp_free_mem( struct sftp_mem *mem )
{
  /* Free a reserved memory */

  free( mem->at );

  mem->at = 0;
  mem->size = 0;
  mem->used = 0;

  return 0;
}

void
sftp_null_mem(struct sftp_mem *s)
{
  s->at = 0;
  s->size = 0;
  s->used = 0;
}

int sftp_toggle_mem( struct sftp_mem* mem )
{
  int newsize = mem->used;
  mem->used = 0;

  return sftp_resize_mem( mem, newsize );
}

int
sftp_store( struct sftp_mem* mem, void* data, uint32_t datalen )
{
  if( ( mem->size - mem->used ) < datalen ) /* Won't fit? */
    if ( 
	( -1 ==
	  sftp_resize_mem( mem, mem->used + datalen )   /* Make it larger */
	  )
	)
      return -1; /* Resize failed? */

  memcpy( mem->at + mem->used, data, datalen );
  mem->used += datalen;

  return 0;
}

void* sftp_retrieve( struct sftp_mem* mem, uint32_t desired, uint32_t* realsize )
{
  uint8_t* s;

  if( ( mem->size - mem->used ) < desired ) /* Requests more than available? */
    *realsize =  mem->size - mem->used;
  else
    *realsize = desired;

  s = xmalloc( *realsize+1 );
  memcpy( s, mem->at + mem->used, *realsize );
  mem->used += *realsize;

  s[*realsize] = 0; /* NUL-terminate for convenience */
  return s;
}



uint32_t sftp_rumask( uint32_t new )
{
  static uint32_t old = 0;
  uint32_t ret = old;

  old = new;
  return ret;
}


uint32_t sftp_unique_id()
{
  static uint32_t id = 0;

  id++;
  return id;
}


int sftp_handshake( 
		   struct sftp_input *in,
		   struct sftp_output *out
		   )
{
  uint8_t msg = -1 ;
  uint32_t use_version = -1;
  int ok;

  sftp_set_msg( out, SSH_FXP_INIT );
  sftp_set_id( out, SFTP_VERSION );

  ok = sftp_write_packet( out );

  if( -1 == ok ) /* Failed */
    return -1;

  ok = 0;

  while( !ok )
    ok = sftp_read_packet( in );
  
  if( -1 == ok )
    return -1;

  ok = 0;

  while( !ok )
    ok = sftp_get_uint8( in, &msg );
  
  if( -1 == ok )
    return -1;

  ok = 0;

  while( !ok )
    ok = sftp_get_uint32( in, &use_version );
  
  if( -1 == ok )
    return -1;

  if( msg == SSH_FXP_VERSION )
/*     { */
/*       printf( "Server responded with message %d and version %d \n", msg, use_version ); */
      return 0;
/*     } */
  
  perror( "failed" );
  printf( "Server responded with message %d and version %d \n", msg, use_version );

  /* FIXME; we silently ignore the version given by the server */

  return -1; /* Report failure to handshake correctly */

}




void
sftp_rename_init(struct sftp_callback *state,
		 int op_id,
		 struct sftp_input *in UNUSED, 
		 struct sftp_output *out,
		 const uint8_t *srcname,
		 uint32_t srclen,
		 const uint8_t *dstname,
		 uint32_t dstlen)
{
  uint32_t id;

  sftp_null_state(state);

  id=sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_RENAME );
  sftp_set_id( out, id );

  sftp_put_string( out, srclen, srcname );
  sftp_put_string( out, dstlen, dstname );

  state->last = SFTP_RENAME_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}


void
sftp_symlink_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in UNUSED, 
		  struct sftp_output *out, 
		  const uint8_t *linkname,
		  uint32_t linklen,
		  const uint8_t *targetname,
		  uint32_t targetlen)
{
  uint32_t id;
  sftp_null_state(state);
  
  id = sftp_unique_id();
  
  sftp_set_msg( out, SSH_FXP_SYMLINK );
  sftp_set_id( out, id );
  
  sftp_put_string( out, linklen, linkname );
  sftp_put_string( out, targetlen, targetname );

  state->last = SFTP_SYMLINK_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}

void
sftp_remove_init(struct sftp_callback *state,
		 int op_id,
		 struct sftp_input *in UNUSED, 
		 struct sftp_output *out,
		 const uint8_t *name,
		 uint32_t namelen)
{
  uint32_t id;
  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_REMOVE );
  sftp_set_id( out, id );

  sftp_put_string( out, namelen, name );

  state->last = SFTP_REMOVE_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}


void
sftp_mkdir_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in UNUSED, 
		struct sftp_output *out,
		const uint8_t *name,
		uint32_t namelen,
		struct sftp_attrib *a)
{
  uint32_t id;
  uint32_t mask;
  struct sftp_attrib locala = *a;

  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_MKDIR );
  sftp_set_id( out, id );

  mask = sftp_rumask( 0 );   /* Perform remote umasking */
  sftp_rumask( mask );

  locala.permissions = locala.permissions & ~mask;

  sftp_put_string( out, namelen, name );
  sftp_put_attrib( out, &locala );
  state->last = SFTP_MKDIR_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}

void
sftp_realpath_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in UNUSED, 
		   struct sftp_output *out,
		   const uint8_t *name,
		   uint32_t namelen)
{
  uint32_t id;

  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_REALPATH );
  sftp_set_id( out, id );

  sftp_put_string( out, namelen, name );

  state->last = SFTP_REALPATH_INIT;
  state->id = id;
  state->nextfun = sftp_handle_name;
  state->op_id = op_id;
}

void
sftp_readlink_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in UNUSED, 
		   struct sftp_output *out,
		   const uint8_t *name,
		   uint32_t namelen)
{
  uint32_t id;
  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_READLINK );
  sftp_set_id( out, id );

  sftp_put_string( out, namelen, name );

  state->last = SFTP_READLINK_INIT;
  state->id = id;
  state->nextfun = sftp_handle_name;
  state->op_id = op_id;
}


void
sftp_rmdir_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in UNUSED, 
		struct sftp_output *out,
		const uint8_t *name,
		uint32_t namelen)
{
  uint32_t id;
  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_RMDIR );
  sftp_set_id( out, id );

  sftp_put_string( out, namelen, name );

  state->last = SFTP_RMDIR_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}


void
sftp_stat_init(struct sftp_callback *state,
	       int op_id,
	       struct sftp_input *in UNUSED,
	       struct sftp_output *out,
	       const uint8_t *name,
	       uint32_t namelen)
{
  uint32_t id;
  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_STAT );
  sftp_set_id( out, id );

/*    printf( "Doing stat for %s\n", name ); */

  sftp_put_string( out, namelen, name );

  state->last = SFTP_STAT_INIT;
  state->id = id;
  state->nextfun = sftp_handle_attrs;
  state->op_id = op_id;
}


void
sftp_lstat_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in UNUSED,
		struct sftp_output *out,
		const uint8_t *name,
		uint32_t namelen)
{
  uint32_t id;
  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_LSTAT );
  sftp_set_id( out, id );

  sftp_put_string( out, namelen, name );

  state->last = SFTP_LSTAT_INIT;
  state->id = id;
  state->nextfun = sftp_handle_attrs;
  state->op_id = op_id;
}


void
sftp_fstat_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in UNUSED,
		struct sftp_output *out,
		const uint8_t *handle,
		uint32_t handlelen)
{
  uint32_t id;
  sftp_null_state(state);

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_FSTAT );
  sftp_set_id( out, id );

  sftp_put_string( out, handlelen, handle );

  state->last = SFTP_FSTAT_INIT;
  state->id = id;
  state->nextfun = sftp_handle_attrs;
  state->op_id = op_id;
}

void
sftp_setstat_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in UNUSED,
		  struct sftp_output *out,
		  const uint8_t *name,
		  uint32_t namelen,
		  struct sftp_attrib* attrib)
{
  uint32_t id;
  sftp_null_state(state);

  id=sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_SETSTAT );
  sftp_set_id( out, id );

  sftp_put_string( out, namelen, name );
  sftp_put_attrib( out, attrib );

  state->last = SFTP_SETSTAT_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}

void
sftp_fsetstat_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input* in UNUSED,
		   struct sftp_output* out,
		   const uint8_t *handle,
		   uint32_t handlelen,
		   struct sftp_attrib* attrib)
{
  uint32_t id;
  sftp_null_state(state);

  id=sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_FSETSTAT );
  sftp_set_id( out, id );

  sftp_put_string( out, handlelen, handle );
  sftp_put_attrib( out, attrib );

  state->last = SFTP_FSETSTAT_INIT;
  state->id = id;
  state->nextfun = sftp_handle_status;
  state->op_id = op_id;
}


/* Get a file to memory */

void
sftp_get_mem_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in UNUSED,
		  struct sftp_output *out,
		  const uint8_t *name, 
		  uint32_t namelen,
		  struct sftp_mem *mem,
		  off_t startat)
{
  uint32_t id;
  struct sftp_attrib a;

  sftp_null_state( state );   /* Make sure these structures are "clean" */
  sftp_clear_attrib( &a );

  id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_OPEN );
  sftp_set_id( out, id );
  sftp_put_string( out, namelen, name );
  sftp_put_uint32( out, SSH_FXF_READ ); /* Only read, no other flag apply */
  sftp_put_attrib( out, &a );            /* Send an empty attribute */

  state->filepos = startat; /* Start reading from the given position */
  state->id = id;
  state->mem = *mem;
  state->nextfun = sftp_get_mem_step_one;
  state->op_id = op_id;
  state->last = SFTP_GET_MEM_INIT;
}

void
sftp_get_mem_step_one(struct sftp_callback *next,
		      uint8_t msg,
		      uint32_t id,
		      struct sftp_input *in,
		      struct sftp_output *out,
		      const struct sftp_callback *state)
{
  *next = *state;
  
  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }

/* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_HANDLE && id == state->id ); 
  
  next->handle = sftp_get_string( 
				    in, 
				    &next->handlelen 
				    ); /* Get handle */



  /* Now we send a read command */

  id=sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_READ );
  sftp_set_id( out, id );

  sftp_put_string( out, next->handlelen, next->handle );
  sftp_put_uint64( out, next->filepos ); /* Offset */
  sftp_put_uint32( out, SFTP_BLOCKSIZE ); /* Length */

  next->id = id;
  next->nextfun = sftp_get_mem_main;
}

void
sftp_get_mem_main(struct sftp_callback *next,
		  uint8_t msg,
		  uint32_t id,
		  struct sftp_input *in,
		  struct sftp_output *out,
		  const struct sftp_callback *state)
{
 
  uint32_t datalen;
  int done = 0;

  uint8_t* tmp;  

  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
  /* Otherwise we shouldn't be here FIXME: Fail gracefully?*/
  assert( msg == SSH_FXP_DATA && id == state->id );
  
  next->last = SFTP_GET_MEM_MAIN; /* Tell theme where we are */
  
  tmp = sftp_get_string( in, &datalen ); /* Get data */

  /* Append to buffer */
  assert( sftp_store( &next->mem, tmp, datalen ) == 0 );
  
  /* Move forward in file */

  next->filepos += datalen;

  if( datalen != SFTP_BLOCKSIZE )                /* Got what we asked for? */
    done = 1;               /* Nah, assume short read => EOF => we're done */

  sftp_free_string( tmp );                       /* Free temporary buffer */

  /* Now we send a read command */
  
  id = sftp_unique_id();
  next->id = id;

  if( !done ) /* Not yet finished? */
    {
      sftp_set_msg( out, SSH_FXP_READ );
      sftp_set_id( out, id );
      
      sftp_put_string( out, next->handlelen, next->handle );
      sftp_put_uint64( out, next->filepos ); /* Offset */
      sftp_put_uint32( out, SFTP_BLOCKSIZE ); /* Length */
      
      next->nextfun = sftp_get_mem_main;
    }
  else
    {
      /* We're done - close the file */

      sftp_set_msg( out, SSH_FXP_CLOSE );
      sftp_set_id( out, id );
      
      sftp_put_string( out, next->handlelen, next->handle );
      
      next->nextfun = sftp_handle_status;

      sftp_free_string( next->handle ); /* Release memory used by handle */

      next->handle = 0;                  /* Replace with a null string */
      next->handlelen = 0;  
    }
}


/* Put a file from memory */

void
sftp_put_mem_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in UNUSED,
		  struct sftp_output *out,
		  const uint8_t *name, 
		  uint32_t namelen,
		  struct sftp_mem *mem,
		  off_t startat,
		  struct sftp_attrib a)
{
  uint32_t id;
  uint32_t flags;
  uint32_t mask;

  sftp_null_state(state);

  id = sftp_unique_id();

  if( startat ) /* Offset given? */
    flags = SSH_FXF_CREAT | SSH_FXF_WRITE;
  else
    flags = SSH_FXF_CREAT | SSH_FXF_WRITE | SSH_FXF_TRUNC;

  mask = sftp_rumask( 0 );  /* Perform remote umasking */
  sftp_rumask( mask );

  a.permissions = a.permissions & ~mask;

  sftp_set_msg( out, SSH_FXP_OPEN );
  sftp_set_id( out, id );
  sftp_put_string( out, namelen, name );
  sftp_put_uint32( out, flags ); /* How to open */
  sftp_put_attrib( out, &a );

  state->filepos = startat;
  state->id = id;
  state->mem = *mem;
  state->nextfun = sftp_put_mem_step_one;
  state->op_id = op_id;
  state->last = SFTP_PUT_MEM_INIT;
}

void
sftp_put_mem_step_one(struct sftp_callback *next,
		      uint8_t msg,
		      uint32_t id,
		      struct sftp_input *in,
		      struct sftp_output *out,
		      const struct sftp_callback *state)
{
  uint32_t datalen;
  uint8_t* tmp;
  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
/* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_HANDLE && id == state->id ); 

  next->handle = sftp_get_string( in, &next->handlelen ); /* Get handle */

  /* Now we send a read command */

  next->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_WRITE );
  sftp_set_id( out, next->id );

  sftp_put_string( out, next->handlelen, next->handle );
  sftp_put_uint64( out, next->filepos ); /* Offset */

  tmp = sftp_retrieve( &next->mem, SFTP_BLOCKSIZE, &datalen ); /* Get (if possible) SFTP_BLOCKSIZE bytes */
  sftp_put_string( out, datalen, tmp ); /* Write the data */

  sftp_free_string( tmp ); /* Free temporary string */

  next->nextfun = sftp_put_mem_main;
  next->last = SFTP_PUT_MEM_STEP_ONE;
}

void
sftp_put_mem_main(struct sftp_callback *next,
		  uint8_t msg,
		  uint32_t id,
		  struct sftp_input *in,
		  struct sftp_output *out,
		  const struct sftp_callback *state)
{
  int done = 0;

  uint32_t datalen;
  uint8_t* tmp;  
  *next = *state;
 
  assert( msg == SSH_FXP_STATUS && id == state->id); 
  assert( sftp_get_uint32( in, &next->retval ) > 0 );

  next->id = sftp_unique_id();
  
  if( next->retval != SSH_FX_OK ) /* Write failed? */
    {
      next->nextfun = sftp_handle_status;
      next->last = SFTP_PUT_MEM_MAIN;

      next->bad_status = next->retval; /* Store status */
      next->last_bad = SFTP_PUT_MEM_MAIN;
  
      if( next->handle ) /* Any old handles around (we should have) */
	{       
	  sftp_set_msg( out, SSH_FXP_CLOSE ); /* Try to close */
	  sftp_set_id( out, next->id );
	  
	  sftp_put_string( out, next->handlelen, next->handle );
	  
	  sftp_free_string( next->handle );
	  next->handle = 0;
	  next->handlelen = 0;

	}
     
      return;
    }


  tmp = sftp_retrieve( &next->mem, SFTP_BLOCKSIZE, &datalen ); /* Get data to write */

/*   printf("sftp_put_mem_main: Will write %d bytes at pos %d\n", datalen, next->filepos); */

  if( ! datalen  )                  /* Got nothing at all? */ 
    done = 1;                       /* Assume short read => EOF => we're done */ 

  /* Now we send a write command */
  
/*   printf("sftp_put_mem_main: Datalen is  %d, done is %d\n", datalen, done); */

  if( !done ) /* Not yet finished? */
    {
      sftp_set_msg( out, SSH_FXP_WRITE );
      sftp_set_id( out, next->id );
      
/*       printf("Writing %d bytes at %d \n", datalen, next->filepos ); */

      sftp_put_string( out, next->handlelen, next->handle );
      sftp_put_uint64( out, next->filepos ); /* Offset */
      sftp_put_string( out, datalen, tmp ); /* What to write */

      next->filepos += datalen;

      sftp_free_string( tmp ); /* Free temporary buffer */

      next->nextfun = sftp_put_mem_main;
    }
  else
    {
      /* We're done, just close the file and wrap it up */

      sftp_set_msg( out, SSH_FXP_CLOSE );
      sftp_set_id( out, next->id );
      
/*       printf("Closing handle %s of len %d\n", next->handle, next->handlelen ); */

      sftp_put_string( out, next->handlelen, next->handle );
      
      next->nextfun = sftp_handle_status;

      sftp_free_string( tmp )               /* Free temporary buffer */;
      sftp_free_string( next->handle ); /* Release memory used by handle */

      next->handle = 0;            /* Replace with a null string */
      next->handlelen = 0;
    }

  next->last = SFTP_PUT_MEM_STEP_ONE;
}


/* End of put from memory */

/* Get to file - wrapper for the functions to memory */

void
sftp_get_file_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const uint8_t *name, 
		   uint32_t namelen,
		   const uint8_t *fname,
		   /* FIXME: fnamelen not used??? */
		   uint32_t fnamelen,
		   int cont)
{
  int openmode;

  off_t startat = 0;
  struct sftp_mem mem;
  int ret, fd;
  int mask;

  sftp_null_state( state ); /* Make sure state is "clean" */

  /* FIXME: Should probably try to retain permissions from the other side */

  openmode = O_CREAT | O_RDWR;
  state->last = SFTP_GET_FILE_INIT;

  if( cont ) /* If continue mode, stat and continue at end of file */
    {
      int statret;
      struct stat st;
      
      statret = stat( fname, &st );
      
      if( !statret ) /* Returned 0 - file exists */
	startat = st.st_size;
    }
  else
    openmode |= O_TRUNC; /* Not continue? Restart from beginning  */

  mask = umask( 0 );
  umask( mask );

  ret = open( fname, openmode, 0777 & ~mask );
  
  if( ret == -1 ) /* Failed? */
    {
      state->localerr = ret;
      state->localerrno = errno;
      return;
    }
  else
    fd = ret; /* Success */

#ifdef USING_CYGWIN
  setmode( fd, O_BINARY );
#endif

  if( startat ) /* Only seek if we're continuing */
    {
      ret = lseek( fd, startat, SEEK_SET ); 

      if( ret == -1 ) /* Failed? */
	{
	  state->localerr = ret;
	  state->localerrno = errno;
	  
	  return;
	} 
    }

  sftp_alloc_mem(&mem, 2 * SFTP_BLOCKSIZE );
  sftp_get_mem_init(state, op_id, in, out, name, namelen, &mem, startat );

  state->nextfun = sftp_get_file_step_one;
  state->fd = fd;
  state->last = SFTP_GET_FILE_INIT;
}

void
sftp_get_file_step_one(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state)
{
  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
  assert( msg == SSH_FXP_HANDLE && id == state->id ); 
  
  next->handle = sftp_get_string( 
				    in, 
				    &next->handlelen 
				    ); /* Get handle */



  /* Now we send a stat command */

  next->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_FSTAT );
  sftp_set_id( out, next->id );

  sftp_put_string( out, next->handlelen, next->handle );

  next->nextfun = sftp_get_file_step_two;
  next->last = SFTP_GET_FILE_STEP_ONE;
}

void
sftp_get_file_step_two(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state)
{
  int remoteperms = 0700;
  int mask;
  int ret;

  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
  sftp_handle_attrs(next, msg, id, in, out, state );

  if( next->attrib.flags &  SSH_FILEXFER_ATTR_PERMISSIONS )
    remoteperms = next->attrib.permissions;

  if( next->attrib.flags &  SSH_FILEXFER_ATTR_SIZE )
    next->filesize = next->attrib.size;

  
  mask = umask( 0 );
  umask( mask );

  ret = fchmod( next->fd, remoteperms & ~mask ); /* Note: may remove our write permissions */

  if( -1 == ret )
    {
      next->localerr = ret;
      next->localerrno = errno;
	  
      return;
    }

  next->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_READ );
  sftp_set_id( out, next->id );

  sftp_put_string( out, next->handlelen, next->handle );
  sftp_put_uint64( out, next->filepos ); /* Offset */
  sftp_put_uint32( out, SFTP_BLOCKSIZE ); /* Length */

  next->nextfun = sftp_get_file_main;
  next->last = SFTP_GET_FILE_STEP_TWO;
}

void
sftp_get_file_main(struct sftp_callback *next,
		   uint8_t msg,
		   uint32_t id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const struct sftp_callback *state)
{
  int ret;
  int write_needed = 0;
  uint32_t i;

  /* FIXME: This is ugly. The me object needs to be moved a pointer
   * away, or redesigned so that we don't need to modify it here. (Or
   * the constification of sftp_callback could be undone
   * altogether). */
  ((struct sftp_callback *)state)->mem.used = 0;

  sftp_get_mem_main(next, msg, id, in, out, state );

  /* Should we do a lseek here? Just to be sure? */

  next->last = SFTP_GET_FILE_MAIN;

  ret = lseek( 
	      next->fd,
	      next->filepos - next->mem.used,  /* filepos is the current position - 
						      * at the end of the block we'll write
						      */
	      SEEK_SET
	      );

  if( ret == -1 ) /* Failed? */
    {
      next->nextfun = 0; /* No next callback */
      next->id = 0;
      next->localerr = ret;
      next->localerrno = errno;
      return;
    }


  for( i = 0; i < next->mem.used; i++ )   /* Go through the block */
    if( next->mem.at[i] )                 
      write_needed = 1;        /* If any byte is non-zero, we write it all */

  if( write_needed )
    {
      ret = write(                                /* Write what we got */
		  next->fd, 
		  next->mem.at, 
		  next->mem.used 
		  );
      
      if( ret == -1 ) /* Failed? */
	{
	  next->nextfun = 0; /* No next callback */
	  next->id = 0;
	  next->localerr = ret;
	  next->localerrno = errno;
	  return;
	}
    }
  
  /* It seems to be done, so we close our file and free the memory block */

  if( next->nextfun == sftp_handle_status ) 
    {
      /* Just to be sure the file has grown, we (possibly re-) write the last byte of the file 
       * before closing it.
       */

      ret = lseek(                       /* Seek to EOF - 1 (or what should be it anyway) */
		  next->fd,
		  next->filepos - 1, 
		  SEEK_SET
	      );

      if( ret == -1 ) /* Failed? */
	{
	  next->nextfun = 0; /* No next callback */
	  next->id = 0;
	  next->localerr = ret;
	  next->localerrno = errno;
	  return;
	}
      

      ret = write(                                /* Write what we got */
		  next->fd, 
		  next->mem.at + next->mem.used - 1, 
		  1 
		  );
      
      if( ret == -1 ) /* Failed? */
	{
	  next->nextfun = 0; /* No next callback */
	  next->id = 0;
	  next->localerr = ret;
	  next->localerrno = errno;
	  return;
	}



      ret = close( next->fd );       /* Close file */

      if( ret == -1 ) /* Failed? */
	{
	  next->nextfun = 0; /* No next callback */
	  next->id = 0;
	  next->localerr = ret;
	  next->localerrno = errno;
	  return;
	}
	
      sftp_free_mem( &next->mem );
      sftp_null_mem(&next->mem);
    }
  /* Check so we didn't bail out */
  else if( next->nextfun )
      /* Not yet done, see to that we interfere the next time */
      next->nextfun = sftp_get_file_main; 
}


/* End of get to file */



/* Put from file - wrapper for the functions from memory */

void
sftp_put_file_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const uint8_t *name,
		   uint32_t namelen,
		   const uint8_t *fname,
		   /* FIXME: fnamelen not used??? */
		   uint32_t fnamelen,
		   int cont)
{
/*
 * Passing a startat of zero makes put_mem_init truncate the
 * destination file, which is not desired in continue-mode.
 *
 * The filepos is fixed after the stat
 */

  off_t startat = cont; 
  off_t filesize = 0;

  struct sftp_mem mem;
  struct stat st;
  struct sftp_attrib a;
  int ret;
  int fd;

  sftp_null_state( state ); /* Make sure state is "clean" */

  state->last = SFTP_PUT_FILE_INIT;

  ret = open( fname, O_RDONLY );
  
  if( ret == -1 ) /* Failed? */
    {
      state->localerr = ret;
      state->localerrno = errno;
      state->nextfun = 0;
      state->id = 0;

      return;
    }
  else
    fd = ret; /* Success */

#ifdef USING_CYGWIN
  setmode( fd, O_BINARY );
#endif

  ret = fstat( fd, &st ); /* */

  if( ret == -1 )     /* We had an error while stating, send an empty attribute object instead */
                      /* (we'll still try to send the file) */
    sftp_clear_attrib( &a );
  else /* No error - get attrib from stat */
    {
      sftp_attrib_from_stat( &st, &a );
      filesize = st.st_size;  /* Fill in size */
    }

  /* FIXME: We should probably have calculated startat here */

  sftp_alloc_mem(&mem, SFTP_BLOCKSIZE );
  sftp_put_mem_init(state, op_id, in, out, name, namelen, &mem, startat, a );

  state->fd = fd;
  state->filesize = filesize;

  if( cont )
    state->nextfun = sftp_put_file_do_fstat;
  else
    state->nextfun = sftp_put_file_step_one;
}

void
sftp_put_file_do_fstat(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state)
{
  uint8_t* handle;
  uint32_t handlelen;
    
  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Status? (open failed) */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
  /* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_HANDLE && id == state->id ); 

  handle = sftp_get_string( in, &handlelen ); /* Get handle */
  sftp_fstat_init( next, state->op_id, in, out, handle, handlelen );

  next->mem = state->mem; /* FIXME; should probably not be here */
  next->fd = state->fd;

  next->last = SFTP_PUT_FILE_DO_FSTAT;
  next->handle = handle;
  next->handlelen = handlelen;
  next->nextfun = sftp_put_file_handle_stat;
}

void
sftp_put_file_handle_stat(struct sftp_callback *next,
			  uint8_t msg,
			  uint32_t id,
			  struct sftp_input *in,
			  struct sftp_output *out,
			  const struct sftp_callback *state)
{
  uint8_t tmp[ SFTP_BLOCKSIZE ];
  int ret;

  sftp_handle_attrs(next, msg, id, in, out, state );

  next->filepos = 0;
  
  if( next->attrib.flags & SSH_FILEXFER_ATTR_SIZE )
    next->filepos = next->attrib.size;
 
  next->last = SFTP_PUT_FILE_HANDLE_STAT;

  ret = lseek( next->fd, next->filepos, SEEK_SET );

  if( ret == -1 ) /* Failed? */
    {
      next->localerr = ret;
      next->localerrno = errno;
      next->id = 0;
      next->nextfun = 0;

      return;
    }

  ret = read( next->fd, tmp, SFTP_BLOCKSIZE ); /* Hrrm? */

  if( ret == -1 ) /* Failed? */
    {
      next->localerr = ret;
      next->localerrno = errno;
      next->id = 0;
      next->nextfun = 0;

      return;
    }

  next->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_WRITE );
  sftp_set_id( out, next->id );

  sftp_put_string( out, next->handlelen, next->handle );
  sftp_put_uint64( out, next->filepos ); /* Offset */
  sftp_put_string( out, ret, tmp );

  next->nextfun = sftp_put_file_main;
  next->filepos += ret;

  return;
}


void
sftp_put_file_step_one(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state)
{
  int ret;
  uint8_t tmp[ SFTP_BLOCKSIZE ];

  *next = *state;

  next->last = SFTP_PUT_FILE_STEP_ONE;

  /* Not much to do here, we just alter the next callback */
  ret = lseek( next->fd, next->filepos, SEEK_SET );

  if( ret == -1 ) /* Failed? */
    {
      next->localerr = ret;
      next->localerrno = errno;
      next->id = 0;
      next->nextfun = 0;

      return;
    }

  ret = read( next->fd, tmp, SFTP_BLOCKSIZE );

  if( ret == -1 ) /* Failed? */
    {
      next->localerr = ret;
      next->localerrno = errno;
      next->id = 0;
      next->nextfun = 0;

      return;
    }
  {
    sftp_store( &next->mem, tmp, ret );
    sftp_toggle_mem( &next->mem );
  }

  /* FIXME: Perhaps it should be (next, ..., state)? */
  sftp_put_mem_step_one(next, msg, id, in, out, next);

  next->last = SFTP_PUT_FILE_STEP_ONE; 

  next->nextfun = sftp_put_file_main; /* FIXME: Check for bailing out */
}

void
sftp_put_file_main(struct sftp_callback *next,
		   uint8_t msg,
		   uint32_t id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const struct sftp_callback *state)
{
  uint8_t tmp[ SFTP_BLOCKSIZE ];
  int ret;
  *next = *state;

  next->last = SFTP_PUT_FILE_MAIN;

  ret = lseek( next->fd, next->filepos, SEEK_SET );

  if( ret == -1 ) /* Failed? */
    {
      next->localerr = ret;
      next->localerrno = errno;
      next->id = 0;
      next->nextfun = 0;

      return;
    }

  ret = read( next->fd, tmp, SFTP_BLOCKSIZE );

  if( ret == -1 ) /* Failed? */
    {
      next->localerr = ret;
      next->localerrno = errno;
      next->id = 0;
      next->nextfun = 0;

      return;
    }
  else
  {
/*     printf("Storing %d bytes from filepos %d\n", ret, next->filepos ); */

    sftp_toggle_mem( &next->mem ); /* Clear buffer by toggling two times */
    sftp_toggle_mem( &next->mem );

/*     printf("sftp_put_mem_main: mem has size %d and used %d \n", next->mem.size, next->mem.used ); */
    sftp_store( &next->mem, tmp, ret );
/*     printf("sftp_put_mem_main: mem has size %d and used %d \n", next->mem.size, next->mem.used ); */
    sftp_toggle_mem( &next->mem );
/*     printf("sftp_put_mem_main: mem has size %d and used %d \n", next->mem.size, next->mem.used ); */
  }


/*   printf("Read %d bytes to be written at %d \n", ret, next->filepos); */

  /* FIXME: Perhaps it should be (next, ..., state)? */
  sftp_put_mem_main(next, msg, id, in, out, next );

  next->last = SFTP_PUT_FILE_MAIN;

  if( next->nextfun == sftp_handle_status ) 
    /* It seems to be done, so we close our file and free the memory block */
    {
      close( next->fd );
      sftp_free_mem( &next->mem );
      sftp_null_mem(&next->mem);
    }
  else 
    if( next->nextfun ) /* Check so we didn't bail out */
      /* Not yet done, see to that we interfere the next time */
      next->nextfun = sftp_put_file_main; 
}


/* End of put from file */


/* Do a ls, save the results in the result-string */

void
sftp_ls_init(struct sftp_callback *state,
	     int op_id,
	     struct sftp_input *in UNUSED,
	     struct sftp_output *out,
	     const uint8_t *dir,
	     uint32_t dirlen)
{
  sftp_null_state(state);

  state->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_OPENDIR );
  sftp_set_id( out, state->id );

  sftp_put_string( out, dirlen, dir );

  state->nextfun = sftp_ls_step_one;
  state->op_id = op_id;
}

void
sftp_ls_step_one(struct sftp_callback *next,
		 uint8_t msg,
		 uint32_t id,
		 struct sftp_input *in,
		 struct sftp_output *out,
		 const struct sftp_callback *state)
{
  *next = *state;
  
  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }  
/* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_HANDLE && id == state->id );

  next->handle = sftp_get_string( in, &next->handlelen ); /* Get handle */

  /* Now we send a readdir command */

  next->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_READDIR );
  sftp_set_id( out, next->id );

  sftp_put_string( out, next->handlelen, next->handle );

  next->nextfun = sftp_ls_main;
}

void
sftp_ls_main(struct sftp_callback *next,
	     uint8_t msg,
	     uint32_t id,
	     struct sftp_input *in,
	     struct sftp_output *out,
	     const struct sftp_callback *state)
{
  uint32_t count;
 
  uint32_t i;
   uint8_t *fname;
   uint8_t *longname;
   
   uint32_t fnamelen;
   uint32_t longnamelen;
  
  struct sftp_attrib a;

  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
/* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_NAME && id == state->id ); 

  assert( sftp_get_uint32( in, &count ) > 0 ); /* Get count */

  for( i=0; i < count; i++ ) /* Do count times */
    {
      uint32_t attriblen = sizeof( struct sftp_attrib );

      /* Read filename, longname and attrib */

      fname = sftp_get_string( in, &fnamelen ); 
      longname = sftp_get_string( in, &longnamelen );

      sftp_clear_attrib( &a );

      assert( sftp_get_attrib( in, &a ) > 0 );

      /* Write string length before. Explicit casts */

      if( fname ) /* sftp_get_string succedeed */
	{
	  sftp_store( &next->mem, &fnamelen, sizeof( uint32_t ) );
	  sftp_store( &next->mem, fname, fnamelen );
	}
      else
	{
	  uint32_t zero = 0;
	  sftp_store( &next->mem, &zero, sizeof( uint32_t ) );
	  sftp_store( &next->mem, &zero, 0 );
	}


      if( longname ) /* sftp_get_string succedeed */
	{
	  /* Write string length before */
	  sftp_store( &next->mem, &longnamelen, sizeof( uint32_t ) );
	  sftp_store( &next->mem, longname, longnamelen );
	}
      else
	{
	  uint32_t zero = 0;
	  sftp_store( &next->mem, &zero, sizeof( uint32_t ) );
	  sftp_store( &next->mem, &zero, 0 );
	}


      /* Write length before */
      sftp_store( &next->mem, &attriblen, sizeof( uint32_t ) );
      sftp_store( &next->mem, &a, attriblen );

      sftp_free_string( fname );
      sftp_free_string( longname );
    }
  
  /* Now we send a new readdir command */

  next->id = sftp_unique_id();

  sftp_set_msg( out, SSH_FXP_READDIR );
  sftp_set_id( out, next->id );

  sftp_put_string( out, next->handlelen, next->handle );

  next->nextfun = sftp_ls_main;
  next->last = SFTP_LS_MAIN;
}

/* End ls */


/* More general handlers */

void
sftp_handle_status(struct sftp_callback *next,
		   uint8_t msg,
		   uint32_t id,
		   struct sftp_input *in, 
		   struct sftp_output *out,
		   const struct sftp_callback *state)
{
    *next = *state;  
    
    assert( msg == SSH_FXP_STATUS && id == state->id );    
    assert( sftp_get_uint32( in, &next->retval ) > 0 );
    
    next->id = 0;
    next->nextfun = 0;
    next->last = SFTP_HANDLE_STATUS;
    
    if( state->handle ) /* Any old handles around */
      {
	/* We send a SSH_FXP_CLOSE (and ignore the answer) */
	id = sftp_unique_id(); 
	  
	sftp_set_msg( out, SSH_FXP_CLOSE );
	sftp_set_id( out, id );
	
	sftp_put_string( out, state->handlelen, state->handle );
	
	sftp_free_string( state->handle );
	
	next->handle = 0;
	next->handlelen = 0;
      }
}


void
sftp_handle_attrs(struct sftp_callback *next,
		  uint8_t msg,
		  uint32_t id,
		  struct sftp_input *in, 
		  struct sftp_output *out,
		  const struct sftp_callback *state)
{
  struct sftp_attrib a;

  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Not attrib but status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
  /* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_ATTRS && id == state->id );

  assert( sftp_get_attrib( in, &a ) > 0 );

  next->attrib = a;
  next->last = SFTP_HANDLE_ATTRS;

/*    printf( "Flags: %d, perms %d\n", a.flags, a.permissions ); */

  next->id = 0;
  next->nextfun = 0;
}

void
sftp_handle_name(struct sftp_callback *next,
		 uint8_t msg,
		 uint32_t id,
		 struct sftp_input *in, 
		 struct sftp_output *out,
		 const struct sftp_callback *state)
{
  uint32_t count;
  uint32_t i;

  *next = *state;

  if( msg == SSH_FXP_STATUS ) /* Not attrib but status? */
    {
      sftp_handle_status(next, msg, id, in, out, state );
      return;
    }
/* Otherwise we shouldn't be here FIXME: Fail gracefully?*/

  assert( msg == SSH_FXP_NAME && id == state->id); 

  assert( sftp_get_uint32( in, &count ) > 0 );

  for( i=0; i<count; i++ ) /* Do count times */
    {
      uint8_t* fname;
      uint8_t* longname;
      struct sftp_attrib a;
      uint32_t fnamelen;
      uint32_t longnamelen;

      uint32_t attriblen = sizeof( struct sftp_attrib );

      fname = sftp_get_string( in, &fnamelen ); /* Read filename, longname and attrib */
      longname = sftp_get_string( in, &longnamelen );

      assert( sftp_get_attrib( in, &a ) > 0 );
      /* Write string length before */
      sftp_store( &next->mem, &fnamelen, sizeof( uint32_t ) );
      sftp_store( &next->mem, fname, fnamelen );

      /* Write length before */

      sftp_store( &next->mem, &longnamelen, sizeof( uint32_t ) );
      sftp_store( &next->mem, longname, longnamelen );

      /* Write length before */

      sftp_store( &next->mem, &attriblen, sizeof( uint32_t ) );
      sftp_store( &next->mem, &a, attriblen );

      sftp_free_string( fname );
      sftp_free_string( longname );
    }

  next->last = SFTP_HANDLE_NAME;

  next->id = 0;
  next->nextfun = 0;
}

/* End general */

