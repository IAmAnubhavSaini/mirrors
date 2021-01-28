/* lsh-execuv.c
 *
 * Helper program for securely executing a program as a different user.
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2000 Niels Möller
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

/* The problem this program solves is as follows:
 *
 * The usual way for a trusted daemon, running as root, to execute
 * user programs, is a sequence of
 *
 *   fork, setuid, exec
 *
 * However, this opens a race condition: Between the setuid and exec,
 * a user process might be able to stop the process and/or attach a
 * debugger to it, or examine data in it's address space in other
 * ways. If the address space, which at this point is a copy of the
 * daemon's, contains secret data, this is a security weakness.
 *
 * Linux seems to have a hack that disables ptrace and other ways to
 * examine the process' address space between setuid and exec, which
 * seems like a good thing to do. However, this feature seems to be
 * undocumented, and not standard, so we can't rely on it.
 *
 * Instead, we let the daemon exec this helper program while still
 * running as root. We then change uid and exec the real program. This
 * way, the address space that is potentially leaked to an
 * unauthorized user contains only this helper program, and no
 * secrets.
 */

/* TODO: Add handling of options
 *
 * -e name=value
 *
 * for modifying the environment. Is it safe to call putenv directly
 * while parsing options, or should changes the environment be delayed
 * until after setuid? */

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_CRYPT_H
# include <crypt.h>
#endif
#include <pwd.h>
#include <grp.h>

static void
usage(void)
{
  fprintf(stderr,
	  "lsh-execuv [-u uid] [-n name] [-g gid] [-i] [-c] [-p] "
	  "program [--] real-argv\n\n"
	  "Options:\n"
	  "  -u   Numeric user id\n"
	  "  -g   Numeric group id\n"
	  "  -n   User name, needed for initgroups\n"
	  "  -i   Call initgroups\n"
	  "  -c   Clear the list of supplementary groups\n"
	  "  -p   Use PATH\n"
	  "  -?   Show this help\n");
}

static void
die(const char *msg) NORETURN;

static void
die(const char *msg)
{
  fprintf(stderr, "lsh-execuv: %s\n", msg);
  exit(EXIT_FAILURE);
}

static void
die_errno(const char *msg) NORETURN;

static void
die_errno(const char *msg)
{
  fprintf(stderr, "lsh-execuv: %s (errno = %d): %s\n",
	  msg, errno, STRERROR(errno));
  exit(EXIT_FAILURE);
}

/* Convert a string to a number */
static long
atoid(const char *s, int *ok)
{
  char *end;
  long value = strtol(s, &end, 0);
  *ok = *s && !*end;

  return value;
}

#if INITGROUPS_WORKAROUND

/* Currently, we do this only for linux and gcc */

#if defined(__linux__) && defined(__GNUC__)
/* This worked fine, and was adopted into glibc, until setgroups got a
   similar limitation, so we override it as well. */
#include <linux/posix_types.h>
#include <sys/syscall.h>

#define __NR_my_setgroups __NR_setgroups
static _syscall2 (int, my_setgroups, size_t, n, __kernel_gid_t *, groups)

static int
xsetgroups (size_t n, const gid_t *groups)
{
  size_t i;
  __kernel_gid_t kernel_groups[n];

  for (i = 0; i < n; i ++)
    kernel_groups[i] = groups[i];
  
  return my_setgroups (n, kernel_groups);
}

#define setgroups xsetgroups
#endif /* linux && GNUC && i386 */
/* The GNU C Library currently has a compile-time limit on the number of
   groups a user may be a part of, even if the underlying kernel has been
   fixed, and so we define our own initgroups. */
static int
xinitgroups (char *user, gid_t gid)
{
  struct group *grp;
  gid_t *buf;
  int buflen, ngroups, res;

  /* Initialise the list with the specified GID. */
  ngroups = 0;
  buflen = 16;
  buf = malloc (buflen * sizeof (*buf));
  if (!buf)
    {
      errno = ENOMEM;
      return -1;
    }
  buf[ngroups ++] = gid;

  setgrent ();
  while ((grp = getgrent ()))
    {
      /* Scan the member list for our user. */
      char **p = grp->gr_mem;
      while (*p && strcmp (*p, user))
      p ++;

      if (*p)
      {
        /* We found the user in this group. */
        if (ngroups == buflen)
          {
	    gid_t *newbuf;
	    
            /* Enlarge the group list. */
            buflen *= 2;
            newbuf = realloc (buf, buflen * sizeof (*buf));
	    if (!newbuf)
	      {
		free(buf);
		errno = ENOMEM;
		return -1;
	      }
	    buf = newbuf;
          }

        /* Add the group id to our list. */
        buf[ngroups ++] = grp->gr_gid;
      }
    }
  endgrent ();

  /* Return whatever setgroups says. */
  res = setgroups (ngroups, buf);
  free (buf);
  return res;
}
#define initgroups xinitgroups

#endif /* INITGROUPS_WORKAROUND */
  
int main(int argc, char **argv)
{
  uid_t uid = -1;
  int uid_provided = 0;
  gid_t gid = -1;
  int gid_provided = 0;

  int grouplist_init = 0;
  int grouplist_clear = 0;

  int use_path = 0;
  
  char *name = NULL;
  
  char *program;
  
  int ok;

  struct passwd *pwd = NULL;
  
  for (;;)
    switch (getopt(argc, argv, "u:n:g:ic"))
      {
      case 'u':
	uid = atoid(optarg, &ok);
	if (!ok)
	  die("Invalid uid argument.");
	uid_provided = 1;
	break;

      case 'g':
	gid = atoid(optarg, &ok);
	if (!ok)
	  die("Invalid gid argument.");
	gid_provided = 1;
	break;

      case 'n':
	name = optarg;
	break;

      case 'i':
	grouplist_init = 1;
	break;

      case 'c':
	grouplist_clear = 1;
	break;

      case 'p':
	use_path = 1;
	break;

      case -1:
	/* Options done */
	goto options_done;
	
      default:
	usage();
	return EXIT_FAILURE;
      }
 options_done:

  argc -= optind;
  argv += optind;

  if (argc < 2)
    die("Program name and argv[0] are mandatory.");

  program = *argv++; argc--;
  
  if (grouplist_clear && grouplist_init)
    die("The -i and -c options are mutually exclusive.");
  
  if (name)
    {
      pwd = getpwnam(name);

      if (!pwd)
	die("No such user");

      if (!uid_provided)
	uid = pwd->pw_uid;

      if (!gid_provided)
	gid = pwd->pw_gid;
    }
  else
    {
      if (!uid_provided)
	die("Without username, the user id option is mandatory.");
      if (!gid_provided)
	die("Without username, the group id option is mandatory.");
      if (grouplist_init)
	die("group init (-i) requires a user name.");
    }

  if (!uid)
    die("Won't exec as root.");
  
  /* Fixup the group list */
  if (grouplist_clear)
    {
      if (setgroups(0, NULL) < 0)
	die_errno("Failed to clear supplimentary groups list");
    }
  else if (grouplist_init)
    {
      assert(name);
      if (initgroups(name, gid) < 0)
	die_errno("Failed to initialize supplimentary groups list");
    }

  if (setgid(gid) < 0)
    die_errno("setgid failed");

  /* FIXME: On obscure systems, notably UNICOS, it's not enough to
   * change our uid, we must also explicitly lower our privileges. */

  if (setuid(uid) < 0)
    die_errno("setuid failed");

  assert(uid == getuid());

  if (use_path)
    execvp(program, argv);
  else
    execv(program, argv);

  die_errno("exec failed");

  return EXIT_FAILURE;
}
