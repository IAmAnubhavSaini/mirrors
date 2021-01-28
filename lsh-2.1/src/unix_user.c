/* unix_user.c
 *
 * User-related functions on UN*X
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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <signal.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#if HAVE_CRYPT_H
# include <crypt.h>
#endif
#include <pwd.h>
#include <grp.h>

#if HAVE_SHADOW_H
#include <shadow.h>
#endif

#if WITH_UTMP
# if HAVE_UTMP_H
#  include <utmp.h>
# endif

# if HAVE_UTMPX_H
#  include <utmpx.h>
# endif
#else /* !WITH_UTMP */
  struct utmp;
#endif

#if HAVE_LIBUTIL_H
# include <libutil.h>
#endif

#include "server_userauth.h"

#include "environ.h"
#include "format.h"
#include "io.h"
#include "lsh_string.h"
#include "reaper.h"
#include "server_pty.h"
#include "werror.h"
#include "xalloc.h"


/* Forward declaration */
struct unix_user_db;

#include "unix_user.c.x"

/* GABA:
   (class
     (name unix_user)
     (super lsh_user)
     (vars
       (gid . gid_t)

       ; Context needed for some methods.
       (ctx object unix_user_db)
       
       ; These strings include a terminating NUL-character, for 
       ; compatibility with library and system calls. This applies also
       ; to the inherited name attribute.

       (passwd string)  ; Crypted passwd
       (home string)
       (shell string))) */

/* GABA:
   (class
     (name pwhelper_callback)
     (super exit_callback)
     (vars
       (user object unix_user)
       (c object command_continuation)
       (e object exception_handler)))
*/

static void
do_pwhelper_callback(struct exit_callback *s,
		     int signaled, int core UNUSED, int value)
{
  CAST(pwhelper_callback, self, s);

  if (signaled || value)
    {
      static const struct exception invalid_password
	= STATIC_EXCEPTION(EXC_USERAUTH, "Invalid password according to helper program.");
      EXCEPTION_RAISE(self->e, &invalid_password);
    }
  else
    COMMAND_RETURN(self->c, self->user);
}

static struct exit_callback *
make_pwhelper_callback(struct unix_user *user,
		       struct command_continuation *c,
		       struct exception_handler *e)
{
  NEW(pwhelper_callback, self);
  self->super.exit = do_pwhelper_callback;
  self->user = user;
  self->c = c;
  self->e = e;

  return &self->super;
}

/* NOTE: Consumes the pw string if successful.
   FIXME: Should be renamed. */
static int
kerberos_check_pw(struct unix_user *user, struct lsh_string *pw,
		  struct command_continuation *c,
		  struct exception_handler *e)
{
  /* Because kerberos is big and complex, we fork a separate process
   * to do the work. */

  int in[2];
  pid_t child;

  /* First look if the helper program seems to exist. */
  if (access(user->ctx->pw_helper, X_OK) < 0)
    {
      /* No help available */
      werror("Password helper program '%z' not available %e\n",
	     user->ctx->pw_helper, errno);
      return 0;
    }
  
  if (!lsh_make_pipe(in))
    {
      werror("kerberos_check_pw: Failed to create pipe.\n");
      return 0;
    }
  
  child = fork();
  
  switch (child)
    {
    case -1:
      werror("kerberos_check_pw: fork failed %e\n", errno);
      return 0;

    case 0:
      {  /* Child */
	int null_fd;
      
	null_fd = open("/dev/null", O_RDWR);
	if (null_fd < 0)
	  {
	    werror("kerberos_check_pw: Failed to open /dev/null.\n");
	    _exit(EXIT_FAILURE);
	  }
	if (dup2(in[0], STDIN_FILENO) < 0)
	  {
	    werror("kerberos_check_pw: Can't dup stdin!\n");
	    _exit(EXIT_FAILURE);
	  }

	if (dup2(null_fd, STDOUT_FILENO) < 0)
	  {
	    werror("kerberos_check_pw: Can't dup stdout!\n");
	    _exit(EXIT_FAILURE);
	  }

	if (dup2(null_fd, STDERR_FILENO) < 0)
	  {
	    _exit(EXIT_FAILURE);
	  }
      
	close(in[1]);
	close(null_fd);

	execl(user->ctx->pw_helper, user->ctx->pw_helper,
	      lsh_get_cstring(user->super.name), NULL);
	_exit(EXIT_FAILURE);
      }
    default:
      {
	/* Parent */
	struct lsh_fd *fd;

	close(in[0]);

	fd = io_write(make_lsh_fd(in[1], IO_NORMAL, "password helper stdin",
				  e),
		      lsh_string_length(pw), NULL);

	A_WRITE(&fd->write_buffer->super, pw);

	REAP(user->ctx->reaper, child, make_pwhelper_callback(user, c, e));

	return 1;
      }
    }
}

