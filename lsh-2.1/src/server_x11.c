/* server_x11.c
 *
 */

/* lsh, an implementation of the ssh protocol
 *
 * Copyright (C) 2002 Niels Möller
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/types.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>

#include "server_x11.h"

#include "channel_commands.h"
#include "channel_forward.h"
#include "environ.h"
#include "format.h"
#include "lsh_string.h"
#include "reaper.h"
#include "resource.h"
#include "userauth.h"
#include "werror.h"
#include "xalloc.h"


#define GABA_DEFINE
#include "server_x11.h.x"
#undef GABA_DEFINE

/* Forward declarations */

/* FIXME: Should be static */
struct command open_forwarded_x11;
#define OPEN_FORWARDED_X11 (&open_forwarded_x11.super)

#include "server_x11.c.x"


#if WITH_X11_FORWARD

#ifndef SUN_LEN
# define SUN_LEN(x) \
  (offsetof(struct sockaddr_un, sun_path) + strlen((x)->sun_path))
#endif

#define X11_WINDOW_SIZE 10000

/* GABA:
   (class
     (name channel_open_command_x11)
     (super channel_open_command)
     (vars
       (peer object listen_value)))
*/

static struct ssh_channel *
new_x11_channel(struct channel_open_command *c,
		struct ssh_connection *connection,
		uint32_t local_channel_number,
		struct lsh_string **request)
{
  CAST(channel_open_command_x11, self, c);
  struct ssh_channel *channel;

  /* NOTE: All accepted fd:s must end up in this function, so it
   * should be ok to delay the REMEMBER call until here. It is done
   * by make_channel_forward. */

  debug("server_x11.c: new_x11_channel\n");

  channel = &make_channel_forward(self->peer->fd, X11_WINDOW_SIZE)->super;
  channel->connection = connection;

  /* NOTE: The request ought to include some reference to the
   * corresponding x11 request, but no such id is specified in the
   * protocol spec. */
  /* NOTE: The name "unix-domain" was suggested by Tatu in
   * <200011290813.KAA17106@torni.hel.fi.ssh.com> */
  *request = format_channel_open(ATOM_X11, local_channel_number,
				 channel,
				 "%z%i", "unix-domain", 0);
  
  return channel;
}

DEFINE_COMMAND(open_forwarded_x11)
     (struct command *s UNUSED,
      struct lsh_object *x,
      struct command_continuation *c,
      struct exception_handler *e UNUSED)      
{
  CAST(listen_value, peer, x);

  NEW(channel_open_command_x11, self);
  self->super.super.call = do_channel_open_command;
  self->super.new_channel = new_x11_channel;

  self->peer = peer;

  COMMAND_RETURN(c, self);
}

/* Quite similar to forward_local_port in tcpforward_commands.c */
/* GABA:
   (expr
     (name server_x11_callback)
     (params
       (connection object ssh_connection))
     (expr
       ;; FIXME: This is a common construction for all types of
       ;; forwardings.
       (lambda (peer)
	 (start_io
	   (catch_channel_open 
       	     (open_forwarded_x11 peer) connection)))))
*/
	     
#define XAUTH_DEBUG_TO_STDERR 0
#define X11_MIN_COOKIE_LENGTH 10
#define X11_SOCKET_DIR "/tmp/.X11-unix"

/* The interval of display numbers that we use. */
#define X11_MIN_DISPLAY 10
#define X11_MAX_DISPLAY 1000

/* FIXME: Create the /tmp/.X11-unix directory, if needed. Figure out
 * if and how we should use /tmp/.X17-lock. Consider using display
 * "unix:17" instead of just ":17".
 */

/* GABA:
   (class
     (name server_x11_socket)
     (super resource)
     (vars
       ; fd to the directory where the socket lives
       (dir . int)
       ; Name of the local socket
       (name string)
       (display_number . int)
       ; The user that should own the socket
       (uid . uid_t)
       ; The corresponding listening fd
       (fd object lsh_fd)))
*/

/* This code is quite paranoid in order to avoid symlink attacks when
 * creating and deleting the socket. Similar paranoia in xlib would be
 * desirable, but not realistic. However, most of this is not needed
 * if the sticky bit is set properly on the /tmp and /tmp/.X11-unix
 * directories. */

static void
delete_x11_socket(struct server_x11_socket *self)
{
  if (unlink(lsh_get_cstring(self->name)) < 0)
    werror("Failed to delete x11 socket %S for user %i %e\n",
	   self->name, self->uid, errno);
}

