/* sftp-server.c
 *
 * $Id: 4c66043fa0d0637c99b3972f3d1ac83966500d10 $
 *
 * The server side of the sftp subsystem. */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001 Markus Friedl, Niels Möller, Pontus Sköld
 *
 * Also includes parts from GNU fileutils, Copyright by Free Software
 * Foundation, Inc.
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

/* Some of this code is written by Markus Friedl, and licensed as follows,
 *
 * Copyright (c) 2000 Markus Friedl.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
   
#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/stat.h>

#include <dirent.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#include "io.h"
#include "sftp.h"

#include "filemode.h"
#include "idcache.h"
#include "xmalloc.h"
#include "werror.h"

const char *werror_program_name = "sftp-server";

#define SFTP_VERSION 3


#define MAX_DIRENTS_PER_REQUEST 100

#define WITH_DEBUG 1

#if WITH_DEBUG
static int debug_flag = 0;

static void
debug(const char *format, ...) PRINTF_STYLE(1,2);

static void
debug(const char *format, ...)
{
  if (debug_flag)
    {
      va_list args;
  
      va_start(args, format);
      vfprintf(stderr, format, args);
      va_end(args);
    }
}
# define DEBUG(x) debug x
#else /* !WITH_DEBUG */
# define DEBUG(x)
#endif /* !WITH_DEBUG */

#define SFTP_MAX_HANDLES 200

/* We need to keep the name of directories around, in order to stat
 * entries. */
struct sftp_dir
{
  DIR *dir;
  char *name;
};

/* And we need the position, so we know if we need to seek.
 * Not all fd:s have an internal file position */
struct sftp_file
{
  int fd;
  off_t position;
};

struct sftp_handle
{
  enum sftp_handle_type
  { HANDLE_UNUSED = 0, HANDLE_TYPE_FILE, HANDLE_TYPE_DIRECTORY } type;

  union
  {
    struct sftp_file file;
    struct sftp_dir dir;
  } u;
};

#define HANDLE_TYPE(ctx, handle) ((ctx)->handles[handle].type)

#define HANDLE_DIR(ctx, handle) ((ctx)->handles[handle].u.dir)
#define HANDLE_FILE(ctx, handle) ((ctx)->handles[handle].u.file)

struct sftp_ctx
{
  struct sftp_input *i;
  struct sftp_output *o;
  struct sftp_user_info *user_cache;
  struct sftp_user_info *group_cache;
  
  struct sftp_handle handles[SFTP_MAX_HANDLES];
};

static void
sftp_init(struct sftp_ctx *ctx, int in, int out)
{
  unsigned i;
  
  ctx->i = sftp_make_input(in);
  ctx->o = sftp_make_output(out);

  ctx->user_cache = NULL;
  ctx->group_cache = NULL;
  
  for (i = 0; i < SFTP_MAX_HANDLES; i++)
    ctx->handles[i].type = HANDLE_UNUSED;
}

static const char *
sftp_get_user(struct sftp_ctx *ctx, uid_t id)
{
  const char *name;
  
  if (sftp_cache_assoc(&ctx->user_cache, id, &name))
    return name;

  else
    {
      struct passwd *pwd = getpwuid(id);
      name = pwd ? xstrdup(pwd->pw_name) : NULL;
      sftp_cache_push(&ctx->user_cache, id, name);
      return name;
    }
}

static const char *
sftp_get_group(struct sftp_ctx *ctx, gid_t id)
{
  const char *name;
  
  if (sftp_cache_assoc(&ctx->group_cache, id, &name))
    return name;

  else
    {
      struct group *grp = getgrgid(id);
      name = grp ? xstrdup(grp->gr_name) : NULL;
      sftp_cache_push(&ctx->group_cache, id, name);
      return name;
    }
}

static void
sftp_attrib_from_stat(struct stat *st, struct sftp_attrib* a)
{
  a->permissions=st->st_mode;
  a->uid=st->st_uid;
  a->gid=st->st_gid;
  a->atime=st->st_atime;
  a->mtime=st->st_mtime;
  a->size=st->st_size;
  a->flags= ( SSH_FILEXFER_ATTR_SIZE
              | SSH_FILEXFER_ATTR_UIDGID
              | SSH_FILEXFER_ATTR_PERMISSIONS
              | SSH_FILEXFER_ATTR_ACMODTIME);
}

static void
sftp_put_longname_mode(struct sftp_output *o, struct stat *st)
{
  /* A 10 character modestring and a space */
  uint8_t modes[MODE_STRING_LENGTH];

  filemodestring(st, modes);

  sftp_put_data(o, sizeof(modes), modes);
}

static void
sftp_put_longname(struct sftp_ctx *ctx,
		  struct stat *st, const uint8_t *fname)
{
  /* NOTE: The current spec doesn't mandate utf8. */

  /* Where to store the length. */
  uint32_t length_index = sftp_put_reserve_length(ctx->o);
  const char *user_name;
  const char *group_name;
  time_t now, when;
  struct tm *when_local;
  const char *time_format;
  