/* FIXME: This could be generalized to support some kind of list of
 * password databases. The current code first checks for unix
 * passwords, and if that fails, it optionally invokes a helper
 * program to verify the password, typically used for kerberos. */
static void
do_verify_password(struct lsh_user *s,
		   struct lsh_string *password,
		   struct command_continuation *c,
		   struct exception_handler *e)
{
  CAST(unix_user, user, s);
  const struct exception *x = NULL;
  const uint8_t *cpasswd;
  
  /* No supported password verification methods allows passwords
   * containing NUL, so check that here. */

  cpasswd = lsh_get_cstring(password);
  if (!cpasswd)
    {
      static const struct exception invalid_passwd
	= STATIC_EXCEPTION(EXC_USERAUTH, "NUL character in password.");

      EXCEPTION_RAISE(e, &invalid_passwd);
      return;
    }
  
  /* NOTE: Check for accounts with empty passwords, or generally short
   * passwd fields like "NP" or "x". */
  if (!user->passwd || (lsh_string_length(user->passwd) < 5) )
    {
      static const struct exception no_passwd
	= STATIC_EXCEPTION(EXC_USERAUTH, "No password in passwd db.");

      x = &no_passwd;

      /* NOTE: We attempt using kerberos passwords even if the
       * passwd entry is totally bogus. */
      goto try_helper;
    }

  /* Try password authentication against the ordinary unix database. */
  {
    const char *salt = lsh_string_data(user->passwd);

    /* NOTE: crypt uses the *ugly* convention of returning pointers
     * to static buffers. */

    if (strcmp(crypt(cpasswd, salt),
	       lsh_get_cstring(user->passwd)))
      {
	/* Passwd doesn't match */
	static const struct exception invalid_passwd
	  = STATIC_EXCEPTION(EXC_USERAUTH, "Incorrect password.");
	
	x = &invalid_passwd;

	goto try_helper;
      }
    /* Unix style password authentication succeded. */
    lsh_string_free(password);
    COMMAND_RETURN(c, user);
    return;
  }
  
 try_helper:

  /* We get here if checks against the ordinary passwd database failed. */
  assert(x);
  
  if (user->ctx->pw_helper && kerberos_check_pw(user, password, c, e))
    /* The helper program takes responsibility for password
     * verification, and it also consumed the password string so that
     * we don't need to free it here. */
    ;
  else
    {
      lsh_string_free(password);
      EXCEPTION_RAISE(e, x);
    }
}

/* NOTE: No arbitrary file names are passed to this function, so we don't have
 * to check for things like "../../some/secret/file" */
static int
do_file_exists(struct lsh_user *u,
	       struct lsh_string *name,
	       int free)
{
  CAST(unix_user, user, u);
  struct lsh_string *path;
  struct stat st;
  const char *s;
  
  if (!user->home)
    {
      if (free)
	lsh_string_free(name);
      return 0;
    }
  
  path = ssh_format(free ? "%lS/%lfS" : "%lS/%lS",
		    user->home, name);
  s = lsh_get_cstring(path);
  
  if (s && (stat(s, &st) == 0))
    {
      lsh_string_free(path);
      return 1;
    }
  lsh_string_free(path);
  return 0;
}

static const struct exception *
check_user_permissions(struct stat *sbuf, const char *fname,
		       uid_t uid, int secret)
{
  mode_t bad = secret ? (S_IRWXG | S_IRWXO) : (S_IWGRP | S_IWOTH);

  if (!S_ISREG(sbuf->st_mode))
    {
      werror("io.c: %z is not a regular file.\n",
	     fname);
      return make_io_exception(EXC_IO_OPEN_READ, NULL, 0, "Not a regular file");
    }
  if (sbuf->st_uid != uid)
    {
      werror("io.c: %z not owned by the right user (%i)\n",
	     fname, uid);
      return make_io_exception(EXC_IO_OPEN_READ, NULL, 0, "Bad owner");
    }

  if (sbuf->st_mode & bad)
    {
      werror("io.c: Permissions on %z too loose.\n",
	     fname);
      return make_io_exception(EXC_IO_OPEN_READ, NULL, 0, "Bad permissions");
    }

  return NULL;
}

/* GABA:
   (class
     (name exc_read_user_file_handler)
     (super exception_handler)
     (vars
       (c object abstract_write))))
*/

