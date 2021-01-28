/* reaper.c
 *
 * Handle child processes.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 1998 Niels Möller
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

#include <assert.h>
#include <errno.h>

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "reaper.h"

#include "alist.h"
#include "werror.h"
#include "xalloc.h"

#define GABA_DEFINE
#include "reaper.h.x"
#undef GABA_DEFINE

#include "reaper.c.x"

static void
do_reap(struct reaper *c,
	pid_t pid, struct exit_callback *callback)
{
  CAST(reaper, closure, c);

  ALIST_SET(closure->children, pid, &callback->super);
}

/* GABA:
   (class
     (name reaper_callback)
     (super lsh_callback)
     (vars
       (reaper object reaper)))
*/

static void
do_reaper_callback(struct lsh_callback *s)
{
  CAST(reaper_callback, self, s);
  struct reaper *r = self->reaper;
  
  pid_t pid;
  int status;

  while( (pid = waitpid(-1, &status, WNOHANG)) )
    {
      if (pid > 0)
	{
	  int signaled;
	  int value;
	  int core;
	  struct exit_callback *callback;
	  
	  if (WIFEXITED(status))
	    {
	      verbose("Child %i died with exit code %i.\n",
		      pid, WEXITSTATUS(status));
	      signaled = 0;
	      core = 0;
	      value = WEXITSTATUS(status);
	    }
	  else if (WIFSIGNALED(status))
	    {
	      verbose("Child %i killed by signal %i.\n",
		      pid, WTERMSIG(status));
	      signaled = 1;
#ifdef WCOREDUMP
	      core = !!WCOREDUMP(status);
#else
	      core = 0;
#endif
	      value = WTERMSIG(status);
	    }
	  else
	    fatal("Child died, but neither WIFEXITED or WIFSIGNALED is true.\n");

	  {
	    CAST_SUBTYPE(exit_callback, c, ALIST_GET(r->children, pid));
	    callback = c;
	  }
	  
	  if (callback)
	    {
	      ALIST_SET(r->children, pid, NULL);
	      EXIT_CALLBACK(callback, signaled, core, value);
	    }
	  else
	    {
	      if (WIFSIGNALED(status))
		werror("Unregistered child %i killed by signal %i.\n",
		       pid, value);
	      else
		werror("Unregistered child %i died with exit status %i.\n",
		       pid, value);
	    }
	}
      else switch(errno)
	{
	case EINTR:
	  werror("reaper.c: waitpid returned EINTR.\n");
	  break;
	case ECHILD:
	  /* No more child processes */
	  return;
	default:
	  fatal("reaper.c: waitpid failed %e\n", errno);
	}
    }
}

static struct lsh_callback *
make_reaper_callback(struct reaper *reaper)
{
  NEW(reaper_callback, self);
  self->super.f = do_reaper_callback;
  self->reaper = reaper;

  return &self->super;
}

struct reaper *
make_reaper(void)
{
  NEW(reaper, self);

  self->reap = do_reap;
  self->children = make_linked_alist(0, -1);

  io_signal_handler(SIGCHLD, make_reaper_callback(self));

  return self;
}