static void
do_kill_x11_socket(struct resource *s)
{
  CAST(server_x11_socket, self, s);
  uid_t me = geteuid();
  int old_cd;

  if (self->super.alive)
    {
      self->super.alive = 0;

      assert(self->fd);
      close_fd(self->fd);

      assert(self->dir >= 0);

      /* Temporarily change to the right directory. */
      old_cd = lsh_pushd_fd(self->dir);
      if (old_cd < 0)
	return;

      close(self->dir);
      self->dir = -1;
      
      if (me == self->uid)
	delete_x11_socket(self);
      else
	{
	  if (seteuid(self->uid) < 0)
	    {
	      werror("Couldn't change euid (to %i) for deleting x11 socket.\n",
		     self->uid);
	      goto done;
	    }
	  assert(geteuid() == self->uid);

	  delete_x11_socket(self);
	  
	  if (seteuid(me) < 0)
	    /* Shouldn't happen, abort if it ever does. */
	    fatal("Failed to restore euid after deleting x11 socket.\n");

	  assert(geteuid() == me);
	}

    done:
      lsh_popd(old_cd, X11_SOCKET_DIR);
    }
}

/* The processing except the uid change stuff. */
static struct server_x11_socket *
open_x11_socket(struct ssh_channel *channel)
{
  int old_cd;
  int dir;
  mode_t old_umask;
  
  int number;
  struct lsh_fd *s;
  struct lsh_string *name = NULL;

  /* We have to change the umask, as that's the only way to control
   * the permissions that bind uses. */

  old_umask = umask(0077);
  
  old_cd = lsh_pushd(X11_SOCKET_DIR, &dir, 0, 0);
  if (old_cd < 0)
    {
      werror("Failed to cd to `%z' %e\n", X11_SOCKET_DIR, errno);

      umask(old_umask);
      return NULL;
    }
  
  for (number = X11_MIN_DISPLAY; number <= X11_MAX_DISPLAY; number++)
    {
      /* The default size if sockaddr_un should always be enough to format
       * the filename "X<display num>". */
      struct sockaddr_un sa;
  
      sa.sun_family = AF_UNIX;
      sa.sun_path[sizeof(sa.sun_path) - 1] = '\0';
      snprintf(sa.sun_path, sizeof(sa.sun_path), "X%d", number);

      s = io_bind_sockaddr((struct sockaddr *) &sa, SUN_LEN(&sa),
			   /* Use the connection's exception handler,
			    * as the channel may live shorter.
			    *
			    * FIXME: How does that handle i/o errors?
			    */
			   channel->connection->e);
      if (s)
	{
	  /* Store name */
	  name = ssh_format("%lz", sa.sun_path);
	  break;
	}
    }

  umask(old_umask);
  
  lsh_popd(old_cd, X11_SOCKET_DIR);

  if (!name)
    {
      /* Couldn't find any display */
      close(dir);
      
      return NULL;
    }

  {
    CAST_SUBTYPE(command, callback, server_x11_callback(channel->connection));
    
    if (!io_listen(s, make_listen_callback(callback,
					   channel->connection->e)))
      {
	close(dir);
	close_fd(s);
	return NULL;
      }
  }
  {
    NEW(server_x11_socket, self);
    init_resource(&self->super, do_kill_x11_socket);

    self->dir = dir;
    self->name = name;
    self->fd = s;
    self->display_number = number;
    
    remember_resource(channel->resources, &self->super);
    return self;
  }
}

/* Creates a socket in tmp, accessible only by the right user. */
static struct server_x11_socket *
server_x11_listen(struct ssh_channel *channel)
{
  struct server_x11_socket *s;
  
  uid_t me = geteuid();
  uid_t user = channel->connection->user->uid;
  
  if (me == channel->connection->user->uid)
    s = open_x11_socket(channel);
  else
    {      
      if (seteuid(user) < 0)
	{
	  /* FIXME: We could display the user name here. */
	  werror("Couldn't change euid (to %i) for creating x11 socket.\n",
		 user);
	  return NULL;
	}

      assert(geteuid() == user);
      
      s = open_x11_socket(channel);
      
      if (seteuid(me) < 0)
	/* Shouldn't happen, abort if it ever does. */
	fatal("Failed to restore euid after deleting x11 socket.\n");
      
      assert(geteuid() == me);
    }
  return s;
}

/* GABA:
   (class
     (name xauth_exit_callback)
     (super exit_callback)
     (vars
       (c object command_continuation)
       (e object exception_handler)))
*/

static void
do_xauth_exit(struct exit_callback *s, int signaled,
	      int core UNUSED, int value)
{
  CAST(xauth_exit_callback, self, s);
  
  if (signaled || value)
    {
      /* xauth failed */
      const struct exception xauth_failed
	= STATIC_EXCEPTION(EXC_CHANNEL_REQUEST, "xauth failed");
      EXCEPTION_RAISE(self->e, &xauth_failed);
      if (signaled)
	werror("xauth invocation failed: Signal %i\n", value);
      else
	werror("xauth invocation failed: exit code: %i\n", value);
    }
  else
    /* FIXME: Doesn't return the channel. */
    COMMAND_RETURN(self->c, NULL);
}