static void
do_exc_read_user_file_handler(struct exception_handler *s,
			       const struct exception *e)
{
  CAST(exc_read_user_file_handler, self, s);

  verbose("reading user file failed: %z\n", e->msg);
  
  switch (e->type)
    {
    case EXC_IO_READ:
      A_WRITE(self->c, NULL);
      break;
    default:
      werror("reading user file failed: %z\n", e->msg);
      EXCEPTION_RAISE(self->super.parent, e);
    }
}

static struct exception_handler *
make_exc_read_user_file_handler(struct abstract_write *c,
				struct exception_handler *parent,
				const char *context)
{
  NEW(exc_read_user_file_handler, self);

  self->super.parent = parent;
  self->super.raise = do_exc_read_user_file_handler;
  self->super.context = context;
  
  self->c = c;

  return &self->super;
}

#define USER_FILE_BUFFER_SIZE 1000

/* NOTE: No arbitrary file names are passed to this method, so we
 * don't have to check for things like "../../some/secret/file",
 * or for filenames containing NUL. */

static const struct exception *
do_read_file(struct lsh_user *u, 
	     const char *name, int secret,
	     uint32_t limit,
	     struct abstract_write *c)
{
  CAST(unix_user, user, u);
  struct lsh_string *f;
  struct stat sbuf;
  const struct exception *x;

  pid_t child;
  /* out[0] for reading, out[1] for writing */
  int out[2];

  uid_t me = geteuid();

  /* There's no point trying to read other user's files unless we're
   * root. */

  if (me && (me != user->super.uid) )
    return make_io_exception(EXC_IO_OPEN_READ, NULL, 0, "Access denied.");
  
  if (!user->home)
    return make_io_exception(EXC_IO_OPEN_READ, NULL,
			     ENOENT, "No home directory");

  f = ssh_format("%lS/.lsh/%lz", user->home, name);
  
  if (stat(lsh_get_cstring(f), &sbuf) < 0)
    {
      if (errno != ENOENT)
	werror("io_read_user_file: Failed to stat %S %e\n", f, errno);
 
      lsh_string_free(f);

      return make_io_exception(EXC_IO_OPEN_READ, NULL, errno, NULL);
    }

  /* Perform a preliminary permissions check before forking, as errors
   * detected by the child process are not reported as accurately. */

  x = check_user_permissions(&sbuf, lsh_get_cstring(f), user->super.uid, secret);
  if (x)
    {
      lsh_string_free(f);
      return x;
    }
  
  if (!lsh_make_pipe(out))
    {
      lsh_string_free(f);
      return make_io_exception(EXC_IO_OPEN_READ, NULL, errno, NULL);
    }
  
  child = fork();

  switch (child)
    {
    case -1:
      /* Error */

      close(out[0]); close(out[1]);
      lsh_string_free(f);
      return make_io_exception(EXC_IO_OPEN_READ, NULL, errno, NULL);
      
    default:
      /* Parent */
      close(out[1]);

      lsh_string_free(f);

      /* NOTE: We could install an exit handler for the child process,
       * but there's nothing useful for that to do. */
      io_read(make_lsh_fd
	        (out[0], IO_NORMAL, "stdout, reading a user file",
		 make_exc_read_user_file_handler(c,
						 &default_exception_handler,
						 HANDLER_CONTEXT)),
	      make_buffered_read(USER_FILE_BUFFER_SIZE,
				 make_read_file(c, limit)),
	      NULL);

      return NULL;

    case 0:
      /* Child */
      {
	int fd;
	close(out[0]);

	if (me != user->super.uid)
	  {
	    /* We need to change our persona. We can't change out real
	     * uid, though, as that might let user processes send
	     * signals to us. */
	    if (setgid(user->gid) < 0)
	      {
		werror("unix_user.c: do_read_file: setgid failed %e\n", errno);
		_exit(EXIT_FAILURE);
	      }

	    /* For simplicity, ignore the user's supplimentary groups.
	     * They shouldn't be needed to get access to files under
	     * ~/.lsh. */
	    if (setgroups(0, NULL) < 0)
	      {
		werror("unix_user.c: do_read_file: setgroups failed %e\n", errno);
		_exit(EXIT_FAILURE);
	      }
	      
	    if (seteuid(user->super.uid) < 0)
	      {
		werror("unix_user.c: do_read_file: seteuid failed %e\n", errno);
		_exit(EXIT_FAILURE);
	      }
	  }
	assert(user->super.uid == geteuid());
	
	fd = open(lsh_get_cstring(f), O_RDONLY);

	/* Check permissions again, in case the file or some symlinks
	 * changed under our feet. */

	if (fstat(fd, &sbuf) < 0)
	  {
	    werror("unix_user.c: do_read_file: fstat failed %e\n", errno);
	    _exit(EXIT_FAILURE);
	  }

	x = check_user_permissions(&sbuf, lsh_get_cstring(f),
				   user->super.uid, secret);

	if (x)
	  {
	    werror("unix_user.c: do_read_file: %z\n", x->msg);
	    _exit(EXIT_FAILURE);
	  }

	if (lsh_copy_file(fd, out[1]))
	  _exit(EXIT_SUCCESS);
	else
	  _exit(EXIT_FAILURE);
      }
    }
}

