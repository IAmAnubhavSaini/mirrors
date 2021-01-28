/* server_pty.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998, 1999, Niels Möller, Balázs Scheidler
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <errno.h>
/* FIXME: for snprintf, maybe use a custom snprintf? Bazsi */
#include <stdio.h>  

#include <fcntl.h>
#include <grp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#if HAVE_PTY_H
# include <pty.h>  /* openpty() */
#endif

#if HAVE_STROPTS_H
# include <stropts.h>  /* isastream() */
#endif

#include "server_pty.h"

#include "channel.h"
#include "connection.h"
#include "format.h"
#include "io.h"
#include "lsh_string.h"
#include "parse.h"
#include "ssh.h"
#include "werror.h"
#include "xalloc.h"


#define GABA_DEFINE
#include "server_pty.h.x"
#undef GABA_DEFINE

static void
do_kill_pty_info(struct resource *r)
{
  CAST(pty_info, pty, r);

  if (pty->super.alive)
    {
      pty->super.alive = 0;
      if ( (pty->master >= 0) && (close(pty->master) < 0) )
	werror("do_kill_pty_info: closing master failed %e\n", errno);
    }
}

struct pty_info *
make_pty_info(void)
{
  NEW(pty_info, pty);

  init_resource(&pty->super, do_kill_pty_info);
  pty->tty_name = NULL;
  pty->mode = NULL;
  pty->master = -1;
  return pty;
}

/* FIXME: Maybe this name should be configurable? */
#ifndef TTY_GROUP
#define TTY_GROUP "tty"
#endif
#ifndef SYSTEM_GROUP
#define SYSTEM_GROUP "system"
#endif

#ifndef ACCESSPERMS
#define ACCESSPERMS 07777
#endif

/* Sets the permissions on the slave pty suitably for use by USER.
 * This function is derived from the grantpt function in
 * sysdeps/unix/grantpt.c in glibc-2.1. */

static int
pty_check_permissions(const char *name, uid_t user)
{
  struct stat st;
  struct group *grp;
  gid_t tty_gid;
      
  if (stat(name, &st) < 0)
    return 0;

  /* Make sure that the user owns the device. */
  if ( (st.st_uid != user)
       && (chown(name, user, st.st_gid) < 0) )
    return 0;

  /* Points to static area */
  grp = getgrnam(TTY_GROUP);

  if (!grp)
    /* On AIX, tty:s have group "system", not "tty" */
    grp = getgrnam(SYSTEM_GROUP);
    
  if (grp)
    tty_gid = grp->gr_gid;
  else
    {
      /* If no tty group is found, use the server's gid */
      werror("lshd: server_pty.c: No tty group found.\n");
      tty_gid = getgid();
    }

  if ( (st.st_gid != tty_gid)
       && (chown(name, user, tty_gid) < 0))
    return 0;

  /* Make sure the permission mode is set to readable and writable
   * by the owner, and writable by the group. */

  if ( ((st.st_mode & ACCESSPERMS) != (S_IRUSR | S_IWUSR | S_IWGRP))
       && (chmod(name, S_IRUSR | S_IWUSR | S_IWGRP) < 0) )
    return 0;

  /* Everything is fine */
  return 1;
}

#if HAVE_UNIX98_PTYS

/* Returns the name of the slave tty, as a string with an extra
 * terminating NUL. */

static struct lsh_string *
pty_grantpt_uid(int master, uid_t user)
{
  uid_t me = getuid();
  if (me == user)
    {
      /* Use standard grantpt call */
      if (grantpt(master) < 0)
	return NULL;

      return make_string(ptsname(master));
    }
  else
    { /* Set up permissions for user */

      /* Pointer to static area */
      char *name = ptsname(master);
      return (pty_check_permissions(name, user)
	      ? make_string(name)
	      : NULL);
    }
}
#endif /* HAVE_UNIX98_PTYS */

