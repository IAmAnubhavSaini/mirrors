/*
 * @(#) $Id: 25a55b1554e4f90d9233cee20cefa74a5fdfbb5c $
 *
 * sftp_c.h
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

#ifndef SFTP_C_H
#define SFTP_C_H

#define SFTP_VERSION 3
#define SFTP_BLOCKSIZE 16384

#include <sys/types.h>

#include <netinet/in.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "sftp.h"
#include "buffer.h"
#include "xmalloc.h"



struct sftp_mem {
  char* at;
  uint32_t size;
  uint32_t used;
};


struct sftp_callback;

typedef void
(*sftp_callback_func)(struct sftp_callback *next,
		      uint8_t msg, 
		      uint32_t id,
		      struct sftp_input *in,
		      struct sftp_output *out,
		      const struct sftp_callback *state);

struct sftp_callback
{
  sftp_callback_func nextfun;
  
  uint32_t id; /* Id - for which id is this callback valid? */
  
  off_t filepos; 
  off_t filesize;  /* Only used for informational purposes */

  int op_id; /* Operation ID - identifier for caller */
  
  int fd;
  uint32_t retval; /* Return value (if id & nextfun == NULL ) of LAST call */
  int last; /* What was the last callback? */
  
  uint32_t bad_status; /* Return value of FAILED operation */
  uint32_t last_bad;   /* Who got FAILED status */
  
  int localerr; /* Return value when we had an local error */
  int localerrno; /* errno */

  struct sftp_attrib attrib;
  struct sftp_mem mem;  /* Memory buffer */
  
  uint8_t *handle;      /* Handle string from open and opendir calls */
  uint32_t handlelen; 
};


void sftp_null_state(struct sftp_callback *s);

/* Allocates a string/buffer of the given size */
void sftp_alloc_mem(struct sftp_mem *s, int desired_size);

int sftp_free_mem( struct sftp_mem *mem );           /* Free that buffer */ 
int sftp_resize_mem( struct sftp_mem *mem, int newsize ); /* */

/* Suitable for new */
void sftp_null_mem(struct sftp_mem *s);
int sftp_toggle_mem( struct sftp_mem *mem );


int sftp_store( struct sftp_mem* mem, void* data, uint32_t datalen );
void* sftp_retrieve( struct sftp_mem *mem, uint32_t desired, uint32_t* realsize );

uint32_t sftp_rumask( uint32_t new );

void
sftp_attrib_from_stat(const struct stat *st, struct sftp_attrib* a);

/* Returns zero if there were no permission information. */
int
sftp_attrib_perms(const struct sftp_attrib* a,
		  mode_t *mode);

uint32_t sftp_unique_id(void);

void
sftp_symlink_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in, 
		  struct sftp_output *out,
		  const uint8_t *linkname, 
		  uint32_t linklen,
		  const uint8_t *targetname,
		  uint32_t targetlen);



void
sftp_rename_init(struct sftp_callback *state,
		 int op_id,
		 struct sftp_input *in, 
		 struct sftp_output *out,
		 const uint8_t *srcname, 
		 uint32_t srclen,
		 const uint8_t *dstname, 
		 uint32_t dstlen);



void
sftp_remove_init(struct sftp_callback *state,
		 int op_id,
		 struct sftp_input *in, 
		 struct sftp_output *out,
		 const uint8_t *name,
		 uint32_t namelen);

void
sftp_mkdir_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in, 
		struct sftp_output *out,
		const uint8_t *name,
		uint32_t namelen,
		struct sftp_attrib* a);

void
sftp_rmdir_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in, 
		struct sftp_output *out,
		const uint8_t *name,
		uint32_t namelen);


void
sftp_realpath_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in, 
		   struct sftp_output *out,
		   const uint8_t *name,
		   uint32_t namelen);

void
sftp_readlink_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in, 
		   struct sftp_output *out,
		   const uint8_t *name,
		   uint32_t namelen);


void
sftp_stat_init(struct sftp_callback *state,
	       int op_id,
	       struct sftp_input *in, 
	       struct sftp_output *out,
	       const uint8_t *name,
	       uint32_t namelen);


void
sftp_lstat_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in, 
		struct sftp_output *out,
		const uint8_t *name,
		uint32_t namelen);

void
sftp_fstat_init(struct sftp_callback *state,
		int op_id,
		struct sftp_input *in, 
		struct sftp_output *out,
		const uint8_t *handle,
		uint32_t handlelen);


void
sftp_setstat_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in, 
		  struct sftp_output *out,
		  const uint8_t *name,
		  uint32_t namelen,
		  struct sftp_attrib* attrib);


void
sftp_fsetstat_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input* in, 
		   struct sftp_output* out,
		   const uint8_t *handle,
		   uint32_t handlelen,
		   struct sftp_attrib* attrib);



/* Get to memory */

void
sftp_get_mem_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in,
		  struct sftp_output *out,
		  const uint8_t *name, 
		  uint32_t namelen,
		  struct sftp_mem *mem,
		  off_t startat);

void
sftp_get_mem_step_one(struct sftp_callback *next,
		      uint8_t msg,
		      uint32_t id,
		      struct sftp_input *in,
		      struct sftp_output *out,
		      const struct sftp_callback *state);


void
sftp_get_mem_main(struct sftp_callback *next,
		  uint8_t msg,
		  uint32_t id,
		  struct sftp_input *in,
		  struct sftp_output *out,
		  const struct sftp_callback *state);