#define USE_LOGIN_DASH_CONVENTION 1

static const char *
format_env_pair(const char *name, struct lsh_string *value)
{
  assert(lsh_get_cstring(value));
  return lsh_get_cstring(ssh_format("%lz=%lS", name, value));
}

static const char *
format_env_pair_c(const char *name, const char *value)
{
  return lsh_get_cstring(ssh_format("%lz=%lz", name, value));
}

static void
exec_shell(struct spawn_info *info,
	   const char *shell,
	   const char *home,
	   const char *name,
	   /* gid is used only if uid != getuid() */
	   uid_t uid, gid_t gid)
{
  const char **envp;
  const char **argv;
  const char *program;
  const char *argv0;

  char *tz = getenv(ENV_TZ);
  unsigned i, j;

  trace("unix_user: exec_shell\n");
  assert(shell);

  /* Make up an initial environment */
  debug("exec_shell: Setting up environment.\n");
  
  /* We need place for the caller's values, 
   *
   * SHELL, HOME, USER, LOGNAME, TZ, PATH
   *
   * and a terminating NULL */

#define MAX_ENV 6

  envp = alloca(sizeof(char *) * (info->env_length + MAX_ENV + 1));

  i = 0;
  envp[i++] = format_env_pair_c(ENV_SHELL, shell);

  if (home)
    envp[i++] = format_env_pair_c(ENV_HOME, home);

  /* FIXME: The value of $PATH should not be hard-coded */
  envp[i++] = ENV_PATH "=/bin:/usr/bin";
  envp[i++] = format_env_pair_c(ENV_USER, name);
  envp[i++] = format_env_pair_c(ENV_LOGNAME, name);

  if (tz)
    envp[i++] = format_env_pair_c(ENV_TZ, tz);

  assert(i <= MAX_ENV);
#undef MAX_ENV

  for (j = 0; j < info->env_length; j++)
    envp[i++] = format_env_pair(info->env[j].name, info->env[j].value);

  envp[i] = NULL;

  debug("exec_shell: Environment:\n");
  for (i=0; envp[i]; i++)
    debug("    '%z'\n", envp[i]);

#if USE_LOGIN_DASH_CONVENTION
  if (info->login)
    {
      /* Fixup argv[0], so that it starts with a dash */
      const char *p;
      char *s;
      size_t length = strlen(shell);

      /* We can't alloca unlimited storage */
      if (length > 1000)
	{
	  werror("exec_shell: shell name far too long.\n");
	  _exit(EXIT_FAILURE);
	}
      
      debug("do_exec_shell: fixing up name of shell...\n");
      
      s = alloca(length + 2);

      /* Make sure that the shell's name begins with a -. */
      p = strrchr (shell, '/');
      if (!p)
	p = shell;
      else
	p ++;

      s[0] = '-';
      strncpy (s + 1, p, length+1);
      argv0 = s;
    }
  else
#endif /* USE_LOGIN_DASH_CONVENTION */
    argv0 = shell;

  debug("exec_shell: argv0 = '%z'.\n", argv0);

  i = 0;

  /* Use lsh-execuv only if we need to change our uid. */
  if (uid != getuid())
    {
      /* Build argument list for lsh-execuv. We need place for
       *
       * lsh-execuv -u uid -g gid -n name -i -- $SHELL argv0 <user args> NULL
       */
#define MAX_ARG 10
#define NUMBER(x) lsh_get_cstring(ssh_format("%di", (x)))

      program = PATH_EXECUV;
      
      argv = alloca(sizeof(char *) * (MAX_ARG + info->argc + 2));
      argv[i++] = "lsh-execuv";
      argv[i++] = "-u";
      trace("exec_shell: After -u\n");
      argv[i++] = NUMBER(uid);
      argv[i++] = "-g";
      trace("exec_shell: After -g\n");
      argv[i++] = NUMBER(gid);
      argv[i++] = "-n";
      argv[i++] = name;
      argv[i++] = "-i";
      trace("exec_shell: After -i\n");
      argv[i++] = "--";
      argv[i++] = shell;
      assert(i <= MAX_ARG);
#undef MAX_ARG
#undef NUMBER
    }
  else
    {
      program = shell;
      argv = alloca(sizeof(char *) * (info->argc + 2));
    }
  argv[i++] = argv0;
  
  for (j = 0; j<info->argc; j++)
    argv[i++] = info->argv[j];
  argv[i++] = NULL;
  
  debug("exec_shell: Argument list:\n");
  for (i=0; argv[i]; i++)
    debug("    '%z'\n", argv[i]);

  /* NOTE: The execve prototype uses char * const argv, and similarly
   * for envp, which seems broken. */
  
  trace("exec_shell: before exec\n");

  execve(program, (char **) argv, (char **) envp);

  werror("unix_user: exec of `%z' failed %e\n", program, errno);
  _exit(EXIT_FAILURE);
}