  sftp_put_longname_mode(ctx->o, st);
  sftp_put_printf(ctx->o, " %3u ", (unsigned) st->st_nlink);

  user_name = sftp_get_user(ctx, st->st_uid);
  if (user_name)
    sftp_put_printf(ctx->o, "%-8.8s ", user_name);
  else
    sftp_put_printf(ctx->o, "%-8u ", (unsigned int) st->st_uid);

  group_name = sftp_get_group(ctx, st->st_gid);
  if (group_name)
    sftp_put_printf(ctx->o, "%-8.8s ", group_name);
  else
    sftp_put_printf(ctx->o, "%-8u ", (unsigned) st->st_gid);

  /* FIXME: How to deal with long long sizes? */
  sftp_put_printf(ctx->o, "%8lu ", (unsigned long) st->st_size);

  now = time(NULL);
  when = st->st_mtime;

  when_local = localtime( &st->st_mtime );

  if ( (now > when + 6L * 30L * 24L * 60L * 60L)	/* Old. */
       || (now < when - 60L * 60L) )		/* In the future. */
    /* The file is fairly old or in the future.
       POSIX says the cutoff is 6 months old;
       approximate this by 6*30 days.
       Allow a 1 hour slop factor for what is considered "the future",
       to allow for NFS server/client clock disagreement.
       Show the year instead of the time of day.  */
    time_format = "%b %e  %Y";
  else
    time_format = "%b %e %H:%M";
      
  sftp_put_strftime(ctx->o, 12, time_format, when_local);

  sftp_put_printf(ctx->o, " %s", fname);

  sftp_put_final_length(ctx->o, length_index);
}

static void
sftp_put_filename(struct sftp_ctx *ctx,
		  struct stat *st,
		  const char *name)
{
  struct sftp_attrib a;
  
  sftp_attrib_from_stat( st, &a);
  sftp_put_string(ctx->o, strlen(name), name);
  sftp_put_longname(ctx, st, name);
  sftp_put_attrib(ctx->o, &a);
}

/* FIXME: We need to distinguish invalid messages from invalid file
   names. */
static const uint8_t *
sftp_get_name(struct sftp_input *i)
{
  uint8_t *name;
  uint32_t length;
  
  name = sftp_get_string_auto(i, &length);
  if (!length)
    name = ".";

  else if (memchr(name, 0, length))
    name = NULL;

  DEBUG (("sftp_get_name: %s\n", name ? (char *) name : "<INVALID>"));

  return name;
}

static int 
sftp_handle_used(struct sftp_ctx *ctx, uint32_t handle)
{
  return (handle < SFTP_MAX_HANDLES)
    && (ctx->handles[handle].type != HANDLE_UNUSED);
}

static int
sftp_new_handle(struct sftp_ctx *ctx,
		enum sftp_handle_type type,
		uint32_t *handle)
{
  uint32_t i;

  assert(type != HANDLE_UNUSED);

  /* FIXME: This linear search is suboptimal */
  for (i = 0; i<SFTP_MAX_HANDLES; i++)
    if (HANDLE_TYPE(ctx, i) == HANDLE_UNUSED)
      {
	DEBUG (("sftp_new_handle: %d, type %d\n", i, type));

	HANDLE_TYPE(ctx, i) = type;
	*handle = i;
	return 1;
      }

  DEBUG (("sftp_new_handle: ran out of handles\n"));
  
  return 0;
}

static int
sftp_get_handle(struct sftp_ctx *ctx, uint32_t *handle)
{
  uint32_t length;
  uint32_t value;
  
  if (sftp_get_uint32(ctx->i, &length)
      && (length == 4)
      && sftp_get_uint32(ctx->i, &value)
      && sftp_handle_used(ctx, value))
    {
      *handle = value;
      return 1;
    }
  return 0;
}

static int
sftp_get_handle_dir(struct sftp_ctx *ctx, struct sftp_dir **value)
{
  uint32_t handle;

  if (sftp_get_handle(ctx, &handle)
      && (HANDLE_TYPE(ctx, handle) == HANDLE_TYPE_DIRECTORY))
    {
      *value = &HANDLE_DIR(ctx, handle);
      return 1;
    }
  else
    return 0;
}

static int
sftp_get_handle_file(struct sftp_ctx *ctx, struct sftp_file **f)
{
  uint32_t handle;

  if (sftp_get_handle(ctx, &handle)
      && (HANDLE_TYPE(ctx, handle) == HANDLE_TYPE_FILE))
    {
      *f = &HANDLE_FILE(ctx, handle);
      return 1;
    }
  else
    return 0;
}

static int
sftp_get_handle_fd(struct sftp_ctx *ctx, int *fd)
{
  struct sftp_file *f;

  if (sftp_get_handle_file(ctx, &f))
    {
      *fd = f->fd;
      return 1;
    }
  else
    return 0;
}

