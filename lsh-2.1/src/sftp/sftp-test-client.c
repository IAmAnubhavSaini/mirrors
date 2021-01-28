/* sftp-test-client.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001 Niels Möller, Pontus Sköld
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#include "io.h"
#include "sftp.h"
#include "xmalloc.h"
#include "client.h"
#include "werror.h"


#define SFTP_XFER_BLOCKSIZE 16384
#define SFTP_VERSION 3


static uint32_t 
sftp_client_new_id(void)
{
  /* Return a new (monotonically increasing) every time */
  static uint32_t curid=0;
  return curid++;
}

static int
sftp_client_get_id(struct sftp_input *i, uint32_t expected)
{
  uint32_t id;
  return (sftp_get_uint32(i, &id)
	  && (id == expected));
}

static int
sftp_client_get_status(struct sftp_input *i, uint32_t *status)
{
  int res;

  uint8_t *msg = NULL;
  uint8_t *language = NULL;
  uint32_t length;
  
  res = (sftp_get_uint32(i, status)
	 && (msg = sftp_get_string(i, &length))
	 && (language = sftp_get_string(i, &length)));

  sftp_free_string(msg); sftp_free_string(language);

  return res;
}

static void
fork_server(char *name,
	    struct client_ctx *ctx)
{
  /* [0] for reading, [1] for writing */
  int stdin_pipe[2];
  int stdout_pipe[2];

  if (pipe(stdin_pipe) < 0)
    fatal("Creating stdin_pipe failed.");

  if (pipe(stdout_pipe) < 0)
    fatal("Creating stdout_pipe failed.");

  switch(fork())
    {
    case -1:
      fatal("fork failed.");
    default: /* Parent */
      {
	close(stdin_pipe[0]);
	close(stdout_pipe[1]);
	
	ctx->i = sftp_make_input(stdout_pipe[0]);
	ctx->o = sftp_make_output(stdin_pipe[1]);

	return;
      }
    case 0: /* Child */
      if (dup2(stdin_pipe[0], STDIN_FILENO) < 0)
	_fatal("dup2 for stdin failed.");
      if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0)
	_fatal("dup2 for stdout failed.");
	
      close(stdin_pipe[0]);
      close(stdin_pipe[1]);
      close(stdout_pipe[0]);
      close(stdout_pipe[1]);
      
      execl(name, name, NULL);

      _fatal("execl failed.");
    }
}

/* The handshake packets are special, because they don't include any
 * request id. */
static int
client_handshake(struct client_ctx *ctx)
{
  uint8_t msg;
  uint32_t version;

  sftp_set_msg(ctx->o, SSH_FXP_INIT);
  sftp_set_id(ctx->o, SFTP_VERSION);

  if (!sftp_write_packet(ctx->o))
    return 0;

  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  return (sftp_get_uint8(ctx->i, &msg)
	  && (msg == SSH_FXP_VERSION)
	  && sftp_get_uint32(ctx->i, &version)
	  && (version == SFTP_VERSION)
	  && sftp_get_eod(ctx->i));
}   

static int
do_ls(struct client_ctx *ctx, const char *name)
{
  uint32_t id;
  uint32_t status;
  uint8_t* handle;
  uint32_t hlength;

  uint8_t msg;
  int lsloop=1;
  int failure=0;

  id=sftp_client_new_id();

  sftp_set_msg(ctx->o, SSH_FXP_OPENDIR); /* Send a OPENDIR message */
  sftp_set_id(ctx->o, id);
  sftp_put_string(ctx->o, strlen(name), name);
  
  if (!sftp_write_packet(ctx->o))
    return 0;

  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  /* None of these may fail */  
  if ( !(sftp_get_uint8(ctx->i, &msg)
	 && (msg==SSH_FXP_HANDLE)
	 && sftp_client_get_id(ctx->i, id)
	 && (handle=sftp_get_string(ctx->i, &hlength)) ))
    return 0;
  
  /* OK, we now have a successfull call and a handle to a directory. */
  
  while ( lsloop )
    {
      id=sftp_client_new_id();
      
      sftp_set_msg(ctx->o, SSH_FXP_READDIR);
      sftp_set_id(ctx->o, id);
      sftp_put_string(ctx->o, hlength, handle);
      
      if (!sftp_write_packet(ctx->o))
	return 0;
      
      if (sftp_read_packet(ctx->i) <= 0)
	return 0;
      
      if( !(sftp_get_uint8(ctx->i, &msg)
	    && sftp_client_get_id(ctx->i, id)))
	return 0;
      
      if ( msg == SSH_FXP_NAME )
	{
	  uint32_t count;
	  struct sftp_attrib a;
	  
	  sftp_get_uint32(ctx->i, &count );

	  while ( count-- )
	    {
	      uint8_t* fname;
	      uint32_t fnamel;
	      
	      uint8_t* lname;
	      uint32_t lnamel;

	      sftp_input_clear_strings(ctx->i);

	      if (! ( (fname=sftp_get_string_auto(ctx->i, &fnamel))
		      && (lname=sftp_get_string_auto(ctx->i, &lnamel))
		      && sftp_get_attrib(ctx->i, &a)))
		return 0;

	      printf("%s\n", lname);
	    }
	} 
      else
	if ( msg == SSH_FXP_STATUS )
	  {
	    if (!sftp_client_get_status(ctx->i, &status))
	      fatal("Invalid SSH_FXP_STATUS message.");
	    
	    lsloop=0; /* End of loop - EOF or failue */
	    
	    if ( status != SSH_FX_EOF)
	      failure=1;
	  }
    }

  /* Time to close */

  id=sftp_client_new_id();

  sftp_set_msg(ctx->o, SSH_FXP_CLOSE); /* Send a close message */
  sftp_set_id(ctx->o, id);
  sftp_put_string(ctx->o, hlength, handle);

  sftp_free_string(handle);
  
  if (!sftp_write_packet(ctx->o))
    return 0;

  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  return (sftp_get_uint8(ctx->i, &msg)
	  && (msg==SSH_FXP_STATUS)
	  && sftp_client_get_id(ctx->i, id)
	  && sftp_client_get_status(ctx->i, &status)
	  && (status == SSH_FX_OK)
	  && !failure);
}