static void
safe_close(int fd)
{
  if (fd != -1 && close(fd) < 0)
    werror("close failed %e\n", errno);
}

/* FIXME: We need some way to report errors when the child process
 * tries to start the child.
 *
 * One simple approach is to reuse the sync pipe for this. Have
 * do_spawn read a limited amount (say 200 bytes or so) from the pipe.
 * If it gets exactly zero, then everything is ok. If it get's some
 * data, that should be returned to the caller in some way.
 *
 * In the child process, exec_shell can report errors on that fd. If
 * it exec's the shell directly, it should first set the close-on-exec
 * flag. If it uses the lsh-execuv mechanism, it can keep the
 * close-on-exec-flag cleared, and pass a new command line option
 * --error-fd to lsh-execuv. lsh-execuv can write error messages
 * there, and then it has the responsibility of setting the close-on-exec flag.
 *
 * A potential problem with this approach is that the lshd server will
 * hang until lsh-execuv is finished. One can either step back and let
 * lsh-execuv report errors on the ordinary stderr (simplest), or use
 * non-blocking mode for reading the syncronization pipe (probably too
 * complex). Some non-blocking behaviour might be needed anyway,
 * though, because of the xauth bug that results in channel data being
 * sent before the reply to a shell request.
 */

/* Helper functions for the process spawning. */

static void
spawn_error(struct spawn_info *info,
	    const int *sync)
{
  safe_close(sync[0]); safe_close(sync[1]);
  
  safe_close(info->in[0]);  safe_close(info->in[1]);
  safe_close(info->out[0]); safe_close(info->out[1]);
  safe_close(info->err[0]);
  /* Allow the client's stdout and stderr to be the same fd, e.g.
   * both /dev/null. */
  if (info->err[1] != info->out[1])
    safe_close(info->err[1]);
}

/* Parent processing */
static struct lsh_process *
spawn_parent(struct lsh_user *user,
	     struct spawn_info *info,
	     struct exit_callback *c,
	     struct reaper *reaper, pid_t child,
	     const int *sync)
{
  /* Parent */
  struct lsh_process *process;
  char dummy;
  int res;
      
  /* Close the child's fd:s, except ones that are -1 */
  safe_close(info->in[0]);
  safe_close(info->out[1]);

  /* Allow the client's stdout and stderr to be the same fd, e.g.
   * both /dev/null. */
  if (info->err[1] != info->out[1])
    safe_close(info->err[1]);

  safe_close(sync[1]);

  /* On Solaris, reading the master side of the pty before the
   * child has opened the slave side of it results in EINVAL. We
   * can't have that, so we'll wait until the child has opened the
   * tty, after which it should close its end of the
   * syncronization pipe, and our read will return 0.
   *
   * We need the syncronization only if we're actually using a
   * pty, but for simplicity, we do it every time. */
      
  do
    res = read(sync[0], &dummy, 1);
  while (res < 0 && errno == EINTR);

  safe_close(sync[0]);

  trace("do_spawn: parent after sync\n");
      
  process = unix_process_setup(child, user, &c,
			       info->peer,
			       info->pty ? info->pty->tty_name : NULL);

  trace("do_spawn: parent after process setup\n");

  REAP(reaper, child, c);
  return process;
}