static struct exit_callback *
make_xauth_exit_callback(struct command_continuation *c,
			 struct exception_handler *e)
{
  NEW(xauth_exit_callback, self);
  self->super.exit = do_xauth_exit;
  self->c = c;
  self->e = e;

  return &self->super;
}

/* NOTE: We don't check the arguments for spaces or other magic
 * characters. The xauth process in unprivileged, and the user is
 * properly authenticated to call it with arbitrary commands. We still
 * check for NUL characters, though. */
static int
bad_string(uint32_t length, const uint8_t *data)
{
  return !!memchr(data, '\0', length);
}

/* FIXME: Use autoconf */
#ifndef XAUTH_PROGRAM
# define XAUTH_PROGRAM "/usr/X11R6/bin/xauth"
#endif

/* On success, returns 1 and sets *DISPLAY and *XAUTHORITY */
struct server_x11_info *
server_x11_setup(struct ssh_channel *channel, struct lsh_user *user,
		 int single,
		 uint32_t protocol_length, const uint8_t *protocol,
		 uint32_t cookie_length, const uint8_t *cookie,
		 uint32_t screen,
		 struct command_continuation *c,
		 struct exception_handler *e)
{
  struct lsh_string *display;
  struct lsh_string *xauthority;
  struct server_x11_socket *socket;
  
  const char *tmp;

  if (single)
    {
      werror("server_x11_setup: Single forwardings not yet supported.\n");
      return NULL;
    }
  
  if (bad_string(protocol_length, protocol))
    {
      werror("server_x11_setup: Bogus protocol name.\n");
      return NULL;
    }
  
  if (bad_string(cookie_length, cookie))
    {
      werror("server_x11_setup: Bogus cookie.\n");
      return NULL;
    }
  
  if (cookie_length < X11_MIN_COOKIE_LENGTH)
    {
      werror("server_x11_setup: Cookie too small.\n");
      return NULL;
    }

  /* Get a free socket under /tmp/.X11-unix/ */
  socket = server_x11_listen(channel);
  if (!socket)
    return NULL;

  tmp = getenv(ENV_TMPDIR);
  if (!tmp)
    tmp = "/tmp";
  
  display = ssh_format(":%di.%di", socket->display_number, screen);
  xauthority = ssh_format("%lz/.lshd.%lS.Xauthority", tmp, user->name);
  
  {
    struct spawn_info spawn;
    const char *args[2] = { "-c", XAUTH_PROGRAM };
    struct env_value env;

    struct lsh_process *process;

    int null;

    env.name = "XAUTHORITY";
    env.value = xauthority;
    
    memset(&spawn, 0, sizeof(spawn));
    /* FIXME: Arrange that stderr data (and perhaps stdout data as
     * well) is sent as extended data on the channel. To do that, we
     * need another channel flag to determine whether or not EOF
     * should be sent when the number of sources gets down to 0. */
#if XAUTH_DEBUG_TO_STDERR
    null = dup(STDERR_FILENO);
#else
    null = open("/dev/null", O_WRONLY);
#endif
    if (null < 0)
      goto fail;

    /* [0] for reading, [1] for writing */
    if (!lsh_make_pipe(spawn.in))
      {
	close(null);
	goto fail;
      }
    spawn.out[0] = -1; spawn.out[1] = null;
    spawn.err[0] = -1; spawn.err[1] = null;
    
    spawn.peer = NULL;
    spawn.pty = NULL;
    spawn.login = 0;
    spawn.argc = 2;
    spawn.argv = args;
    spawn.env_length = 1;
    spawn.env = &env;

    process = USER_SPAWN(user, &spawn, make_xauth_exit_callback(c, e));
    if (process)
      {
	NEW(server_x11_info, info);
	static const struct report_exception_info report =
	  STATIC_REPORT_EXCEPTION_INFO(EXC_IO, EXC_IO, "writing xauth stdin");

	struct lsh_fd *in
	  = io_write(make_lsh_fd(spawn.in[1], IO_NORMAL,
				 "xauth stdin",
				 make_report_exception_handler
				 (&report, e, HANDLER_CONTEXT)),
		     1024, NULL);

	A_WRITE(&in->write_buffer->super,
		/* NOTE: We pass arbitrary data to the xauth process,
		 * if the user so wish. */
		 ssh_format("add %lS %ls %ls\n",
			   display,
			   protocol_length, protocol,
			   cookie_length, cookie));
	close_fd_write(in);

	remember_resource(channel->resources, &process->super);
	remember_resource(channel->resources, &in->super);	
	
	info->display = display;
	info->xauthority = xauthority;
	
	return info;
      }
    else
      {
	close(spawn.in[0]);
	close(spawn.in[1]);
      fail:
	lsh_string_free(display);
	lsh_string_free(xauthority);
	return NULL;
      }
  }
}

#endif /* WITH_X11_FORWARD */