static void
sftp_put_handle(struct sftp_ctx *ctx, uint32_t handle)
{
  sftp_put_uint32(ctx->o, 4);
  sftp_put_uint32(ctx->o, handle);
}

static int
sftp_send_handle(struct sftp_ctx *ctx, uint32_t handle)
{
  sftp_set_msg(ctx->o, SSH_FXP_HANDLE);
  sftp_put_handle(ctx, handle);
  return 1;
}

/* FIXME: Add an argument for the the human-readable message. */
static int
sftp_send_status(struct sftp_ctx *ctx, uint32_t status)
{
  DEBUG (("sftp_send_status: %d\n", status));
  
  sftp_set_msg(ctx->o, SSH_FXP_STATUS);
  sftp_put_uint32(ctx->o, status);

  switch(status)
    {
    case SSH_FX_OK:
      sftp_put_string(ctx->o, strlen("Success"), "Success");
      break;

    case SSH_FX_EOF:
      sftp_put_string(ctx->o, strlen("End of file"), "End of file");
      break;
    case SSH_FX_NO_SUCH_FILE:
      sftp_put_string(ctx->o, strlen("No such file"), "No such file");
      break;
    case SSH_FX_PERMISSION_DENIED:
      sftp_put_string(ctx->o, strlen("Permission denied"), "Permission denied");
      break;    
    case SSH_FX_BAD_MESSAGE:
      sftp_put_string(ctx->o, strlen("Bad message"), "Bad message");
      break;
#if 0
    case SSH_FX_NO_CONNECTION:
      /* We must never send this */
      fatal( "Status SSH_FX_NO_CONNECTION" );
      break;
    case SSH_FX_CONNECTION_LOST:
      /* We must never send this */
      fatal( "Status SSH_FX_CONNECTION_LOST" );
      break;
#endif
    case SSH_FX_OP_UNSUPPORTED:    
      sftp_put_string(ctx->o, strlen("Operation unsupported"), "Operation unsupported");
      break;
    case SSH_FX_FAILURE:
    default:
      sftp_put_string(ctx->o, strlen("Failure"), "Failure");
      break;
    }

  sftp_put_string(ctx->o, strlen("en"), "en");    /* Send english language tag */
  return 1;
}

static int
sftp_send_errno(struct sftp_ctx *ctx, int e)
{
  uint32_t status;
  
  DEBUG (("sftp_send_errno: %d, %s\n", e, strerror(e)));
  switch(e)
    {
    case ENOENT:
      status = SSH_FX_NO_SUCH_FILE;
      break;
    case EACCES:
      status = SSH_FX_PERMISSION_DENIED;
      break;
    default:
      status = SSH_FX_FAILURE;
      break;
    }
  return sftp_send_status(ctx, status);
}

static int
sftp_bad_message(struct sftp_ctx *ctx)
{
  sftp_send_status(ctx, SSH_FX_BAD_MESSAGE);
  return 0;
}

typedef int sftp_process_func(struct sftp_ctx *ctx);

static int
sftp_process_unsupported(struct sftp_ctx *ctx)
{
  return sftp_bad_message(ctx);
}