static void
spawn_child(struct spawn_info *info,
	    const char *home,
	    const int *sync)
{
  int tty = -1;

  if (!home)
    goto cd_root;

  if (chdir(home) < 0)
    {
      werror("chdir to home directory `%z' failed %e\n", home, errno);

    cd_root:
      if (chdir("/") < 0)
	{
	  werror("chdir to `/' failed %e\n", errno);
	  _exit(EXIT_FAILURE);
	}
    }

  trace("spawn_child: child after chdir\n");

  /* We want to be a process group leader */
  if (setsid() < 0)
    {
      werror("unix_user: setsid failed, already process group leader?\n"
	     "   %e\n", errno);
      _exit(EXIT_FAILURE);
    }
      
#if WITH_PTY_SUPPORT
  if (info->pty)
    {
      debug("lshd: unix_user.c: Opening slave tty...\n");
      if ( (tty = pty_open_slave(info->pty)) < 0)
	{
	  debug("lshd: unix_user.c: "
		"Opening slave tty... Failed!\n");
	  werror("lshd: Can't open controlling tty for child!\n");
	  _exit(EXIT_FAILURE);
	}
      else
	debug("lshd: unix_user.c: Opening slave tty... Ok.\n");
    }
#endif /* WITH_PTY_SUPPORT */

  trace("spawn_child: after pty\n");
      
  /* Now any tty processing is done, so notify our parent by
   * closing the syncronization pipe. */
      
  safe_close(sync[0]); safe_close(sync[1]);

#define DUP_FD_OR_TTY(src, dst) dup2((src) >= 0 ? (src) : tty, dst)

  if (DUP_FD_OR_TTY(info->in[0], STDIN_FILENO) < 0)
    {
      werror("Can't dup stdin!\n");
      _exit(EXIT_FAILURE);
    }

  if (DUP_FD_OR_TTY(info->out[1], STDOUT_FILENO) < 0)
    {
      werror("Can't dup stdout!\n");
      _exit(EXIT_FAILURE);
    }

  trace("spawn_child: child before stderr dup\n");
  if (!dup_error_stream())
    {
      werror("unix_user.c: Failed to dup old stderr. Bye.\n");
      set_error_ignore();
    }

  if (DUP_FD_OR_TTY(info->err[1], STDERR_FILENO) < 0)
    {
      werror("Can't dup stderr!\n");
      _exit(EXIT_FAILURE);
    }
#undef DUP_FD_OR_TTY
      
  trace("spawn_child: after stderr dup\n");

  /* Close all the fd:s, except ones that are -1 */
  safe_close(info->in[0]);
  safe_close(info->in[1]);
  safe_close(info->out[0]);
  safe_close(info->out[1]);
  safe_close(info->err[0]);
  /* Allow the client's stdout and stderr to be the same fd, e.g.
   * both /dev/null. */
  if (info->err[1] != info->out[1])
    safe_close(info->err[1]);

  safe_close(tty);  
}

static struct lsh_process *
do_spawn(struct lsh_user *u,
	 struct spawn_info *info,
	 struct exit_callback *c)
{
  CAST(unix_user, user, u);
  /* Pipe used for syncronization. */
  int sync[2];
  pid_t child;
  
  if (!lsh_make_pipe(sync))
    {
      werror("do_spawn: Failed to create syncronization pipe.\n");
      return NULL;
    }
  
  child = fork();
  if (child < 0)
    {
      werror("unix_user: do_spawn: fork failed %e\n", errno);
      spawn_error(info, sync);

      return NULL;
    }
  else if (child)
    {
      /* Parent */
      trace("do_spawn: parent process\n");

      return spawn_parent(u, info, c,
			  user->ctx->reaper, child,
			  sync);
    }
  else
    { /* Child */
      const char *home;
      
      trace("do_spawn: child process\n");
      if (user->home)
	{
	  home = lsh_get_cstring(user->home);
	  assert(home);
	}
      else
	home= "/";
      
      spawn_child(info, home, sync);
      
      exec_shell(info, lsh_get_cstring(user->shell), home,
		 lsh_get_cstring(user->super.name), user->super.uid, user->gid);
      _exit(EXIT_FAILURE);
    }
}

static struct lsh_user *
make_unix_user(struct lsh_string *name,
	       uid_t uid, gid_t gid,
	       struct unix_user_db *ctx,
	       const char *passwd,
	       const char *home,
	       const char *shell)
{
  NEW(unix_user, user);
  
  assert(lsh_get_cstring(name));

  user->super.name = name;
  user->super.verify_password = do_verify_password;
  user->super.file_exists = do_file_exists;
  user->super.read_file = do_read_file;
  user->super.spawn = do_spawn;
  
  user->super.uid = uid;
  user->gid = gid;

  user->ctx = ctx;
  
  /* Treat empty strings as NULL. */

#define STRING(s) (((s) && *(s)) ? make_string((s)) : NULL)
  user->passwd = STRING(passwd);
  user->home = STRING(home);
  user->shell = STRING(shell);
#undef STRING
  
  return &user->super;
}
			    
