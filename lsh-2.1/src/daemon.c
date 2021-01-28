/* daemon.c
 *
 * Derived from
 * http://www.zip.com.au/~raf2/lib/software/daemon
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1999, 2002, raf, Niels Möller
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>

/* For setrlimit */
#include <sys/time.h>    /* Needed on BSD */
#include <sys/resource.h>

#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "daemon.h"

#include "format.h"
#include "lsh_string.h"
#include "werror.h"
#include "xalloc.h"


#ifndef PID_DIR
#define PID_DIR "/var/run"
#endif

#ifndef ROOT_DIR
#define ROOT_DIR "/"
#endif

#ifndef PID_SUFFIX
#define PID_SUFFIX ".pid"
#endif

/* Creates a pid file for a daemon. For now, only O_EXCL style
   locking. Returns 1 on success or 0 on failure. */
int
daemon_pidfile(const char *name)
{
  int fd;
  
  /* Try to open the file atomically. This provides sufficient locking
   * on normal (non-NFS) file systems. */

  fd = open(name, O_WRONLY | O_CREAT | O_EXCL, 0644);

  if (fd < 0)
    {
      if (errno != EEXIST)
	{
	  werror("Failed to open pid file '%z' %e\n",
		 name, errno);
	  return 0;
	}

      /* FIXME: We could try to detect and ignore stale pid files. */
      werror("Pid file '%z' already exists.\n", name);
      return 0;
    }
  else
    {
      struct lsh_string *pid = ssh_format("%di", getpid());
      uint32_t pid_length = lsh_string_length(pid);
      int res = write(fd, lsh_string_data(pid), pid_length);
      close(fd);
      
      if ( (res > 0) && ((unsigned) res == pid_length) )
	{
	  /* Success! */
	  lsh_string_free(pid);
	  return 1;
	}
      werror("Writing pid file '%z' failed %e\n", name, errno);

      /* Attempt unlinking file */
      if (unlink(name) < 0)
	werror("Unlinking pid file '%z' failed %e\n",
	       name, errno);
      
      lsh_string_free(pid);
	  
      return 0;
    }
}

/* Determines whether or not this process was started by init(8). If
   it was, we might be getting respawned so fork(2) and exit(2) would
   be a big mistake. */
static int
daemon_started_by_init(void)
{
  return (getppid() == 1);
}

/* Determines whether or not this process was started by inetd(8). If
   it was, stdin, stdout and stderr would be opened to a socket.
   Closing stdin and stdout. We also wouldn't need to fork(2) and
   exec(2) because there isn't a controlling terminal in sight. */

/* FIXME: Do we need to detect if the socket is listening or connected
 * to a peer? */
static int
daemon_started_by_inetd(void)
{
  int optval;
  socklen_t optlen = sizeof(optval);
  int res = getsockopt(STDIN_FILENO, SOL_SOCKET, SO_TYPE, &optval, &optlen);
  
  return res == 0;
}

enum daemon_mode
daemon_detect(void)
{
  if (daemon_started_by_init())
    return DAEMON_INIT;
  else if (daemon_started_by_inetd())
    return DAEMON_INETD;
  else
    return DAEMON_NORMAL;  
}

/* Disable core files */
int
daemon_disable_core(void)
{
  struct rlimit limit = { 0, 0 };

  /*
  ** Disable core files to prevent security holes.
  */

  if (getrlimit(RLIMIT_CORE, &limit) == -1)
    return 0;

  limit.rlim_cur = 0;

  if (setrlimit(RLIMIT_CORE, &limit) == -1)
    return 0;

  return 1;
}

int
daemon_dup_null(int fd)
{
  int null = open("/dev/null", O_RDWR);
  if (null < 0)
    {
      werror("Opening /dev/null failed: %e\n", errno);
      return 0;
    }
  if (dup2(null, fd) < 0)
    {
      werror("Failed to redirect fd %i to /dev/null: %e\n", errno);
      close(null);
      return 0;
    }

  close(null);
  return 1;
}

#if !HAVE_GETDTABLESIZE
#define MAX_CLOSE_FD 1024;
int
getdtablesize(void)
{
  long open_max = sysconf(_SC_OPEN_MAX);
  if (open_max < 0 || open_max > MAX_INT)
    {
      werror("No limit on number of openfiles. Some high fd:s might be left open.\n");
      return MAX_CLOSE_FD;
    }
  else
    return open_max;
}
#endif /* !HAVE_GETDTABLESIZE */

/* Try to close all fd:s above stderr. There shouldn't be any open
   file descriptors left over by startup scripts and the like, but if
   there are anyway, they should be closed so that they aren't
   inherited by user processes started by lshd. */
void
daemon_close_fds(void)
{
  int fd = getdtablesize();

  while (--fd > 2)
    {
      int res;

      do
	res = close(fd);
      while (res < 0 && errno == EINTR);

      if (res == 0)
	werror("Closed spurious fd %i\n", fd);
      else if (errno != EBADF)
	werror("Closing spurious fd %i failed: %e\n", fd, errno);
    }
}

/* Initialises a daemon:

   If the process wasn't started by init(8) or inetd(8):

     Backgrounds the process to lose process group leadership.

     Becomes a process session leader.

     Backgrounds the process again to lose process group leadership.
     This prevents the process from gaining a controlling terminal.
     Under BSD, you must still include O_NOCTTY when opening terminals
     to prevent the process from gaining a controlling terminal.

   Changes directory to the root directory so as not to hamper umounts.

   Clears the umask to enable explicit file modes.
*/

int
daemon_init(enum daemon_mode mode)
{
  /* Don't setup a daemon-friendly process context if started by
     init(8) or inetd(8). */

  if (mode == DAEMON_NORMAL)
    {
      pid_t child;
      
      /*
      ** Background the process.
      ** Lose process group leadership.
      */

      switch (fork())
	{
	case -1:
	  return 0;
	case 0:
	  /* Child */
	  break;
	default:
	  /* Parent */
	  _exit(0);
	}
      /* Become a process session leader. */

      if (setsid() < 0)
	{
	  werror("daemon_init: setsid failed.\n");
	  return 0;
	}

      /* Lose process session leadership to prevent gaining a
	 controlling terminal in SVR4.
      */
      switch (child = fork())
	{
	case -1:
	  return 0;
	case 0:
	  break;
	default:
	  _exit(0);
	}
    }

  /* Enter the root directory to prevent hampering umounts. */

  if (chdir(ROOT_DIR) == -1)
    return 0;

  /* Clear umask to enable explicit file modes. */
  umask(0);

  return 1;
}

/* Unlinks the daemon's (locked) process id file. */
int
daemon_close(const char *name)
{
  if (unlink(name) < 0)
    {
      werror("daemon_close: Unlink of pid file '%z' failed %e\n",
	     name, errno);
      return 0;
    }
  return 1;
}