static int
sftp_process_opendir(struct sftp_ctx *ctx)
{
  const uint8_t *name;
  DIR* dir;
  uint32_t handle;

  DEBUG (("sftp_process_opendir\n"));
  
  if ( ! (name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  /* FIXME: Perhaps we should have a sftp_mangle_fname? If not, we
     have to handle an empty filename here */

  dir=opendir(name);

  if (!dir)
    return sftp_send_errno(ctx, errno);

  /* Open successful */

  if (!sftp_new_handle(ctx, HANDLE_TYPE_DIRECTORY, &handle))
    return sftp_send_status(ctx, SSH_FX_FAILURE);

  HANDLE_DIR(ctx, handle).dir = dir;
  HANDLE_DIR(ctx, handle).name = strdup(name);

  return sftp_send_handle(ctx, handle);
}

/* FIXME: Creative use of dirfd and fchdir would be more robust. */
static int
sftp_lstat_in_dir(const char *dir, const char *file,
		  struct stat *sb)
{
  unsigned dir_length = strlen(dir);
  unsigned file_length = strlen(file);
  char *s = alloca(dir_length + file_length + 2);

  memcpy(s, dir, dir_length);
  s[dir_length] = '/';
  memcpy(s + dir_length + 1, file, file_length);
  s[dir_length + file_length + 1] = '\0';

  return lstat(s, sb);
}

static int
sftp_process_readdir(struct sftp_ctx *ctx)
{
  struct sftp_dir *dir;
  struct dirent *entry; 
  char* filenames[MAX_DIRENTS_PER_REQUEST];
  struct stat st[MAX_DIRENTS_PER_REQUEST];
  int i;
  int used_entries = 0;

  if (!sftp_get_handle_dir(ctx, &dir))
    return sftp_bad_message(ctx);

  while (used_entries < MAX_DIRENTS_PER_REQUEST )
    {
      /* readdir doesn't modify errno on EOF, so we need to clear it
       * first. */
      errno = 0;
      
      entry = readdir(dir->dir);
      
      if (!entry) /* Something's wrong */
	{
	  /* errno == 0 => No more dirents, errno != 0 => Error */
	 
	  /* If we have already successfully read entries, ignore this
	   * situation for the moment and trust it to repeat itself on
	   * subsequent readdirs */

	  if (!used_entries)   /* First time? */
	    return (errno ? sftp_send_errno(ctx, errno)
		    : sftp_send_status(ctx, SSH_FX_EOF)); 
	  else
	    break; /* We have data to send, so send it */
	}
      else /* Successfully read entry */
	{
	  if (sftp_lstat_in_dir(dir->name, entry->d_name, &st[used_entries]) )
	    /* FIXME: Would it be better to just skip this entry, or send an
	     * SSH_FXP_NAME with no file attributes? */
	    {
	      for(i=0; i<used_entries; i++)
		free(filenames[i]);
	      return sftp_send_errno(ctx, errno);
	    }

	  filenames[used_entries] = strdup(entry->d_name);

	  if (!filenames[used_entries]) /* strdup failed? */
	    {
	      for(i=0; i<used_entries; i++)
		free(filenames[i]);
	      return sftp_send_errno(ctx, errno);
	    }

	  used_entries++; /* Everything OK, skip to next entry */
	}
    }

  if (!used_entries) /* This should never happen */
    return sftp_send_status(ctx, SSH_FX_EOF);
  
  sftp_put_uint32(ctx->o, used_entries);
  
  for (i=0; i<used_entries; i++)
    {
      sftp_put_filename(ctx, &st[i], filenames[i]);
      free(filenames[i]);
    }
  
  sftp_set_msg(ctx->o, SSH_FXP_NAME );
  return 1;
}

static int
sftp_process_stat(struct sftp_ctx *ctx)
{
  struct stat st;
  struct sftp_attrib a;

  const uint8_t *name;

  if ( ! (name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  if (stat(name, &st ) < 0)
    return sftp_send_errno(ctx, errno);

  sftp_attrib_from_stat( &st, &a);

  sftp_set_msg( ctx->o, SSH_FXP_ATTRS );
  sftp_put_attrib( ctx->o, &a);  

  return 1;
}

static int
sftp_process_lstat(struct sftp_ctx *ctx)
{
  struct stat st;
  struct sftp_attrib a;

  const uint8_t *name;

  if ( !(name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  if (lstat(name, &st) < 0)
    return sftp_send_errno(ctx, errno);

  sftp_attrib_from_stat( &st, &a );
  sftp_set_msg( ctx->o, SSH_FXP_ATTRS );

  sftp_put_attrib( ctx->o, &a); 

  return 1;
}

/* FIXME: Implement fstat for directories as well. */
static int
sftp_process_fstat(struct sftp_ctx *ctx)
{
  struct stat st;
  struct sftp_attrib a;
  int fd;

  if (!sftp_get_handle_fd(ctx, &fd) )
    return sftp_bad_message(ctx);
  
  
  if (fstat(fd, &st ) < 0 )
    return sftp_send_errno(ctx, errno);
	  
  sftp_attrib_from_stat(&st,&a);
  sftp_set_msg( ctx->o, SSH_FXP_ATTRS );
  
  sftp_put_attrib( ctx->o, &a);
  return 1;
}

static int
sftp_process_fsetstat(struct sftp_ctx *ctx)
{
  struct sftp_attrib a;
  int fd;

  if (sftp_get_handle_fd(ctx, &fd) &&
      sftp_get_attrib(ctx->i, &a))
    {
      /* FIXME: What is the right order for doing this? */
	  
      if ( a.flags & SSH_FILEXFER_ATTR_UIDGID )
	if (fchown(fd, a.uid, a.gid ) < 0 )
	  return sftp_send_errno(ctx, errno);
  
      if ( a.flags & SSH_FILEXFER_ATTR_SIZE )
	if( ftruncate(fd, a.size ) < 0)
	  return sftp_send_errno(ctx, errno);
  
      if ( a.flags & SSH_FILEXFER_ATTR_PERMISSIONS )
	if( fchmod(fd, a.permissions ) < 0 ) 
	  return sftp_send_errno(ctx, errno);
  
      if ( a.flags & (SSH_FILEXFER_ATTR_EXTENDED | SSH_FILEXFER_ATTR_ACMODTIME ))
	/* FIXME: how do we? */
	return sftp_send_status(ctx, SSH_FX_OP_UNSUPPORTED );
  
      return sftp_send_status(ctx, SSH_FX_OK);
    }
  else
    return sftp_bad_message(ctx);
}

static int
sftp_process_setstat(struct sftp_ctx *ctx)
{
  struct sftp_attrib a;

  const uint8_t *name;

  if ( !((name = sftp_get_name(ctx->i))
	 && sftp_get_attrib(ctx->i, &a) ))
    return sftp_bad_message(ctx);
  else
    {
      if ( a.flags & SSH_FILEXFER_ATTR_UIDGID )
	if ( chown(name, a.uid, a.gid ) )
	  return sftp_send_errno(ctx, errno);

      if ( a.flags & SSH_FILEXFER_ATTR_SIZE )
	if( truncate(name, a.size ) )
	  return sftp_send_errno(ctx, errno);

      if ( a.flags & SSH_FILEXFER_ATTR_PERMISSIONS )
	if( chmod(name, a.permissions ) )
	  return sftp_send_errno(ctx, errno);
      
      if ( a.flags & (SSH_FILEXFER_ATTR_EXTENDED | SSH_FILEXFER_ATTR_ACMODTIME))
	  /* FIXME: how do we? */
	  return sftp_send_status(ctx, SSH_FX_OP_UNSUPPORTED );
      
      return sftp_send_status(ctx, SSH_FX_OK);
    }
}

static int
sftp_process_remove(struct sftp_ctx *ctx)
{
  const uint8_t *name;

  if ( ! (name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  if (unlink(name) < 0)
    return sftp_send_errno(ctx, errno);
  else
    return sftp_send_status(ctx, SSH_FX_OK);   
}

/* NOTE: This request ought to include the desired attributes. */
static int
sftp_process_mkdir(struct sftp_ctx *ctx)
{
  const uint8_t *name;
  struct sftp_attrib a;
  int fail;

  if ( !((name = sftp_get_name(ctx->i))
         && sftp_get_attrib(ctx->i, &a) ))
    return sftp_bad_message(ctx);

  if (a.flags & SSH_FILEXFER_ATTR_PERMISSIONS)
    {
      mode_t old = umask(0);

      fail = mkdir(name, a.permissions & 0777);
      umask(old);
    }
  else
    fail = mkdir(name, 0755); /* FIXME: Default permissions? */

  if (fail < 0 )
    return sftp_send_errno(ctx, errno);
  else
    return sftp_send_status(ctx, SSH_FX_OK);   
}

static int
sftp_process_rmdir(struct sftp_ctx *ctx)
{
  const uint8_t *name;
  
  if (! (name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  if(rmdir(name) < 0)
    return sftp_send_errno(ctx, errno);
  else
    return sftp_send_status(ctx, SSH_FX_OK);   
}

static int
sftp_process_realpath(struct sftp_ctx *ctx)
{
  const uint8_t *name;
  uint8_t *resolved;
  struct stat st;
  int path_max;

  if (! (name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  /* Code below from the manpage for realpath on my debian system */

#ifdef PATH_MAX
  path_max = PATH_MAX;
#else
  path_max = pathconf (name, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif

  resolved = realpath(name, alloca(path_max));

  if (!resolved)
    return sftp_send_errno(ctx, errno);

  sftp_put_uint32(ctx->o, 1); /* Count */  
  
  /* FIXME: The draft says we should return "just one name and dummy
   * attributes", but I figure we might as well pass the real attributes,
   * if we can get them. */
  
  if (lstat(resolved, &st ) < 0)
    {
      /* Return dummy attributes. */
      sftp_put_string(ctx->o, strlen(resolved), resolved);

      /* Leave the "long name" field empty. */
      sftp_put_uint32(ctx->o, 0);

      /* ATTRS data, containing only a zer flags field. */
      sftp_put_uint32(ctx->o, 0);
    }
  else
    sftp_put_filename(ctx, &st, resolved);

  DEBUG (("sftp_process_realpath: Resolved %s to %s\n", name, resolved));

  sftp_set_msg( ctx->o, SSH_FXP_NAME );
  return 1;
}

static int
sftp_process_rename(struct sftp_ctx *ctx)
{
  const uint8_t *src;
  const uint8_t *dst;

  if (! ( (src = sftp_get_name(ctx->i))
	  && (dst = sftp_get_name(ctx->i))) )
    return sftp_bad_message(ctx);
  
  if (rename(src, dst) < 0)
    return sftp_send_errno(ctx, errno);
  else
    return sftp_send_status(ctx, SSH_FX_OK);   
}


static int
sftp_process_open(struct sftp_ctx *ctx)
{
  const uint8_t *name;
  uint32_t pflags;
  struct sftp_attrib a;

  if (!((name = sftp_get_name(ctx->i))
	&& sftp_get_uint32(ctx->i, &pflags)
	&& sftp_get_attrib(ctx->i, &a)))
    return sftp_bad_message(ctx);
  else
    {
      int fd;
      int mode;
      struct stat sb;
      uint32_t handle;
      
      switch (pflags & (SSH_FXF_READ | SSH_FXF_WRITE))
	{
	case 0:
	  sftp_send_status(ctx, SSH_FX_FAILURE);
	  return 1;
	case SSH_FXF_READ:
	  mode = O_RDONLY;
	  break;
	case SSH_FXF_WRITE:
	  mode = O_WRONLY;
	  break;
	case SSH_FXF_READ | SSH_FXF_WRITE:
	  mode = O_RDWR;
	  break;
	default:
	  abort();
	}
      if (pflags & SSH_FXF_APPEND)
	mode |= O_APPEND;

      if (pflags & SSH_FXF_CREAT)
	mode |= O_CREAT;
      else if (pflags & SSH_FXF_EXCL)
	/* We can't have EXCL without CREAT. */
	sftp_send_status(ctx, SSH_FX_FAILURE);

      if (pflags & SSH_FXF_TRUNC)
	mode |= O_TRUNC;
      if (pflags & SSH_FXF_EXCL)
	mode |= O_EXCL;

      /* Look at the atributes. For now, we care only about the
       * permission bits. */
      if (a.flags & SSH_FILEXFER_ATTR_PERMISSIONS)
	{
	  /* Use the client's permission bits with no umask filtering */
	  mode_t old = umask(0);
	  fd = open(name, mode, a.permissions);
	  umask(old);
	}
      else
	/* Default flags, filtered through our umask */
	fd = open(name, mode, 0666);

      if (fd < 0)
	return sftp_send_errno(ctx, errno);

#if 0
      if (a.flags & SSH_FILEXFER_ATTR_UIDGID)
	if ( fchown(fd, a.uid, a.gid) )
	  return sftp_send_errno(ctx, errno);
#endif    

      if (fstat(fd, &sb) < 0)
	{
	  close(fd);
	  return sftp_send_errno(ctx, errno);
	}

      if (S_ISDIR(sb.st_mode))
	{
	  close(fd);
	  return sftp_send_status(ctx, SSH_FX_NO_SUCH_FILE);
	}

      if (!sftp_new_handle(ctx, HANDLE_TYPE_FILE, &handle))
	return sftp_send_status(ctx, SSH_FX_FAILURE);

      DEBUG (("sftp_process_open: handle = %d, fd = %d\n", handle, fd));
      
      HANDLE_FILE(ctx, handle).fd = fd;
      HANDLE_FILE(ctx, handle).position = 0;

      sftp_send_handle(ctx, handle);
      return 1;
    }
}


static int
sftp_process_close(struct sftp_ctx *ctx)
{
  uint32_t handle;

  if (sftp_get_handle(ctx, &handle) )
    {
      switch (HANDLE_TYPE(ctx, handle))
	{
	case HANDLE_TYPE_FILE:
	  if (close(HANDLE_FILE(ctx, handle).fd) < 0)
	    /* FIXME: Should we do something on error ? */
	    return sftp_send_errno(ctx, errno); 

	  break;

	case HANDLE_TYPE_DIRECTORY:
	  if (closedir(HANDLE_DIR(ctx, handle).dir) < 0)
	    /* FIXME: Should we do something on error ? */
	    return sftp_send_errno(ctx, errno); 

	  free(HANDLE_DIR(ctx, handle).name);
	  
	  break;

	default:
	  abort();
	}

      HANDLE_TYPE(ctx, handle) = HANDLE_UNUSED;
      return sftp_send_status(ctx, SSH_FX_OK);
    }
  else	
    return sftp_bad_message(ctx); 

  /* FIXME: Should we separate cases bad message and illegal handle
   * and return failure for one and bad_message for the other?
   */
}

#define SFTP_MAX_SIZE 32768

static ssize_t
sftp_read(struct sftp_file *f, void *buf, size_t count, off_t offset)
{
  ssize_t done;

  if (offset != f->position)
    {
#if HAVE_PREAD
      do
	done = pread(f->fd, buf, count, offset);
      while ( (done < 0) && (errno == EINTR) );

      return done;
#else /* !HAVE_PREAD */

      if (lseek(f->fd, offset, SEEK_SET) == (off_t) -1)
	return -1;

      f->position = offset;
#endif /* !HAVE_PREAD */
    }

  do /* Plain read */
    done = read(f->fd, buf, count);
  while ( (done < 0) && (errno == EINTR) );
  
  if (done > 0)
    f->position += done;
  
  return done;
}

#define BUFFER_SIZE 4096

static int
sftp_process_read(struct sftp_ctx *ctx)
{
  struct sftp_file *f;
  off_t offset;
  uint32_t length;

  if ( !(sftp_get_handle_file(ctx, &f) && 
	 sftp_get_uint64(ctx->i, &offset) &&
	 sftp_get_uint32(ctx->i, &length)))
    return sftp_bad_message(ctx);
  
  /* FIXME: Should we separate cases bad message
     and illegal handle and return failure 
     for one and bad_message for the other?
  */
  
  /* Check so we are to read at all */
  if (length)
    {
      uint32_t done = 0;
      uint32_t index;
      int res;

      DEBUG (("sftp_process_read: fd = %d\n", f->fd));
      
      /* FIXME: Should we really sanity check the length? */
      if (length > SFTP_MAX_SIZE)
	length = SFTP_MAX_SIZE;
      
      index = sftp_put_reserve_length(ctx->o);

      while (length)
	{
	  uint8_t buf[BUFFER_SIZE];

	  res = sftp_read(f, buf, sizeof(buf), offset);

	  DEBUG (("sftp_process_read: read => %d\n", res));
	  if (res < 0)
	    {
	      sftp_put_reset(ctx->o, index);
	      return sftp_send_errno(ctx, errno);
	    }
	  else if (res > 0)
	    {
	      sftp_put_data(ctx->o, res, buf);
	      done += res;
	      offset += res;
	      length -= res;
	    }
	  else
	    break;
	}

      DEBUG (("sftp_process_read: After read loop, done = %d, left = %d\n",
	      done, length));
      if (!done)
	{
	  sftp_put_reset(ctx->o, index);
	  return sftp_send_status( ctx, SSH_FX_EOF );
	}
      else
	{
	  sftp_put_final_length(ctx->o, index);
	  sftp_set_msg( ctx->o, SSH_FXP_DATA );
	  return 1;
	}
    }
  /* An empty string */
  sftp_put_uint32(ctx->o, 0);
  sftp_set_msg( ctx->o, SSH_FXP_DATA );
  return 1;
}

static ssize_t
sftp_write(struct sftp_file *f, const void *buf, size_t count, off_t offset)
{
  ssize_t done;

  if (offset != f->position)
    {
#if HAVE_PWRITE
      do
	done = pwrite(f->fd, buf, count, offset);
      while ( (done < 0) && (errno == EINTR) );

      return done;
#else /* !HAVE_PWRITE */

      if (lseek(f->fd, offset, SEEK_SET) == (off_t) -1)
	return -1;

      f->position = offset;
#endif /* !HAVE_PWRITE */
    }

  do /* Plain write */
    done = write(f->fd, buf, count);
  while ( (done < 0) && (errno == EINTR) );
  
  if (done > 0)
    f->position += done;
  
  return done;
}

static int
sftp_process_write(struct sftp_ctx *ctx)
{
  struct sftp_file *f;
  off_t offset;
  size_t done;
  
  uint8_t *data;
  uint32_t length;
  
  if ( !(sftp_get_handle_file(ctx, &f)
	 && sftp_get_uint64(ctx->i, &offset)
	 && (data = sftp_get_string_auto(ctx->i, &length))))
    return sftp_bad_message(ctx);

  /* FIXME: Should we separate cases bad message
     and illegal handle and return failure 
     for one and bad_message for the other?
  */

  /* FIXME: 64-bit support works differently on solaris at least */

  /* Nothing happens if length is 0 - hence we need no special test */
  
  for (done = 0; length; )
    {
      int res = sftp_write(f, data + done, length, offset);

      if (res <= 0)
	return sftp_send_errno(ctx, errno);

      else if (res > 0)
	{
	  done += res;
	  offset += res;
	  length -= res;
	}
    }
  return sftp_send_status(ctx, SSH_FX_OK); 
}

static int
sftp_process_readlink(struct sftp_ctx *ctx)
{
  const uint8_t *name;
  uint8_t *linktarget;
  struct stat st;
  int path_max;
  int link_len;

  if (! (name = sftp_get_name(ctx->i)))
    return sftp_bad_message(ctx);

  /* Code below from the manpage for realpath on my debian system.
   * /Pontus */

  /* It's not quite POSIX compliant, and will not do the right thing
   * on the Hurd. /Niels */
#ifdef PATH_MAX
  path_max = PATH_MAX;
#else
  path_max = pathconf (name, _PC_PATH_MAX);
  if (path_max <= 0)
    path_max = 4096;
#endif

  linktarget = alloca(path_max);
  link_len = readlink(name, linktarget, path_max);

  if (link_len < 0)
    return sftp_send_errno(ctx, errno);

  sftp_put_uint32(ctx->o, 1); /* Count */  
  
  /* FIXME: The draft says we should return "just one name and dummy
   * attributes", but I figure we might as well pass the real attributes,
   * if we can get them. */
  
  if (lstat(linktarget, &st ) < 0)
    {
      /* Return dummy attributes. */
      sftp_put_string(ctx->o, link_len, linktarget);

      /* Leave the "long name" field empty. */
      sftp_put_uint32(ctx->o, 0);

      /* ATTRS data, containing only a zer flags field. */
      sftp_put_uint32(ctx->o, 0);
    }
  else
    sftp_put_filename(ctx, &st, linktarget);

  sftp_set_msg( ctx->o, SSH_FXP_NAME );
  return 1;
}

static int
sftp_process_symlink(struct sftp_ctx *ctx)
{
  const uint8_t *linkpath;
  const uint8_t *targetpath;

  if (! ( (linkpath = sftp_get_name(ctx->i))
	  && (targetpath = sftp_get_name(ctx->i))) )
    return sftp_bad_message(ctx);
  
  DEBUG (("sftp_process_symlink: Linkpath: %s Targetpat: %s\n", linkpath, targetpath));

  if (symlink(targetpath, linkpath) < 0)
    return sftp_send_errno(ctx, errno);
  else
    return sftp_send_status(ctx, SSH_FX_OK);   
}

static void
sftp_process(sftp_process_func **dispatch,
	     struct sftp_ctx *ctx)
{
  uint8_t msg;
  uint32_t id;
  
  int ok;

  switch (sftp_read_packet(ctx->i))
    {
    case 1:
      break;
    case 0:
      exit(EXIT_FAILURE);
    case -1: /* EOF */
      exit(EXIT_SUCCESS);
    }
  
  /* All packets start with a msg byte and a 32-bit id. */
  if (!sftp_get_uint8(ctx->i, &msg))
    fatal("Invalid packet.");

  if (!sftp_get_uint32(ctx->i, &id))
    fatal("Invalid packet.");

  DEBUG (("sftp_process: msg: %d, id: %d\n", msg, id));

  /* Every reply starts with the id, so copy it through */
  sftp_set_id(ctx->o, id);
    
  /* Calls fatal on protocol errors. */
  ok = dispatch[msg](ctx);
    
  /* Every handler should result in at least one message */
  if (!sftp_write_packet(ctx->o))
    exit(EXIT_FAILURE);
  
  if (!ok)
    exit(EXIT_FAILURE);
}  

/* The handshake packets are special, because they don't include any
 * request id. */
static int
sftp_handshake(struct sftp_ctx *ctx)
{
  uint8_t msg;
  uint32_t version;

  DEBUG (("sftp_handshake\n"));
  
  if (sftp_read_packet(ctx->i) <= 0)
    return 0;

  DEBUG (("sftp_handshake, read packet\n"));
  
  if (sftp_get_uint8(ctx->i, &msg)
      && (msg == SSH_FXP_INIT)
      && sftp_get_uint32(ctx->i, &version))
    {
      DEBUG (("sftp_handshake, got version packet\n"));
      
      while (!sftp_get_eod(ctx->i))
	if (!sftp_skip_extension(ctx->i))
	  return 0;

      if (version < SFTP_VERSION)
	return 0;

      /* The VERSION message puts the version number where
       * the id is usually located. */

      sftp_set_msg(ctx->o, SSH_FXP_VERSION);
      sftp_set_id(ctx->o, SFTP_VERSION);

      return sftp_write_packet(ctx->o);
    }
  return 0;
}

static int
parse_options(int argc, char **argv)
{
  int i;

  for (i = 1; i<argc; i++)
    {
      if (argv[i][0] != '-')
	return 0;
      switch (argv[i][1])
	{
#if WITH_DEBUG
	case 'd':
	  debug_flag = 1;
	  break;
#endif
	default:
	  return 0;
	}
    }
  return 1;
}

int
main(int argc, char **argv)
{
  unsigned i;
  
  struct sftp_ctx ctx;
  sftp_process_func *dispatch[0x100];

  if (!parse_options(argc, argv))
    return EXIT_FAILURE;
  
  sftp_init(&ctx, STDIN_FILENO, STDOUT_FILENO);

  if (!sftp_handshake(&ctx))
    return EXIT_FAILURE;
  
  for (i = 0; i<0x100; i++)
    dispatch[i] = sftp_process_unsupported;
  
  dispatch[SSH_FXP_OPEN] = sftp_process_open;
  dispatch[SSH_FXP_CLOSE] = sftp_process_close;
  dispatch[SSH_FXP_READ] = sftp_process_read;
  dispatch[SSH_FXP_WRITE] = sftp_process_write;
  dispatch[SSH_FXP_LSTAT] = sftp_process_lstat;
  dispatch[SSH_FXP_STAT] = sftp_process_stat;
  dispatch[SSH_FXP_FSTAT] = sftp_process_fstat;
  dispatch[SSH_FXP_SETSTAT] = sftp_process_setstat;
  dispatch[SSH_FXP_FSETSTAT] = sftp_process_fsetstat;  
  dispatch[SSH_FXP_MKDIR] = sftp_process_mkdir;
  dispatch[SSH_FXP_RMDIR] = sftp_process_rmdir;
  dispatch[SSH_FXP_REMOVE] = sftp_process_remove;
  dispatch[SSH_FXP_RENAME] = sftp_process_rename;
  dispatch[SSH_FXP_OPENDIR] = sftp_process_opendir;
  dispatch[SSH_FXP_READDIR] = sftp_process_readdir;
  dispatch[SSH_FXP_REALPATH] = sftp_process_realpath;
  dispatch[SSH_FXP_READLINK] = sftp_process_readlink;
  dispatch[SSH_FXP_SYMLINK] = sftp_process_symlink;

  for(;;)
    sftp_process(dispatch, &ctx);
}