/* GABA:
   (class
     (name unix_user_db)
     (super user_db)
     (vars
       (reaper object reaper)

       ; A program to use for verifying passwords.
       (pw_helper . "const char *")

       ; Override the login shell for all users.
       (login_shell . "const char *")
       
       (allow_root . int)))
*/


/* It's somewhat tricky to determine when accounts are disabled. To be
 * safe, it is recommended that all disabled accounts have a harmless
 * login-shell, like /bin/false.
 *
 * We return NULL for disabled accounts, according to the following
 * rules:
 *
 * If our uid is non-zero, i.e. we're not running as root, then an
 * account is considered valid if and only if it's uid matches the
 * server's. We never try checking the shadow record.
 *
 * If we're running as root, first check the passwd record.
 *
 * o If the uid is zero, consider the account disabled. --root-login
 *   omits this check.
 *
 * o If the passwd equals "x", look up the shadow record, check
 *   expiration etc, and replace the passwd value with the one from the
 *   shadow record. If there's no shadow record, consider the account
 *   disabled.
 *
 * o If the passwd field is empty, consider the account disabled (we
 *   usually don't want remote logins on pasword-less accounts). We may
 *   need to make this check optional, though.
 *
 * o If the passwd entry starts with a "*" and is longer than one
 *   character, consider the account disabled. (Other bogus values like
 *   "NP" means that the account is enabled, only password login is
 *   disabled)
 *
 * o Otherwise, the account is active, and a user record is returned.
 *
 * FIXME: What about systems that uses a single "*" to disable
 * accounts?
 */

static struct lsh_user *
do_lookup_user(struct user_db *s,
	       struct lsh_string *name, int free)
{
  CAST(unix_user_db, self, s);
  
  struct passwd *passwd;
  const char *home;
  const char *shell;
  const char *cname = lsh_get_cstring(name);
  char *crypted;
  uid_t me;

  if (!cname)
    {
      if (free)
	lsh_string_free(name);
      return NULL;
    }

  me = getuid();
  passwd = getpwnam(cname);

  if (!passwd)
    {
    fail:
      if (free)
	lsh_string_free(name);
      return NULL;
    }

  crypted = passwd->pw_passwd;
  
  if (!crypted || !*crypted)
    /* Ignore accounts with empty passwords. */
    goto fail;

  if (me)
    {
      /* We're not root. Disable all accounts but our own. */
      if (passwd->pw_uid != me)
	goto fail;

      /* NOTE: If we are running as the uid of the user, it seems like
       * a good idea to let the HOME environment variable override the
       * passwd-database. */
      home = getenv(ENV_HOME);
      if (!home)
	home = passwd->pw_dir;
    }
  else
    {
      /* Check for root login */
      if (!passwd->pw_uid && !self->allow_root)
	goto fail;

#if HAVE_GETSPNAM && HAVE_SHADOW_H
      {
      struct spwd *shadowpwd;

      /* FIXME: What's the most portable way to test for shadow
       * passwords? For now, we look up shadow database if and only if
       * the passwd field equals "x". If there's no shadow record, we
       * just keep the value from the passwd-database, the user may be
       * able to login using a publickey, or the password helper. */
      if (strcmp(crypted, "x") == 0
	  && (shadowpwd = getspnam(cname)))
	{
	  /* Current day number since January 1, 1970.
	   *
	   * FIXME: Which timezone is used in the /etc/shadow file? */
	  long now = time(NULL) / (3600 * 24);
	  
	  if (!(shadowpwd = getspnam(cname)))
	    goto fail;

          /* sp_expire == -1 means there is no account expiration date.
           * although chage(1) claims that sp_expire == 0 does this */
	  if ( (shadowpwd->sp_expire >= 0)
	       && (now > shadowpwd->sp_expire))
	    {
	      werror("Access denied for user '%pS', account expired.\n", name); 
	      goto fail;
	    }
	  		     
          /* sp_inact == -1 means expired password doesn't disable account.
	   *
	   * During the time
	   *
	   *   sp_lstchg + sp_max < now < sp_lstchg + sp_max + sp_inact
	   *
	   * the user is allowed to log in only by changing her
	   * password. As lsh doesn't support password change, this
	   * means that access is denied. */

          if ( (shadowpwd->sp_inact >= 0) &&
	       (now > (shadowpwd->sp_lstchg + shadowpwd->sp_max)))
            {
	      werror("Access denied for user '%pS', password too old.\n", name);
	      goto fail;
	    }

	  /* FIXME: We could look at sp_warn and figure out if it is
	   * appropriate to send a warning about passwords about to
	   * expire, and possibly also a
	   * SSH_MSG_USERAUTH_PASSWD_CHANGEREQ message.
	   *
	   * A warning is appropriate when
	   *
	   *   sp_lstchg + sp_max - sp_warn < now < sp_lstchg + sp_max
	   *
	   */

	  crypted = shadowpwd->sp_pwdp;
	}
      }
#endif /* HAVE_GETSPNAM */
      /* Check again for empty passwd field (as it may have been
       * replaced by the shadow one). */
      if (!crypted || !*crypted)
	goto fail;

      /* A passwd field of more than one character, which starts with a star,
       * indicates a disabled account. */
      if ( (crypted[0] == '*') && crypted[1])
	goto fail;
      
      home = passwd->pw_dir;
    }
  