int
pty_open_master(struct pty_info *pty,
		uid_t user
#if !HAVE_UNIX98_PTYS
		UNUSED
#endif
	     )
{
#if HAVE_UNIX98_PTYS
  struct lsh_string *name = NULL;
  if ((pty->master = open("/dev/ptmx", O_RDWR | O_NOCTTY)) < 0)
    {
      werror("pty_open_master: Opening /dev/ptmx failed %e\n", errno);
      return 0;
    }

  io_set_close_on_exec(pty->master);

  if ((name = pty_grantpt_uid(pty->master, user))
      && (unlockpt(pty->master) == 0))
    {
      pty->tty_name = name;
      return 1;
    }

  close (pty->master); pty->master = -1;
  
  if (name)
    lsh_string_free(name);
  return 0;
  
#elif PTY_BSD_SCHEME

#define PTY_BSD_SCHEME_MASTER "/dev/pty%c%c"
#define PTY_BSD_SCHEME_SLAVE  "/dev/tty%c%c"
  char first[] = PTY_BSD_SCHEME_FIRST_CHARS;
  char second[] = PTY_BSD_SCHEME_SECOND_CHARS;
  char master[MAX_TTY_NAME];
  char slave[MAX_TTY_NAME];
  unsigned int i, j;

  for (i = 0; first[i]; i++)
    {
      for (j = 0; second[j]; j++) 
        {
	  snprintf(master, sizeof(master),
		   PTY_BSD_SCHEME_MASTER, first[i], second[j]);
	  master[sizeof(master) - 1] = 0;

	  pty->master = open(master, O_RDWR | O_NOCTTY);
	  if (pty->master != -1) 
	    {
	      /* master succesfully opened */

	      io_set_close_on_exec(pty->master);

	      snprintf(slave, sizeof(slave),
		       PTY_BSD_SCHEME_SLAVE, first[i], second[j]);
	      slave[sizeof(slave) - 1] = 0;

	      /* NOTE: As there is no locking, setting the permissions
	       * properly does not guarantee that nobody else has the
	       * pty open, and can snoop the traffic on it. But it
	       * should be a little better than nothing. */

	      /* FIXME: Should we do something about the master
	       * permissions as well? */
	      if (pty_check_permissions(slave, user))
		{
		  pty->tty_name = make_string(slave);
		  return 1;
		}
	      close(pty->master); pty->master = -1;
	      return 0;
	    }
        }
    }
  return 0;
  /* FIXME: Figure out if we can use openpty. Probably not, as we're
   * not running with the right uid. */
#else /* PTY_BSD_SCHEME */
  /* No pty:s */
  return 0;
#endif
}

/* Opens the slave side of the tty, intitializes it, and makes it our
 * controlling terminal. Should be called by the child process.
 *
 * Also makes the current process a session leader.
 *
 * Returns an fd, or -1 on error. */
int
pty_open_slave(struct pty_info *pty)
{
  struct termios ios;
  int fd;
  
  trace("pty_open_slave\n");

  /* Open the slave. On Sys V, that also makes it our controlling tty. */
  fd = open(lsh_get_cstring(pty->tty_name), O_RDWR);

  if (fd < 0)
    {
      werror("pty_open_slave: open(\"%S\") failed,\n"
	     "   %e\n",
	     pty->tty_name, errno);
      return -1;
    }

  io_set_close_on_exec(fd);

  /* For Sys V and Solaris, push some streams modules.
   * This seems to also have the side effect of making the
   * tty become our controlling terminal. */
# ifdef HAVE_STROPTS_H
  if (isastream(fd))
    {
      if (ioctl(fd, I_PUSH, "ptem") < 0)
	{
	  werror("pty_open_slave: Failed to push streams module `ptem'.\n"
		 "   %e\n", errno);
	
	  close(fd);
	  return -1;
	}
      if (ioctl(fd, I_PUSH, "ldterm") < 0)
	{
	  werror("pty_open_slave: Failed to push streams module `ldterm'.\n"
		 "   %e\n", errno);
	
	  close(fd);
	  return -1;
	}
    }
# endif /* HAVE_STROPTS_H */

  /* On BSD systems, use TIOCSCTTY. */

#ifdef TIOCSCTTY
  if (ioctl(fd, TIOCSCTTY, NULL) < 0)
    {
      werror("pty_open_slave: Failed to set the controlling tty.\n"
	     "   %e\n", errno);
      close(fd);
      return -1;
    }
#endif /* defined(TIOCSCTTY) */

  /* Set terminal modes */
  if (!tty_getattr(fd, &ios))
    {
      werror("pty_open_slave: Failed to get tty attributes.\n"
	     "   %e\n", errno);
      close(fd);
      return -1;
    }

  if (!tty_decode_term_mode(&ios, STRING_LD(pty->mode)))
    {
      werror("pty_open_slave: Invalid terminal modes from client.\n");
      close(fd);
      return -1;
    }

  if (!tty_setattr(fd, &ios))
    {
      werror("pty_open_slave: Failed to set tty attributes.\n"
	     "   %e\n", errno);
      close(fd);
      return -1;
    }
	  
  if (!tty_setwinsize(fd, &pty->dims))
    {
      werror("pty_open_slave: Failed to set tty window size.\n"
	     "   %e\n", errno);
      close(fd);
      return -1;
    }

  return fd;
}