static uint8_t *
do_open(struct client_ctx *ctx, 
	const char *name,
	uint32_t flags,
	const struct sftp_attrib *a,
	uint32_t *handle_length)
{
  uint8_t msg;
  uint32_t id = sftp_client_new_id();
  
  /* Send a OPEN message */
  sftp_set_msg(ctx->o, SSH_FXP_OPEN);
  sftp_set_id(ctx->o, id);
  sftp_put_string(ctx->o, strlen(name), name );
  sftp_put_uint32(ctx->o, flags);
  sftp_put_attrib(ctx->o, a);

  if (!sftp_write_packet(ctx->o))
    return 0;

  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  /* None of these may fail */
  return (sftp_get_uint8(ctx->i, &msg)
	  && (msg==SSH_FXP_HANDLE)
	  && sftp_client_get_id(ctx->i, id))
    ? sftp_get_string(ctx->i, handle_length)
    : NULL;
}

static int
do_close(struct client_ctx *ctx, 
	 uint32_t handle_length,
	 const uint8_t *handle)
{
  uint8_t msg;
  uint32_t status;

  uint32_t id = sftp_client_new_id();

  sftp_set_msg(ctx->o, SSH_FXP_CLOSE); /* Send a close message */
  sftp_set_id(ctx->o, id);
  sftp_put_string(ctx->o, handle_length, handle);

  if (!sftp_write_packet(ctx->o))
    return 0;

  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  /* None of these may fail */
  return (sftp_get_uint8(ctx->i, &msg)
	  && (msg==SSH_FXP_STATUS)
	  && sftp_client_get_id(ctx->i, id)
	  && sftp_client_get_status(ctx->i, &status)
	  && (status == SSH_FX_OK));
}

static int
do_get(struct client_ctx *ctx, 
       const char *name,
       int dst)
{
  uint32_t id;
  uint8_t* handle;
  uint32_t handle_length;
  uint32_t status;

  off_t curpos=0;

  struct sftp_attrib a;
  
  uint8_t msg;
  int getloop=1;
  int ok = 1;

  sftp_clear_attrib(&a); /* Don't pass any information on how to open */

  if (! (handle = do_open(ctx, name, SSH_FXF_READ, &a, &handle_length)))
    return 0;
  
  /* OK, we now have a successfull call and file handle */
     
  id=sftp_client_new_id();

  while ( getloop )
    {
      id=sftp_client_new_id();
      
      sftp_set_msg(ctx->o, SSH_FXP_READ); /* Send a read request */
      sftp_set_id(ctx->o, id);
      sftp_put_string(ctx->o, handle_length, handle);
      sftp_put_uint64(ctx->o, curpos);
      sftp_put_uint32(ctx->o, SFTP_XFER_BLOCKSIZE);

      if (!sftp_write_packet(ctx->o))
	return 0;
      
      if (sftp_read_packet(ctx->i) <= 0)
	return 0;
      
      if( !(sftp_get_uint8(ctx->i, &msg)
	    && sftp_client_get_id(ctx->i, id)))
	return 0;
      
      switch (msg)
	{
	case SSH_FXP_DATA:
	  {
	    uint8_t *data;
	    uint32_t length;
	  
	    data = sftp_get_string_auto(ctx->i, &length);
	    curpos += length;
	  
	    while (length)
	      {
		int res;
		do
		  res = write(dst, data, length);
		while ( (res < 0) && (errno = EINTR) );
		if (res < 0)
		  {
		    getloop=0; 
		    ok=0;
		    break;
		  }
		data +=res;
		length -= res;
	      }
	    break;
	  } 
	case SSH_FXP_STATUS:
	  {
	    if (!sftp_client_get_status(ctx->i, &status))
	      fatal("Invalid SSH_FXP_STATUS message.");

	    getloop=0; /* End of loop - EOF or failue */
	  
	    if ( status != SSH_FX_EOF)
	      ok=0;

	    break;
	  }
	default:
	  return 0;
	}
    }

  /* Time to close */

  if (!do_close(ctx, handle_length, handle))
    ok = 0;
  
  sftp_free_string(handle);

  return ok;
}