  if (self->login_shell)
    /* Override the passwd database */
    shell = self->login_shell;
  else
    /* Default login shell is /bin/sh */
    shell = passwd->pw_shell ? passwd->pw_shell : "/bin/sh";
      
  return make_unix_user(free ? name : lsh_string_dup(name), 
			passwd->pw_uid, passwd->pw_gid,
			self,
			crypted,
			home, shell);
}

struct user_db *
make_unix_user_db(struct reaper *reaper,
		  const char *pw_helper, const char *login_shell,
		  int allow_root)
{
  NEW(unix_user_db, self);

  self->super.lookup = do_lookup_user;
  self->reaper = reaper;
  self->pw_helper = pw_helper;
  self->login_shell = login_shell;
  self->allow_root = allow_root;

  return &self->super;
}


/* Class representing the user the server is executing as. Used with
   the login/telnet mode. */
/* GABA:
   (class
     (name unix_user_self)
     (super lsh_user)
     (vars
       (reaper object reaper)
       ; login shell to use
       (shell . "const char *")
       (home . "const char *")))
*/

/* FIXME: Do we need these methods, or is it better to just use NULL
   pointers? */
static void
do_verify_password_fail(struct lsh_user *s UNUSED,
			struct lsh_string *password UNUSED,
			struct command_continuation *c UNUSED,
			struct exception_handler *e)
{
  static const struct exception invalid_passwd
    = STATIC_EXCEPTION(EXC_USERAUTH, "Incorrect password.");

  EXCEPTION_RAISE(e, &invalid_passwd);
}

static int
do_file_exists_fail(struct lsh_user *u UNUSED,
		    struct lsh_string *name,
		    int free)
{
  if (free)
    lsh_string_free(name);
  
  return 0;
}

static const struct exception *
do_read_file_fail(struct lsh_user *u UNUSED, 
		  const char *name UNUSED, int secret UNUSED,
		  uint32_t limit UNUSED,
		  struct abstract_write *c UNUSED)
{
  return make_io_exception(EXC_IO_OPEN_READ, NULL, 0, "Access denied.");  
}

static struct lsh_process *
do_spawn_self(struct lsh_user *u,
	      struct spawn_info *info,
	      struct exit_callback *c)
{
  CAST(unix_user_self, self, u);
  /* Pipe used for syncronization. */
  int sync[2];
  pid_t child;
  
  if (!lsh_make_pipe(sync))
    {
      werror("do_spawn: Failed to create syncronization pipe.\n");
      return NULL;
    }
  
  child = fork();
  if (child < 0)
    {
      werror("unix_user: do_spawn_self: fork failed %e\n", errno);
      spawn_error(info, sync);

      return NULL;
    }
  else if (child)
    {
      /* Parent */
      trace("do_spawn_self: parent process\n");

      return spawn_parent(u, info, c,
			  self->reaper, child,
			  sync);
    }
  else
    { /* Child */
      trace("do_spawn_self: child process\n");
      spawn_child(info, self->home, sync);
      
      exec_shell(info, self->shell, self->home,
		 lsh_get_cstring(self->super.name), getuid(), -1);
      _exit(EXIT_FAILURE);
    }
}

struct lsh_user *
make_unix_user_self(struct lsh_string *name,
		    struct reaper *reaper,
		    const char *home, const char *shell)
{
  NEW(unix_user_self, self);
  self->super.name = name;
  self->super.uid = getuid();
  self->super.verify_password = do_verify_password_fail;
  self->super.file_exists = do_file_exists_fail;
  self->super.read_file = do_read_file_fail;
  self->super.spawn = do_spawn_self;

  self->reaper = reaper;
  self->shell = shell;
  self->home = home;

  return &self->super;
}