/* End get to memory */

/* Get to file */

void
sftp_get_file_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const uint8_t *name, 
		   uint32_t namelen,
		   const uint8_t *fname, 
		   uint32_t fnamelen,
		   int cont);

void
sftp_get_file_step_one(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state);

void
sftp_get_file_step_two(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state);


void
sftp_get_file_main(struct sftp_callback *next,
		   uint8_t msg,
		   uint32_t id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const struct sftp_callback *state);

/* End get to file */

/* Put fromfile */

void
sftp_put_file_init(struct sftp_callback *state,
		   int op_id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const uint8_t *name,
		   uint32_t namelen,
		   const uint8_t *fname,
		   uint32_t fnamelen,
		   int cont);

void
sftp_put_file_step_one(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state);


void
sftp_put_file_main(struct sftp_callback *next,
		   uint8_t msg,
		   uint32_t id,
		   struct sftp_input *in,
		   struct sftp_output *out,
		   const struct sftp_callback *state);


void
sftp_put_file_do_fstat(struct sftp_callback *next,
		       uint8_t msg,
		       uint32_t id,
		       struct sftp_input *in,
		       struct sftp_output *out,
		       const struct sftp_callback *state);


void
sftp_put_file_handle_stat(struct sftp_callback *next,
			  uint8_t msg,
			  uint32_t id,
			  struct sftp_input *in,
			  struct sftp_output *out,
			  const struct sftp_callback *state);

/* End put from file */



/* Put from memory */

void
sftp_put_mem_init(struct sftp_callback *state,
		  int op_id,
		  struct sftp_input *in,
		  struct sftp_output *out,
		  const uint8_t *name, 
		  uint32_t namelen,
		  struct sftp_mem *mem,
		  off_t startat,
		  struct sftp_attrib a);

void
sftp_put_mem_step_one(struct sftp_callback *next,
		      uint8_t msg,
		      uint32_t id,
		      struct sftp_input *in,
		      struct sftp_output *out,
		      const struct sftp_callback *state);


void
sftp_put_mem_main(struct sftp_callback *next,
		  uint8_t msg,
		  uint32_t id,
		  struct sftp_input *in,
		  struct sftp_output *out,
		  const struct sftp_callback *state);

/* End put from memory */


void
sftp_ls_init(struct sftp_callback *state,
	     int op_id,
	     struct sftp_input *in,
	     struct sftp_output *out,
	     const uint8_t *dir,
	     uint32_t dirlen);

void
sftp_ls_step_one(struct sftp_callback *next,
		 uint8_t msg,
		 uint32_t id,
		 struct sftp_input *in,
		 struct sftp_output *out,
		 const struct sftp_callback *state);



void
sftp_ls_main(struct sftp_callback *next,
	     uint8_t msg,
	     uint32_t id,
	     struct sftp_input *in,
	     struct sftp_output *out,
	     const struct sftp_callback *state);

void
sftp_handle_status(struct sftp_callback *next,
		   uint8_t msg,
		   uint32_t id,
		   struct sftp_input *in, 
		   struct sftp_output *out,
		   const struct sftp_callback *state);

void
sftp_handle_attrs(struct sftp_callback *next,
		  uint8_t msg,
		  uint32_t id,
		  struct sftp_input *in, 
		  struct sftp_output *out,
		  const struct sftp_callback *state);


void
sftp_handle_name(struct sftp_callback *next,
		 uint8_t msg,
		 uint32_t id,
		 struct sftp_input *in, 
		 struct sftp_output *out,
		 const struct sftp_callback *state);

int sftp_handshake( 
		   struct sftp_input *in,
		   struct sftp_output *out
		   );



enum sftp_last {
  SFTP_HANDLE_STATUS = 16,
  SFTP_HANDLE_ATTRS = 17,
  SFTP_HANDLE_NAME = 18,
  SFTP_MKDIR_INIT = 19,
  SFTP_RMDIR_INIT = 20,
  SFTP_REALPATH_INIT = 21,
  SFTP_STAT_INIT = 22,
  SFTP_LSTAT_INIT = 23,
  SFTP_FSTAT_INIT = 24,
  SFTP_SETSTAT_INIT = 25,
  SFTP_FSETSTAT_INIT = 26,
  SFTP_GET_MEM_INIT = 27,
  SFTP_GET_FILE_INIT = 28,
  SFTP_GET_FILE_STEP_ONE = 29,
  SFTP_GET_MEM_STEP_ONE = 30,
  SFTP_GET_FILE_MAIN = 31,
  SFTP_GET_MEM_MAIN = 32,
  SFTP_PUT_MEM_INIT = 33,
  SFTP_PUT_FILE_INIT = 34,
  SFTP_PUT_MEM_STEP_ONE = 35,
  SFTP_PUT_FILE_STEP_ONE = 36,
  SFTP_PUT_MEM_MAIN = 37,
  SFTP_PUT_FILE_MAIN = 38,
  SFTP_PUT_FILE_DO_FSTAT = 39,
  SFTP_PUT_FILE_HANDLE_STAT = 40,
  SFTP_LS_MAIN = 41,
  SFTP_GET_FILE_STEP_TWO = 42,
  SFTP_READLINK_INIT = 43,
  SFTP_SYMLINK_INIT = 44,
  SFTP_RENAME_INIT = 45,
  SFTP_REMOVE_INIT = 46,
  SFTP_HANDLE_LAST_UNKNOWN = -1
};

#endif /* SFTP_H */