static int
do_put(struct client_ctx *ctx,
       const char *name, 
       int fd)
{
  uint32_t id;
  uint32_t status;
  uint8_t* handle;
  uint32_t handle_length;
  off_t curpos=0;

  struct sftp_attrib a;
  
  uint8_t msg;
  int putloop=1;
  int ok = 1;
  
  sftp_clear_attrib(&a); 
  
  a.flags = 0;
  
  if (! (handle = do_open(ctx, name, SSH_FXF_CREAT | SSH_FXF_WRITE, &a,
			  &handle_length)))
    return 0;

  /* OK, we now have a successfull call and file handle */
  
  while ( putloop )
    {
      uint8_t buf[SFTP_XFER_BLOCKSIZE];
      int res;
      
      do
	res = read(fd, buf, sizeof(buf));
      while ( (res < 0) && (errno == EINTR) );

      if (res < 0)
	{
	  putloop=0;
	  ok = 0;
	  break;
	}
      
      if ( !res )
	putloop=0;
      else
	{
	  id=sftp_client_new_id();
	  
	  sftp_set_msg(ctx->o, SSH_FXP_WRITE); /* Send a read request */
	  sftp_set_id(ctx->o, id);
	  sftp_put_string(ctx->o, handle_length, handle);
	  sftp_put_uint64(ctx->o, curpos);
	  sftp_put_string(ctx->o, res, buf);
	  
	  if (!sftp_write_packet(ctx->o))
	    return 0;
	  
	  curpos += res;
	  
	  if (!sftp_read_packet(ctx->i))
	    return 0;
	  
	  if( !(sftp_get_uint8(ctx->i, &msg)
		&& sftp_client_get_id(ctx->i, id)))
	    return 0;
	  
	  if ( msg == SSH_FXP_STATUS )
	    {
	      if (!sftp_client_get_status(ctx->i, &status))
		fatal("Invalid SSH_FXP_STATUS message.");
		
	      putloop=0; /* End of loop - EOF or failue */
	      
	      if ( status != SSH_FX_OK )
		ok = 0;
	    }
	}
    }

  /* Time to close */

  if (!do_close(ctx, handle_length, handle))
    ok = 0;

  sftp_free_string(handle);

  return ok;
}

static int
do_stat(struct client_ctx *ctx, const char *name)
{
  uint32_t id;
  uint8_t msg;
  struct sftp_attrib a;

  id=sftp_client_new_id();
  
  sftp_set_msg(ctx->o, SSH_FXP_STAT);
  sftp_set_id(ctx->o, id);
  sftp_put_string(ctx->o, strlen(name), name);

  if (!sftp_write_packet(ctx->o))
    return 0;

  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  if ( !(sftp_get_uint8(ctx->i, &msg) &&  /* None of these may fail */
	 msg==SSH_FXP_ATTRS &&
	 sftp_client_get_id(ctx->i, id) &&
	 sftp_get_attrib(ctx->i, &a) 	 
	 ))
    return 0;

  /* Fixme; return this somehow */
  printf("Stat succeeded\n");
  
  return 1;
}

const char *werror_program_name = "sftp-test-client";

int
main(int argc, char **argv)
{
  struct client_ctx ctx;
  int i;
  
  if (argc < 2)
    fatal("Too few args.");

  fork_server(argv[1], &ctx);

  if (!client_handshake(&ctx))
    fatal("Handshake failed.");

  for (i = 2; i < argc; i += 2)
    {
      int res; 
      switch (argv[i][0])
	{
	case 'l': /* ls */
	  /* Depends on argv[argc] == NULL */
	  res = do_ls(&ctx, argv[i+1]);
	  break;
	case 'g': /* get */
	  res = do_get(&ctx, argv[i+1], STDOUT_FILENO);
	  break;
	case 'p':
	  res = do_put(&ctx, argv[i+1], STDIN_FILENO);
	  break;
	case 's':
	  res = do_stat(&ctx, argv[i+1]);
	  break;
	default:
	  fatal("Bad arg");
	}
      if (!res)
	exit(EXIT_FAILURE);
    }
  
  return EXIT_SUCCESS;
}
