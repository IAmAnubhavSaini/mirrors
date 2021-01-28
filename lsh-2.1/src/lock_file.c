/* lock_file.c
 *
 * Traditional O_EXCL-style file locking.
 *
 * $id:$
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2001 Niels Möller
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
#include "config.h"
#endif

#include <errno.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>

#include "lock_file.h"

#include "format.h"
#include "lsh_string.h"
#include "resource.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "lock_file.h.x"
#undef GABA_DEFINE

#include "lock_file.c.x"


/* GABA:
   (class
     (name lsh_file_lock)
     (super resource)
     (vars
       (info object lsh_file_lock_info)))
*/

static void
do_kill_file_lock(struct resource *s)
{
  CAST(lsh_file_lock, self, s);

  if (self->super.alive)
    {
      self->super.alive = 0;
      unlink (lsh_get_cstring(self->info->lockname));
    }
}

/* FIXME: Probably doesn't work right with NFS */
static struct resource *
do_lsh_file_lock(struct lsh_file_lock_info *self, unsigned retries)
{
  int fd = -1;
  unsigned i;
  
  fd = open(lsh_get_cstring(self->lockname),
	    O_CREAT | O_EXCL | O_WRONLY,
	    0666);
  
  for (i = 0; (fd < 0) && (errno == EEXIST) && (i<retries); i++)
    {
      /* Wait a few seconds and try again. */
      sleep(17);
      
      fd = open(lsh_get_cstring(self->lockname),
		O_CREAT | O_EXCL | O_WRONLY,
		0666);
    }
  
  if (fd < 0)
    {
      if (errno != EEXIST)
	werror("Could not create lock file `%S' %e\n",
	       self->lockname, errno);
      return NULL;
    }
  else
    {
      NEW(lsh_file_lock, lock);

      trace("lock_file.c: Creation of lock file `%S' succeeded.\n",
	    self->lockname);
      init_resource(&lock->super, do_kill_file_lock);

      lock->info = self;
      
      close(fd);
      return &lock->super;
    }
}

/* Checks if a file is locked, without actually trying to lock it. */
static int
do_lsh_file_lock_p(struct lsh_file_lock_info *self)
{
  struct stat sbuf;

  return (stat(lsh_get_cstring(self->lockname), &sbuf) == 0);
}

struct lsh_file_lock_info *
make_lsh_file_lock_info(struct lsh_string *name)
{
  NEW(lsh_file_lock_info, self);
  self->lockname = name;
  self->lock = do_lsh_file_lock;
  self->lock_p = do_lsh_file_lock_p;

  return self;
}
